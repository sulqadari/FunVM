#include "compiler.h"
#include "scanner.h"
#include "bytecode.h"
#include "object.h"
#include "vm.h"

typedef struct {
	Token current;
	Token previous;
	bool hadError;	/* records a errors occured during compilation. */
	bool panicMode;	/* avoids error cascades. */
} Parser;

typedef enum {
	prec_none,
	prec_assignment,// =
	prec_or,		// |
	prec_and,		// &
	prec_equality,	// == !=
	prec_comparison,// < > <= >=
	prec_term,		// + -
	prec_factor,	// * /
	prec_unary,		// ! -
	prec_call,		// . ()
	prec_primary,
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence prec;
} ParseRule;

typedef struct {
	Token name;
	int32_t depth;
} Local;

typedef struct {
	Local locals[STACK_SIZE];
	int32_t localCount;
	int32_t scopeDepth;
} Compiler;

static Parser parser;
static Compiler* currCplr = NULL;
static ByteCode* currCtx;

static ByteCode*
getCurrentCtx(void)
{
	return currCtx;
}

static void
errorAt(Token* token, const char* message)
{
	if (true == parser.panicMode)
		return;
	
	parser.panicMode = true;

	fprintf(stderr, "[line %d] Error", token->line);

	switch (token->type) {
		case tkn_err:
			// Nothing to do.
		break;
		case tkn_eof:
			fprintf(stderr, " at end");
		break;
		default:
			fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
}

static void
error(const char* message)
{
	errorAt(&parser.previous, message);
}

static void
errorAtCurrent(const char* message)
{
	errorAt(&parser.current, message);
}

static void
advance(void)
{
	parser.previous = parser.current;

	/* Keep looping until encounter a non-error token or reach the end of file. */
	while (true) {
		parser.current = scanToken();
		if (parser.current.type != tkn_err)
			break;
		
		errorAtCurrent(parser.current.start);
	}
}

static void
consume(TokenType type, const char* message)
{
	if (type == parser.current.type) {
		advance();
		return;
	}

	errorAtCurrent(message);
}

/** Checks wether a current token is of given type.*/
static bool
check(const TokenType type)
{
	return type == parser.current.type;
}

/**
 * Advances the parser if current token is of given type.
 * @returns true if match.
 */
static bool
match(TokenType type)
{
	if (!check(type))
		return false;

	advance();
	return true;
}

static void
emitByte(uint8_t byte)
{
	writeByteCode(getCurrentCtx(), byte, parser.previous.line);
}

static void
emitBytes(uint8_t byte1, uint8_t byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

static void
emitShort(uint16_t shrt)
{
	emitBytes(((shrt >> 8) & 0x00FF), (shrt & 0x00FF));
}

static void
emitLoop(int32_t loopStart)
{
	emitByte(op_loop);
	
	int32_t offset = getCurrentCtx()->count - loopStart + 2;
	if (offset > UINT16_MAX)
		error("Loop body too large");
	
	emitShort((uint16_t)offset);
}

/**
 * Produces a bytecode that forces an execution flow to jump over a chunk of bytecode.
 * @returns int32_t offset of the opcode in the bytecode.
 */
static int32_t
emitJump(OpCode opcode)
{
	emitByte(opcode);						// currCtx->count == 1 when we return from this function
	emitShort(0xffff);						// currCtx->count == 3 when we return from this function

	return getCurrentCtx()->count - 2;		// 3 - 2 = 1;
}

/**
 * Updates the value of the operand of the bytecode, produced by the emitJump() function.
 * @param int32_t number of bytes to jump over. */
static void
patchJump(int32_t offset)
{
	int32_t jumpOver = getCurrentCtx()->count - offset - 2;	// Consider, that emitJump() returned offset #1. After that we processed a
	if (jumpOver >= UINT16_MAX) {							// 'then' branch which produced 10 bytes of code. Now, the 'currCtx->count'
		error("Too much code to jump over");				// equals 14 (keep in mind, that emitJump() incremented the 'count' variable by '3').
	}														// The following arithmetic expression reveals, that we need to jump
	getCurrentCtx()->code[offset] = (jumpOver >> 8) & 0xff;	// over ((14 - 1 - 2) = 11) bytes of code. Here '2' adjusts the jump offset itself.
	getCurrentCtx()->code[offset + 1] = jumpOver    & 0xff;
}

static void
emitReturn(void)
{
	emitByte(op_ret);
}

static uint16_t
makeConstant(Value value)
{
	int32_t idx = addConstant(getCurrentCtx(), value);
	if (idx > UINT16_MAX) {
		error("Too many constants in one chunk.");
		exit(1);
	}

	return (uint16_t)idx;
}

static void
emitConstant(Value value)
{
	uint16_t idx = makeConstant(value);
	if (idx <= UINT8_MAX) {
		emitBytes(op_iconst, idx);
	} else {
		emitByte(op_iconstw);
		emitShort(idx);
	}
}

static void
commitCompilation(void)
{
	emitReturn();
}

static void
beginScope(void)
{
	currCplr->scopeDepth++;
}

static void
endScope(void)
{
	uint16_t count = 0;

	currCplr->scopeDepth--;
	while (currCplr->localCount > 0														// When leaving a scope, count the amount of variables that must be
		&& currCplr->locals[currCplr->localCount - 1].depth > currCplr->scopeDepth)		// discarded from the stack.
	{
		count++;					// Instead of producing 'op_pop' for each variable to be removed from the stack,
		currCplr->localCount--;		// count their number.
	}

	emitByte(op_popn);				// Use this common bytecode instruction which will shrink the top of the stack
	emitShort(count);				// to the given length.
}

static void expression(void);
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static void statement(void);
static void declaration(void);
static void namedVariable(Token name, bool canAssign);

static void
binary(bool canAssign)
{
	TokenType opType = parser.previous.type;
	ParseRule* rule = getRule(opType);
	parsePrecedence((Precedence)(rule->prec + 1));

	switch (opType) {
		case tkn_neq:  emitBytes(op_eq, op_not); break;
		case tkn_2eq:  emitByte(op_eq);          break;
		case tkn_gt:   emitByte(op_gt);          break;
		case tkn_gteq: emitBytes(op_lt, op_not); break;
		case tkn_lt:   emitByte(op_lt);          break;
		case tkn_lteq: emitBytes(op_gt, op_not); break;

		case tkn_plus:  emitByte(op_add); break;
		case tkn_minus: emitByte(op_sub); break;
		case tkn_star:  emitByte(op_mul); break;
		case tkn_slash: emitByte(op_div); break;
		default: return; // Unreachable
	}
}

static void
literal(bool canAssign)
{
	switch (parser.previous.type) {
		case tkn_null:  emitByte(op_null);  break;
		case tkn_false: emitByte(op_false); break;
		case tkn_true:  emitByte(op_true);  break;
		default: return; // UNreachable.
	}
}

static void
grouping(bool canAssign)
{
	expression();
	consume(tkn_rparen, "Expect ')' after expression.");
}

static void
number(bool canAssign)
{
	int32_t value = strtol(parser.previous.start, NULL, 10);
	emitConstant(NUM_PACK(value));
}

static void
string(bool canAssign)
{
	/* Trim the leading and trailing quotation marks. */
	emitConstant(OBJ_PACK(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void
variable(bool canAssign)
{
	namedVariable(parser.previous, canAssign);
}

static void
unary(bool canAssign)
{
	TokenType opType = parser.previous.type;
	parsePrecedence(prec_unary);

	switch (opType) {
		case tkn_not:   emitByte(op_not);    break;
		case tkn_minus: emitByte(op_negate); break;
		default: return; // Unreachable
	}
}

/**
 * When this function is called, the value of left-hand side expression is already on the stack.
 * If that value is falsey, then it will be keeped on the stack.
 */
static void
_and(bool canAssign)
{
	int32_t endJump = emitJump(op_jmp_false);	// Skip entire clause if preceding condition is falsey.
	emitByte(op_pop);							// Otherwise: discard the l-hand side value and...
	parsePrecedence(prec_and);					// ...evaluate the right operand.
	patchJump(endJump);
}

static void
_or(bool canAssign)
{
	int32_t elseJump = emitJump(op_jmp_false);
	int32_t endJump = emitJump(op_jmp);

	patchJump(elseJump);
	emitByte(op_pop);

	parsePrecedence(prec_or);
	patchJump(endJump);
}

ParseRule rules[] = {
	[tkn_lparen]   = {grouping, NULL, prec_none},
	[tkn_rparen]   = {NULL,     NULL, prec_none},
	[tkn_lbrace]   = {NULL,     NULL, prec_none},
	[tkn_rbrace]   = {NULL,     NULL, prec_none},
	[tkn_lbracket] = {NULL,     NULL, prec_none},
	[tkn_rbracket] = {NULL,     NULL, prec_none},
	[tkn_semicolon] = {NULL,    NULL, prec_none},
	[tkn_comma]    = {NULL,     NULL, prec_none},
	[tkn_dot]      = {NULL,     NULL, prec_none},
	[tkn_minus]    = {unary,    binary, prec_term},
	[tkn_plus]     = {NULL,     binary, prec_term},
	[tkn_slash]    = {NULL,     binary, prec_factor},
	[tkn_star]     = {NULL,     binary, prec_factor},
	
	[tkn_not]      = {unary,    NULL,   prec_none},
	[tkn_neq]      = {NULL,     binary, prec_equality},
	[tkn_eq]       = {NULL,     NULL,   prec_none},
	[tkn_2eq]      = {NULL,     binary, prec_equality},
	[tkn_gt]       = {NULL,     binary, prec_comparison},
	[tkn_gteq]     = {NULL,     binary, prec_comparison},
	[tkn_lt]       = {NULL,     binary, prec_comparison},
	[tkn_lteq]     = {NULL,     binary, prec_comparison},
	[tkn_and]      = {NULL,     _and,   prec_and},
	[tkn_or]       = {NULL,     _or,    prec_or},
	
	[tkn_id]       = {variable, NULL, prec_none},
	[tkn_str]      = {string,   NULL, prec_none},

	[tkn_var]      = {number,   NULL, prec_none},
	[tkn_if]       = {NULL,     NULL, prec_none},
	[tkn_else]     = {NULL,     NULL, prec_none},
	[tkn_switch]   = {NULL,     NULL, prec_none},
	[tkn_break]    = {NULL,     NULL, prec_none},
	[tkn_while]    = {NULL,     NULL, prec_none},
	[tkn_for]      = {NULL,     NULL, prec_none},
	[tkn_continue] = {NULL,     NULL, prec_none},
	[tkn_class]    = {NULL,     NULL, prec_none},
	[tkn_super]    = {NULL,     NULL, prec_none},
	[tkn_this]     = {NULL,     NULL, prec_none},
	[tkn_fun]      = {NULL,     NULL, prec_none},
	[tkn_null]     = {literal,  NULL, prec_none},
	[tkn_ret]      = {NULL,     NULL, prec_none},
	[tkn_false]    = {literal,  NULL, prec_none},
	[tkn_true]     = {literal,  NULL, prec_none},
	[tkn_err]      = {NULL,     NULL, prec_none},
	[tkn_eof]      = {NULL,     NULL, prec_none},
};


static ParseRule*
getRule(TokenType type)
{
	return &rules[type];
}

/** Parses an expression at the given precedence level or higher. */
static void
parsePrecedence(Precedence prec)
{
	advance();
	ParseFn prefixRule = getRule(parser.previous.type)->prefix;

	if (prefixRule == NULL) {
		error("expect expression.");
		return;
	}

	bool canAssign = (prec <= prec_assignment);
	prefixRule(canAssign);

	while (getRule(parser.current.type)->prec >= prec) {
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule(canAssign);
	}

	if (canAssign && match(tkn_eq)) {
		error("Invalid assignment target.");
	}
}


static bool
identifiersEqual(Token* a, Token* b)
{
	if (a->length != b->length)
		return false;
	
	return memcmp(a->start, b->start, a->length) == 0;
}

static int32_t
resolveLocal(Compiler* compiler, Token* name)
{
	for (int32_t i = compiler->localCount - 1; i >= 0; --i) {
		
		Local* local = &compiler->locals[i];
		if (identifiersEqual(name, &local->name)) {	
			if (local->depth == -1) {	// we can't resolve a variable which is declared, but not defined.
				error("Can't read local variable in its own initializer.");
			}
			return i;
		}
	}

	return -1;
}

/**
 * Adds the lexeme of a given token to the bytecode's constant table as a string.
 * @returns uint16_t index of the constant in the constant pool.
 * */
static uint16_t
identifierConstant(Token* name)
{
	return makeConstant(OBJ_PACK(copyString(name->start, name->length)));
}

static void
namedVariable(Token name, bool canAssign)
{
	OpCode getOp, setOp;
	int32_t arg = resolveLocal(currCplr, &name);

	if (arg != -1) {
		getOp = op_get_locvar;
		setOp = op_set_locvar;
	} else {
		arg = identifierConstant(&name);
		getOp = op_get_gvar;
		setOp = op_set_gvar;
	}

	if (canAssign && match(tkn_eq)) {
		expression();
		if (arg <= UINT8_MAX) {
			emitBytes(setOp, arg);
		} else {
			emitByte(setOp + 1);
			emitShort(arg);
		}
	} else {
		if (arg <= UINT8_MAX) {
			emitBytes(getOp, arg);
		} else {
			emitByte(getOp + 1);
			emitShort(arg);
		}
	}
}

/**
 * Adds a local variable to the compiler's list of locals in the current scope.
 */
static void
addLocal(Token name)
{
	if (currCplr->localCount > STACK_SIZE) {
		error("Too many local variables in function");
		return;
	}

	Local* local = &currCplr->locals[currCplr->localCount++];
	local->name = name;
	local->depth = -1;	// marks a local as declared, but not defined. The difference between these states is that,
}						// the 'declared' variable can't be used. 

/**
 * Records the existence of a local variable.
 */
static void
declareVariable(void)
{
	if (currCplr->scopeDepth == 0)
		return;	// Leave if we are in global scope.
	
	Token* name = &parser.previous;

	// Detect two or more variables with the same name in joint scope.
	for (int32_t i = currCplr->localCount - 1; i >= 0; --i) {				// Starting from the innermost scope, which is current one, interate through the array.
		Local* local = &currCplr->locals[i];
		if (local->depth != -1 && local->depth < currCplr->scopeDepth) {	// if local is defined, and its depth is less than currCplr's one, then that means that we
			break;															// didn't find a variable with the same name in current scope and stepped back to outer one.
		}																	// We don't consider to having a variable with the same name in outer scope as an error.
																			// Thus, just stop looping.
		if (identifiersEqual(name, &local->name)) {
			error("The variable is already declared in this scope");
		}
	}

	addLocal(*name);
}

/**
 * Consumes the identifier token for the variable name and adds its lexeme to the bytecode's
 * constatn table as a string.
 * @returns uint16_t the constant table index of the lexeme.
 */
static uint16_t
parseVariable(const char* errorMessage)
{
	// Report an error if the current token isn't variable's name.
	consume(tkn_id, errorMessage);

	declareVariable();
	if (currCplr->scopeDepth > 0)	// Halt further execution if we're in a local scope.
		return 0;					// In other words, the local variable's name shouldn't be stored in the constant pool.
	
	return identifierConstant(&parser.previous);
}

static void
markInitialized(void)
{
	currCplr->locals[currCplr->localCount - 1].depth = currCplr->scopeDepth;
}

/**
 * Produces the bytecode instruction that defines the new variable
 * and stores its initial value.
 * @param uint16_t the index of the variable's name in the constant table.
 */
static void
defineVariable(uint16_t global)
{
	if (currCplr->scopeDepth > 0) {
		markInitialized();
		return;		// There is no bytecode to create a local variable at runtime.
	}
	
	if (global <= UINT8_MAX) {
		emitBytes(op_def_gvar, global);
	} else {
		emitByte(op_def_gvarw);
		emitShort(global);
	}
}

static void
expression(void)
{
	parsePrecedence(prec_assignment);
}

static void
block(void)
{
	while (!check(tkn_rbrace) && !check(tkn_eof)) {
		declaration();
	}

	consume(tkn_rbrace, "Expect '}' after block");
}

static void
varDeclaration(void)
{
	uint16_t global = parseVariable("Expect variable name.");

	if (match(tkn_eq)) {
		expression();
	} else {
		emitByte(op_null);
	}

	consume(tkn_semicolon, "Expect ';' after variable declaration");
	defineVariable(global);
}

static void
expressionStatement(void)
{
	expression();
	consume(tkn_semicolon, "Expect ';' after value.");
	emitByte(op_pop);
}

static void
ifStatement(void)
{
	consume(tkn_lparen, "Expect '(' after 'if'.");
	expression();									// expr within the if() statement
	consume(tkn_rparen, "Expect ')' after 'if'.");

	int32_t jmpOverThen = emitJump(op_jmp_false);	// Jump over the 'then' branch if expr in 'if()' stmt is falsey, e.g. proceed to 'else'.
	emitByte(op_pop);								// Otherwise, if we're in 'if(){ }', then first of all pop out result of 'if('expr')' of the stack.
	statement();									// Process the 'then branch'.
													
	int32_t jmpOverElse = emitJump(op_jmp);			// If exec flow entered the 'then' branch, then this instruction will force it to
													// jump over the 'else' branch.
	patchJump(jmpOverThen);
	
	// Pop expr at the beginning of the 'else' branch.
	emitByte(op_pop);
	if (match(tkn_else))
		statement();

	patchJump(jmpOverElse);	// count the actual number of bytes in 'else' branch we need to jump over.
}

static void
printStatement(void)
{
	consume(tkn_lparen, "Expect '(' after 'print'.");
	expression();
	consume(tkn_rparen, "Expect ')' after 'print'.");
	consume(tkn_semicolon, "Expect ';' after print().");
	emitByte(op_print);
}

static void
whileStatement(void)
{
	int32_t loopStart = getCurrentCtx()->count;

	consume(tkn_lparen, "Expect '(' after 'while'");
	expression();
	consume(tkn_rparen, "Expect ')' after condition");

	int32_t exitJump = emitJump(op_jmp_false);
	emitByte(op_pop);
	statement();

	emitLoop(loopStart);

	patchJump(exitJump);
}

static void
forStatement(void)
{
	beginScope();
	consume(tkn_lparen, "Expect '(' after 'for'.");

	// empty initializer case.
	if (check(tkn_semicolon)) {		// This clause is desugared intentionally: instead of using match(), which combines check() and advance(), 
		advance();					// we call them separately to avoid leaving this clause empty. Empty clause might be optimized by compiler.
	} else if (match(tkn_var)) {	// A user declares a new variable.
		varDeclaration();
	} else {						// All other cases go this clause.
		expressionStatement();		// This function is used instead of expression() to detect 
	}								// the mandatory semicolon and produce the op_pop bytecode.

	int32_t loopStart = getCurrentCtx()->count;	// Loop starting point.
	
	int32_t exitJump = -1;			// negative value designates absence of condition clause.

	// In case the condition expression clause isn't empty. 
	if (!match(tkn_semicolon)) {	// If next token isn't of type semicolon,
		expression();				// there must be a condition expression.
		consume(tkn_semicolon, "Expect ';' after loop condition.");

		// Jump out of the loop if condition is false.
		exitJump = emitJump(op_jmp_false);	// condition expression leaves the value on top of the stack, and op_jmp_false leaves it untouched too.
		emitByte(op_pop);					// thus, it must be poped off before executing the body.
	}

	// In case the increment clause isn't empty.
	if (!match(tkn_rparen)) {

		int32_t bodyJump = emitJump(op_jmp);				// 1. Hop over the increment clause for the first time.
		int32_t incrementStart = getCurrentCtx()->count;
		
		expression();										// Execute increment expression and pop it off, because all that we need
		emitByte(op_pop);									// is its side effect (e.g. a variable with changed value). 
		consume(tkn_rparen, "Expect ')' after 'for' clauses.");

		emitLoop(loopStart);								// Take us back to the top fo the for loop, right before the condition expression.
		loopStart = incrementStart;							// Change the loopStart to point to the offset where the increment expression begins.
		patchJump(bodyJump);
	}

	statement();
	emitLoop(loopStart);
	
	if (exitJump != -1) {
		patchJump(exitJump);
		emitByte(op_pop);		// Likewise 'op_jmp_false' case, but must be poped off before leaving the 'for' statement.
	}

	endScope();
}

static void
statement(void)
{
	if (match(tkn_print)) {
		printStatement();
	} else if (match(tkn_for)) {
		forStatement();
	}else if (match(tkn_if)) {
		ifStatement();
	} else if (match(tkn_while)) {
		whileStatement();
	} else if (match(tkn_lbrace)) {
		beginScope();
		block();
		endScope();
	} else {
		expressionStatement();
	}
}

static void
synchronize(void)
{
	parser.panicMode = false;
	

	while (parser.current.type != tkn_eof) {
		
		// at the very first iteration, check that the previous token wasn't one,
		// which designates the end of expression.
		if (parser.previous.type == tkn_semicolon)
			return;
		
		switch (parser.current.type) {
			case tkn_class:		// These tokens mark the synchronization point,
			case tkn_fun:		// i.e. they represent a starting point of new a statements.
			case tkn_var:		// We want to skip all tokens within erroneous expression, and jump over 
			case tkn_for:		// to these ones so that the compiler proceeds to further statements and expression.
			case tkn_if:		// Doing this way the compiler will process and reveal not only current
			case tkn_while:		// error (which led us to this function) but all other ahead too (if any),
			case tkn_print:		// yielding all errors in one message.
			case tkn_ret:
				return;
			default: /* do nothing. */
		}
		advance();
	}
}

static void
declaration(void)
{
	if (match(tkn_var)) {
		varDeclaration();
	} else {
		statement();
	}

	if (parser.panicMode)
		synchronize();
}

static void
initCompiler(Compiler* compiler) {
	compiler->localCount = 0;
	compiler->scopeDepth = 0;
	currCplr = compiler;
}

bool
compile(const char* source, ByteCode* bCode)
{
	initScanner(source);
	Compiler compiler;
	initCompiler(&compiler);

	currCtx = bCode;
	parser.hadError = false;
	parser.panicMode = false;

	advance();
	// consume(tkn_eof, "Expect end of expression.");
	while (!match(tkn_eof)) {
		declaration();
	}

	commitCompilation();
	return !parser.hadError;
}
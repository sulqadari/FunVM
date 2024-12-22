#include "compiler.h"
#include "scanner.h"
#include "bytecode.h"
#include "object.h"

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

static Parser parser;
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

static bool
check(const TokenType type)
{
	return type == parser.current.type;
}

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
		emitBytes(((idx >> 8) & 0x00FF), (idx & 0x00FF));
	}
}

static void
commitCompilation(void)
{
	emitReturn();
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

ParseRule rules[] = {
	[tkn_lparen]   = {grouping, NULL, prec_none},
	[tkn_rparen]   = {NULL,  NULL, prec_none},
	[tkn_lbrace]   = {NULL,  NULL, prec_none},
	[tkn_rbrace]   = {NULL,  NULL, prec_none},
	[tkn_lbracket] = {NULL,  NULL, prec_none},
	[tkn_rbracket] = {NULL,  NULL, prec_none},
	[tkn_semicolon] = {NULL, NULL, prec_none},
	[tkn_comma]    = {NULL,  NULL, prec_none},
	[tkn_dot]      = {NULL,  NULL, prec_none},
	[tkn_minus]    = {unary, binary, prec_term},
	[tkn_plus]     = {NULL,  binary, prec_term},
	[tkn_slash]    = {NULL,  binary, prec_factor},
	[tkn_star]     = {NULL,  binary, prec_factor},
	
	[tkn_not]      = {unary, NULL, prec_none},
	[tkn_neq]      = {NULL,  binary, prec_equality},
	[tkn_eq]       = {NULL,  NULL, prec_none},
	[tkn_2eq]      = {NULL,  binary, prec_equality},
	[tkn_gt]       = {NULL,  binary, prec_comparison},
	[tkn_gteq]     = {NULL,  binary, prec_comparison},
	[tkn_lt]       = {NULL,  binary, prec_comparison},
	[tkn_lteq]     = {NULL,  binary, prec_comparison},
	[tkn_and]      = {NULL,  NULL, prec_none},
	[tkn_or]       = {NULL,  NULL, prec_none},
	
	[tkn_id]       = {variable, NULL, prec_none},
	[tkn_str]      = {string, NULL, prec_none},

	[tkn_var]      = {number, NULL, prec_none},
	[tkn_if]       = {NULL, NULL, prec_none},
	[tkn_else]     = {NULL, NULL, prec_none},
	[tkn_switch]   = {NULL, NULL, prec_none},
	[tkn_break]    = {NULL, NULL, prec_none},
	[tkn_while]    = {NULL, NULL, prec_none},
	[tkn_for]      = {NULL, NULL, prec_none},
	[tkn_continue] = {NULL, NULL, prec_none},
	[tkn_class]    = {NULL, NULL, prec_none},
	[tkn_super]    = {NULL, NULL, prec_none},
	[tkn_this]     = {NULL, NULL, prec_none},
	[tkn_fun]      = {NULL, NULL, prec_none},
	[tkn_null]     = {literal, NULL, prec_none},
	[tkn_ret]      = {NULL, NULL, prec_none},
	[tkn_false]    = {literal, NULL, prec_none},
	[tkn_true]     = {literal, NULL, prec_none},
	[tkn_err]      = {NULL, NULL, prec_none},
	[tkn_eof]      = {NULL, NULL, prec_none},
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
	uint16_t arg = identifierConstant(&name);
	OpCode opcode;

	if (canAssign && match(tkn_eq)) {
		expression();
		opcode = op_set_gvar;
	} else {
		opcode = op_get_gvar;
	}

	if (arg <= UINT8_MAX) {
		emitBytes(opcode, arg);
	} else {
		emitByte(opcode + 1);
		emitBytes(((arg >> 8) & 0x00FF), (arg & 0x00FF));
	}
}

static uint16_t
parseVariable(const char* errorMessage)
{
	consume(tkn_id, errorMessage);
	return identifierConstant(&parser.previous);
}

/**
 * Produces the bytecode instruction that defines the new variable
 * and stores its initial value.
 * @param uint16_t the index of the variable's name in the constant table.
 */
static void
defineVariable(uint16_t global)
{
	if (global <= UINT8_MAX) {
		emitBytes(op_def_gvar, global);
	} else {
		emitByte(op_def_gvarw);
		emitBytes(((global >> 8) & 0x00FF), (global & 0x00FF));
	}
}

static void
expression(void)
{
	parsePrecedence(prec_assignment);
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
			case tkn_fun:		// i.e. they represent a starting point of new statements.
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
printStatement(void)
{
	consume(tkn_lparen, "Expect '(' after 'print'.");
	expression();
	consume(tkn_rparen, "Expect ')' after 'print'.");
	consume(tkn_semicolon, "Expect ';' after pirnt().");
	emitByte(op_print);
}

static void
statement(void)
{
	if (match(tkn_print)) {
		printStatement();
	} else {
		expressionStatement();
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


bool
compile(const char* source, ByteCode* bCode)
{
	initScanner(source);
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
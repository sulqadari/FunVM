#include "funvmConfig.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bytecode.h"
#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "object.h"
#include "value.h"

#ifdef FUNVM_DEBUG
#include "debug.h"
#endif

typedef struct {
	Token current;	/* current token */
	Token previous;	/* previous token */
	bool hadError;	/* records a errors occured during compilation. */
	bool panicMode;	/* avoids error cascades. */
} Parser;

/**
 * The enumeration of all precedence levels in order from
 * lowest to highest.
 * Using this precedence order, consider the following example:
 * say, the compiler is sitting on a chunk of code like:
 * -a.b + c;
 * if we call parsePrecedence(PREC_ASSIGNMENT), then it will parse the
 * entire expression because '+' has highest precedence than assignment.
 * If instead we call parsePrecedence(PREC_UNARY), it will compile the '-a.b'
 * and stop there. It doesn't keep going through the '+' because the addition
 * has lower precedence than unary operators.
*/
typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT,	// =
	PREC_OR,			// or
	PREC_AND,			// and
	PREC_EQUALITY,		// == !=
	PREC_COMPARISON,	// < > <= >=
	PREC_TERM,			// + -
	PREC_FACTOR,		// * /
	PREC_UNARY,			// ! -
	PREC_CALL,			// . ()
  	PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

Parser parser;
Bytecode *currentCtx;

static Bytecode*
currentContext(void)
{
	return currentCtx;
}

/**
 * The top-level error handling function.
 * 
 * When an error occurs, all available information (line, lexeme and message)
 * will be printed out. Before function returns to the caller, it will
 * cock the 'panic mode' flag. This is done to prevent compiler from outputing
 * the rest messages about the errors which are the result of the first
 * one in the series of the errors. Consider the following:
 * if (& > idx) idx + 2;
 * Right after compiler processes '(& >' expression, the remaining is nothing but
 * meaningless blob of code.
 * Thus, when the compiler encounteres subsequent error it just returns without
 * annoying user with useless information.
 * This 'silent mode' will end after the compiler reaches a synchronization
 * point.
 * Consider the following code:
 * if (& > idx)
 * 		idx + 2;
 * else if (7 => idx)
 * 		idx + 3;
 * else
 * 		idx++;
 * 
 * Right after compiler throws the error message in 'if' statement it will
 * suppress any other error messages until after reaches the ';' token.
 * This ';' is the excact synchronization point.
 * After that it proceeds to 'else if' statement and will send the error
 * message about incorrent 'greater or equal' expression syntax, but silently
 * walk through until encounter ';' token, passing over other probable errors.
 * This way the compiler will process and print all errors in the single pass.
 */
static void
errorAt(Token *token, const char *message)
{
	/* Once we've encountered an error in the source code,
	 * there is no reason to print out further messages. */
	if (true == parser.panicMode)
		return;
	
	/* If we are here, then it's mean that the compiler have encountered
	 * the first error in the code. Print all relative information. */
	parser.panicMode = true;

	/* Print the line number of error occurence. */
	fprintf(stderr, "[line %d] Error", token->line);

	/* First, process the corner cases: either the current
	 * token is of type error, or designates end of file.
	 * Neither TOKEN_ERROR, nor TOKEN_EOF have human-readable lexeme. */
	switch (token->type) {
		case TOKEN_ERROR:
			// Nothing to do.
		break;
		case TOKEN_EOF:
			// Error occured at the very end of a file.
			fprintf(stderr, " at end");
		break;
		default:
			// Print out lexeme (in case it's human-readable).
			fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
}

/**
 * Passes the previous token followed by error message
 * to the errorAt() function.
 * Compared to errorAtCurrent() function, this one is used more
 * frequently, when either consume() or advance() functions
 * have returned from the call.
 */
static void
error(const char *message)
{
	errorAt(&parser.previous, message);
}

/**
 * Passes the current token followed by error message
 * to the errorAt() function.
 * This function is primarily used in a functions that
 * in the middle of token processing, e.g. advance() and consume().
 */
static void
errorAtCurrent(const char *message)
{
	errorAt(&parser.current, message);
}

/**
 * Steps forward through the token stream.
 * Calling scanToken() it asks the scanner for the
 * next token and stores it for later use.
 * The 'parser.previous' field is initialized with the
 * current token so that we can track the beginning
 * of the lexeme's name.
 *
 * NOTE: the scanToken() doesn't report lexical errors, instead
 * it creates special error token and leaves it up to the scanner
 * to report them.
 */
static void
advance(void)
{
	/* Stash the old 'current' token. */
	parser.previous = parser.current;

	while (true) {
		parser.current = scanToken();
		if (parser.current.type != TOKEN_ERROR)
			break;
		/* The beginning of the lexeme of error token is
		 * passed to it*/
		errorAtCurrent(parser.current.start);
	}
}

/**
 * Reads and validates subsequent token.
 */
static void
consume(TokenType type, const char *message)
{
	/* Ensure that the current token's type matchs
	 * expected value. If this is the case,
	 * then consume it return. */
	if (type == parser.current.type) {
		advance();
		return;
	}
	/* Throw error otherwise. */
	errorAtCurrent(message);
}

static bool
check(const TokenType type)
{
	return type == parser.current.type;
}

/**
 * Consumes the current token if it has the given type.
 * @returns bool: true if match, false otherwise.
*/
static bool
match(TokenType type)
{
	if (!check(type))
		return false;

	advance();
	return true;
}

/**
 * Assigns a given byte to the bytecode chunk which
 * can be an opcode or an operand to an instruction.
 * This function also writes the previous token's line number
 * so that runtime errors are associated with that line.
 *
 * QUESTION: why do we pass in the previous token, not current?
 * ANSWER: because a bytecode to be written is the result of
 * token being consumed, whilst the 'current' token is still
 * waiting its turn to be processed. 
 */
static void
emitByte(uint32_t byte)
{
	Bytecode* ctx = currentContext();
	writeBytecode(ctx, byte, parser.previous.line);
}

/**
 * Writes an opcode followed by a one-byte operand.
 */
static void
emitBytes(uint32_t byte1, uint32_t byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

static void
emitReturn(void)
{
	emitByte(OP_RETURN);
}

/**
 * Push the given value into Constant Pool.
 * @returns uint32_t - an offset within Constant pool where the value is stored.
 */
static uint32_t
makeConstant(Value value)
{
	uint32_t offset = addConstant(currentContext(), value);
	return offset;
}

static void
emitConstant(Value value)
{
	uint32_t offset = makeConstant(value);
	uint8_t opcode = OP_CONSTANT;

	/* If the total number of constants in constant pool
	 * exceeds 255 elements, then instead of OP_CONSTANT which
	 * occupes two bytes [OP_CONSTANT, operand], the OP_CONSTANT_LONG
	 * comes into play, which spawns over four bytes:
	 * [OP_CONSTANT_LONG, operand1, operand2, operand3]*/
	if (offset > UINT8_MAX)
		opcode = OP_CONSTANT_LONG;

	emitBytes(opcode, offset);
}

static void
endCompiler(void)
{
	emitReturn();
#ifdef FUNVM_DEBUG
	if (parser.hadError) {
		disassembleBytecode(currentContext(), "code");
	}
#endif // !FUNVM_DEBUG
}

static void expression(void);

/* Blocks can contain declaration, and control flow statements
 * can contain other statements, which means these two functions
 * will eventually be recursive. */
static void statement(void);
static void declaration(void);

static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);
static uint8_t identifierConstant(Token *name);

/**
 * Infix expressions parser.
 * This function compiles the right operands and emits the bytecode instruction
 * for the operator.
 * The fact that the left operang e.g. '= 1 +...' or '= (1 + 2 * 3) +...' gets
 * compiled first, means that at runtime that code gets executed first.
 * When it runs, the value it produces will end up on the stack, which the
 * right where the infix operator is going to need it.
 * Then this function handles the rest of the arithmetic operators, e.g.
 * '...2;' or '...(4 / 5);'.
*/
static void
binary(bool canAssign)
{
	/* The leading operand's token has already been comsumed, as well as
	 * infix operator. Thus, take it back so that we can get to know
	 * what king of arithmetic operation we're dealing with. */
	TokenType operatorType = parser.previous.type;
	
	/* Consider the expression: '2 * 3 + 4'.
	 * when we parse the right operand of the '*' expression, we need to
	 * just capture '3', and not '3 + 4', because '+' is lower precedence
	 * than '*'.
	 * Each binary operator's right-hand operand precedence is one level
	 * higher than its own. Using getRule() we lookup operator's precedence
	 * and call parsePrecedence() with one level higher than this
	 * operator's level. This hack allows us to use the single binary()
	 * function instead of declaring a separate function set for each king
	 * of operators.
	 * One higher level of precedence is used because the binary operators
	 * are lef-associative. Given a series of the same operators, like:
	 * 1 + 2 + 3 + 4,
	 * we want to parse it like:
	 * (((1 + 2) + 3) + 4),
	 * Thus, when parsing the right-hand operand to the first '+', we want
	 * to consume the '2', but not the rest, so we use one level
	 * above addition's precedence.
	 * Calling parsePrecedence() with the same precedence as the current
	 * operator is essential for right-associative operators like assignment:
	 * a = b = c = d,
	 * must be parsed as:
	 * (a = (b = (c = d))),
	 * */
	ParseRule *rule = getRule(operatorType);
	parsePrecedence((Precedence)(rule->precedence + 1));

	switch (operatorType) {
		case TOKEN_BANG_EQUAL:		emitBytes(OP_EQUAL, OP_NOT); break;
		case TOKEN_EQUAL_EQUAL:		emitByte(OP_EQUAL); break;
		case TOKEN_GREATER:			emitByte(OP_GREATER); break;
		case TOKEN_GREATER_EQUAL:	emitBytes(OP_LESS, OP_NOT); break;
		case TOKEN_LESS:			emitByte(OP_LESS); break;
		case TOKEN_LESS_EQUAL:		emitBytes(OP_GREATER, OP_NOT); break;

		case TOKEN_PLUS:	emitByte(OP_ADD);		break;
		case TOKEN_MINUS:	emitByte(OP_SUBTRACT);	break;
		case TOKEN_STAR:	emitByte(OP_MULTIPLY);	break;
		case TOKEN_SLASH:	emitByte(OP_DIVIDE);	break;

		default: return; // Unreachable
	}
}

static void
literal(bool canAssign)
{
	switch (parser.previous.type) {
		case TOKEN_FALSE:	emitByte(OP_FALSE);	break;
		case TOKEN_NIL:		emitByte(OP_NIL);	break;
		case TOKEN_TRUE:	emitByte(OP_TRUE);	break;
		default: return;	// Unreachable
	}
}
/**
 * Parentheses expression handler.
 *
 * Assumes the initial opening parenthesis has already been consumed.
 * Recursively calls back into expression() to compile
 * the expression between the parenthesis, then parse the closing one
 * at the end.
 *
 * As far as the back end is concerned, there's literally nothing to a
 * grouping expression. Its sole function is syntactic - it lets insert
 * a lower-precedence expression where a higher precedence is expected.
 * Thus, it has no runtime semantics on its own and therefore doesn't emit
 * any bytecode.
 */
static void
grouping(bool canAssign)
{
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/**
 * Compiles number literal. Assumes the token for the
 * number literal has already been consumed and is stored
 * in 'parser.previous' field.
 */
static void
number(bool canAssign)
{
	double value = strtod(parser.previous.start, NULL);
	emitConstant(NUMBER_PACK(value));
}

/**
 * Takes the string's characters directly from the lexeme,
 * trims surrounding double quotes and then creates a string object,
 * wraps it in a 'Value' and stores in into the constant pool.
 * 
 * Also makes VM track the new object in the VM's linked list so that,
 * it can be tracked and deleted before interpreter terminates.
*/
static void
string(bool canAssign)
{
	ObjString *str = copyString(parser.previous.start + 1,
								parser.previous.length - 2);
	
	/* Trim the leading double quote and exclude
	 * from the length both leading and trailing ones. */
	emitConstant(OBJECT_PACK(str));
}

/**
 * Passes the given identifier token and adds its lexeme to the bytecode's
 * constant table as a string.
 */
static void
namedVariable(Token name, bool canAssign)
{
	uint8_t arg = identifierConstant(&name);

	if (canAssign && match(TOKEN_EQUAL)) {	// If we find an equal sign, then..
		expression();				// ..evaluate the expression and..
		emitBytes(OP_SET_GLOBAL, arg);	// ..set the value
	} else {						// Otherwise
		emitBytes(OP_GET_GLOBAL, arg);	// get the variable's value.
	}
}

static void
variable(bool canAssign)
{
	namedVariable(parser.previous, canAssign);
}

/**
 * Processes 'negation' and 'not' operations.
 * This function takes into account the expressions with the certain
 * precedence, so that, the '-a.b + c' expression will be parsed as follows:
 * 1. The '.' operator calls 'b' field of the 'a' structure;
 * 2. negation is performed on 'b' field;
 * 3. the rest of the expression will be parsed in the appropriate
 * part of the compiler, not being chewed through in this function, 
 * including '+ c'.
*/
static void
unary(bool canAssign)
{
	/* Grab the token type to note which unary operator we're
	 * dealing with. Note that it has already been consumed,
	 * that's why we are addressing previous token, not the current one. */
	TokenType operatorType = parser.previous.type;
	
	/* Compile the operand. Note that we write the target expression
	 * (either negation or not) *before* operand's bytecode. This is
	 * because of order of execution:
	 * 1. We evaluate the operand first which leaves its value on the stack.
	 * 2. Then we pop that value (negate or NOT'ing it) and push the result.
	 * 
	 * NOTE: passing PREC_UNARY means, that this call to parsePrecedence()
	 * will consume the only part of the expression, whom precedence is equal
	 * or greater than PREC_UNARY.
	 * */
	parsePrecedence(PREC_UNARY);

	/* Emit the operator instruction */
	switch (operatorType) {
		case TOKEN_BANG:	emitByte(OP_NOT);		break;
		case TOKEN_MINUS:	emitByte(OP_NEGATE);	break;
		default: return; // Unreachable
	}
}

/* The table of expressions parsing functions.
 * Each type of expression is processed by a specified function
 * that outputs the appropriate bytecode. */
ParseRule rules[] = {
	[TOKEN_LEFT_PAREN]		= {grouping, NULL, PREC_NONE},
	[TOKEN_RIGHT_PAREN]		= {NULL,     NULL,   PREC_NONE},
	[TOKEN_LEFT_BRACE]		= {NULL,     NULL,   PREC_NONE}, 
	[TOKEN_RIGHT_BRACE]		= {NULL,     NULL,   PREC_NONE},
	[TOKEN_COMMA]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_DOT]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_MINUS]			= {unary,    binary, PREC_TERM},
	[TOKEN_PLUS]			= {NULL,     binary, PREC_TERM},
	[TOKEN_SEMICOLON]		= {NULL,     NULL,   PREC_NONE},
	[TOKEN_SLASH]			= {NULL,     binary, PREC_FACTOR},
	[TOKEN_STAR]			= {NULL,     binary, PREC_FACTOR},
	[TOKEN_BANG]			= {unary,     NULL,   PREC_NONE},
	[TOKEN_BANG_EQUAL]		= {NULL,     binary,   PREC_EQUALITY},
	[TOKEN_EQUAL]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_EQUAL_EQUAL]		= {NULL,     binary,   PREC_EQUALITY},
	[TOKEN_GREATER]			= {NULL,     binary,   PREC_COMPARISON},
	[TOKEN_GREATER_EQUAL]	= {NULL,     binary,   PREC_COMPARISON},
	[TOKEN_LESS]			= {NULL,     binary,   PREC_COMPARISON},
	[TOKEN_LESS_EQUAL]		= {NULL,     binary,   PREC_COMPARISON},
	[TOKEN_IDENTIFIER]		= {variable,     NULL,   PREC_NONE},
	[TOKEN_STRING]			= {string,     NULL,   PREC_NONE},
	[TOKEN_NUMBER]			= {number,   NULL,   PREC_NONE},
	[TOKEN_AND]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_CLASS]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_ELSE]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_FALSE]			= {literal,     NULL,   PREC_NONE},
	[TOKEN_FOR]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_FUN]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_IF]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_NIL]				= {literal,     NULL,   PREC_NONE},
	[TOKEN_OR]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_PRINT]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_RETURN]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_SUPER]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_THIS]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_TRUE]			= {literal,     NULL,   PREC_NONE},
	[TOKEN_VAR]				= {NULL,     NULL,   PREC_NONE},
	[TOKEN_WHILE]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_ERROR]			= {NULL,     NULL,   PREC_NONE},
	[TOKEN_EOF]				= {NULL,     NULL,   PREC_NONE},
};

/**
 * The main function which orchestrates all of the parsing functions.
 * It starts at the current token and parses any expression at the given
 * precedence level or higher.
 */
static void
parsePrecedence(Precedence precedence)
{
	advance();

	/* Parsing prefix expressions. The first token is allways going to belong
	 * to some kind of prefix expressions, by definition. */
	ParseFn prefixRule = getRule(parser.previous.type)->prefix;
	if (NULL == prefixRule) {
		error("Expect expression.");
		return;
	}

	bool canAssign = precedence <= PREC_ASSIGNMENT;
	prefixRule(canAssign);

	/* Parsing infix expressions. The prefix expression we already compiled
	 * might be an operand for it if and only if the 'precedence' is low enough
	 * to permit that infix operator. */
	while (precedence <= getRule(parser.current.type)->precedence) {
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule(canAssign);
	}

	if (canAssign && match(TOKEN_EQUAL)) {
		error("Invalid assignment target.");
	}
}

/**
 * Takes the given token and adds its lexeme to the bytecode's constant pool
 * as a string.
 * @returns uint8_t: an index of the constant in the constant pool.
 */
static uint8_t
identifierConstant(Token *name)
{
	ObjString *str = copyString(name->start, name->length);
	return makeConstant(OBJECT_PACK(str));
}

static uint8_t
parseVariable(const char *errorMessage)
{
	consume(TOKEN_IDENTIFIER, errorMessage);
	return identifierConstant(&parser.previous);
}

/**
 * Produces the bytecode instruction that defines the new
 * global variable's index in its operand.
 */
static void
defineVariable(uint8_t global)
{
	emitBytes(OP_DEFINE_GLOBAL, global);
}

static ParseRule*
getRule(TokenType type)
{
	return &rules[type];
}

/**
 * Simply parses the lowest precedence level, which subsumes all of the
 * higher-precedence expressions too.
*/
static void
expression(void)
{
	parsePrecedence(PREC_ASSIGNMENT);
}

static void
varDeclaration(void)
{
	// Manage variable name (which follows right after the 'var' keyword)
	uint8_t global = parseVariable("Expect variable name.");

	if (match(TOKEN_EQUAL)) {	// is variable initialized?
		expression();				// Handle an initialization expression
	} else { 						// No?.
		emitByte(OP_NIL);		// Initialize with NIL value
	}

	consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

	defineVariable(global);
}

/**
 * An expression followed by a semicolon.
*/
static void
expressionStatement(void)
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_POP);
}

static void
printStatement(void)
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_PRINT);
}

/**
 * Panic mode error recovery function aimed to minimize the
 * number of cascaded compile errors that it reports.
 * The compiler exits the panic mode when it reaches a synchronization
 * point, which is statement boundaries.
*/
static void
synchronize(void)
{
	parser.panicMode = false;	/* Reset panic mode. */

	/* Skip tokens until reach domething that looks like a
	 * statement boundary. */
	while (TOKEN_EOF != parser.current.type) {
		if (TOKEN_SEMICOLON == parser.previous.type)
			return;
		
		switch (parser.current.type) {
			case TOKEN_CLASS:
			case TOKEN_FUN:
			case TOKEN_VAR:
			case TOKEN_FOR:
			case TOKEN_IF:
			case TOKEN_WHILE:
			case TOKEN_PRINT:
			case TOKEN_RETURN:
				return;
			
			default:/* Do nothing. */;
		}

		advance();
	}
}

static void
declaration(void)
{
	if (match(TOKEN_VAR)) {
		varDeclaration();
	} else {
		statement();
	}

	/* The synchronization point. */
	if (parser.panicMode)
		synchronize();
}

static void
statement(void)
{
	if (match(TOKEN_PRINT)) {
		printStatement();
	} else {
		expressionStatement();
	}
}

/**
 * Produces the bytecode from the source file and
 * stores it in 'Bytecode bytecode' output parameter.
 * @param
 * @param
 * @returns bool: TRUE if an error occured. FALSE otherwise.
 */
bool
compile(const char *source, Bytecode *bytecode)
{
	initScanner(source);
	currentCtx = bytecode;
	parser.hadError = false;
	parser.panicMode = false;

	/* Start up the scanner. */
	advance();

	/* Keep compiling declarations until hit the end
	 * of the source file. */
	while (!match(TOKEN_EOF)) {
		declaration();
	}

	endCompiler();

	return parser.hadError;
}
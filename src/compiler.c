#include "funvmConfig.h"

#include <stdio.h>
#include <stdlib.h>

#include "bytecode.h"
#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "object.h"
#include "constant_pool.h"

#ifdef FUNVM_DEBUG
#include "debug.h"
#endif

typedef struct {
	Token current;
	Token previous;
	bool hadError;
	bool panicMode;
} Parser;

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

typedef void (*ParseFn)();

typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

Parser parser;
Bytecode *compilingBytecode;

static Bytecode*
currentContext(void)
{
	return compilingBytecode;
}

static void
errorAt(Token *token, const char *message)
{
	if (true == parser.panicMode)
		return;
	
	parser.panicMode = true;

	fprintf(stderr, "[line %d] Error", token->line);

	if (TOKEN_EOF == token->type) {
		fprintf(stderr, " at end");
	} else if (TOKEN_ERROR == token->type) {
		// Nothing to do.
	} else {	// Print out lexeme (in case it's human-readable).
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
}

static void
error(const char *message)
{
	errorAt(&parser.previous, message);
}

static void
errorAtCurrent(const char *message)
{
	errorAt(&parser.current, message);
}

/**
 * Steps forward through the token stream. */
static void
advance(void)
{
	/* Stash the old 'current' token. */
	parser.previous = parser.current;

	while (true) {
		parser.current = scanToken();
		if (parser.current.type != TOKEN_ERROR)
			break;
		
		errorAtCurrent(parser.current.start);
	}
}

/**
 * Reads and validates subsequent token.
 */
static void
consume(TokenType type, const char *message)
{
	if (type == parser.current.type) {
		advance();
		return;
	}

	errorAtCurrent(message);
}

/**
 * Assigns a given value to the bytecode chunk.
 */
static void
emitByte(uint32_t byte)
{
	writeBytecode(currentContext(), byte, parser.previous.line);
}

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
	 * occupes two bytes [OP_CONSTANT, offset], the OP_CONSTANT_LONG
	 * comes into play, which spawns over four bytes:
	 * [OP_CONSTANT_LONG, offset1, offset2, offset3]*/
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
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void
binary(void)
{
	TokenType operatorType = parser.previous.type;
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
literal(void)
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
grouping(void)
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
number(void)
{
	double value = strtod(parser.previous.start, NULL);
	emitConstant(NUMBER_PACK(value));
}

static void
string(void)
{
	/* Trim the leading double quote and exclude
	 * from the length both leading and trailing ones. */
	emitConstant(OBJECT_PACK(copyString(parser.previous.start + 1,
								parser.previous.length - 2)));
}

static void
unary(void)
{
	TokenType operatorType = parser.previous.type;
	
	// Compile the operand
	parsePrecedence(PREC_UNARY);

	// Emit the operator instruction
	switch (operatorType) {
		case TOKEN_BANG:	emitByte(OP_NOT);		break;
		case TOKEN_MINUS:	emitByte(OP_NEGATE);	break;
		default: return; // Unreachable
	}
}

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
	[TOKEN_IDENTIFIER]		= {NULL,     NULL,   PREC_NONE},
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

	prefixRule();

	/* Parsing infix expressions. The prefix expression we already compiled
	 * might be an operand for it if and only if the 'precedence' is low enough
	 * to permit that infix operator. */
	while (precedence <= getRule(parser.current.type)->precedence) {
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule();
	}
}

static ParseRule*
getRule(TokenType type)
{
	return &rules[type];
}

static void
expression(void)
{
	parsePrecedence(PREC_ASSIGNMENT);
}

bool
compile(const char *source, Bytecode *bytecode)
{
	initScanner(source);
	compilingBytecode = bytecode;
	parser.hadError = false;
	parser.panicMode = false;

	/* Start up the scanner. */
	advance();
	/* Parse the single expression. */
	expression();
	/* Check for the sentinel OEF token. */
	consume(TOKEN_EOF, "Expect end of expression.");
	endCompiler();

	return !parser.hadError;
}
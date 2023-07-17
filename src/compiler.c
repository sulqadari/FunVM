#include <stdio.h>
#include <stdlib.h>
#include <sys/endian.h>
#include <x86/_stdint.h>

#include "bytecode.h"
#include "common.h"
#include "compiler.h"
#include "scanner.h"

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

Parser parser;
Bytecode *compilingBytecode;

static Bytecode*
currentBytecode(void)
{
	return compilingBytecode;
}

//	Compiler's Front End	//
//							//

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
 * Reads and validates subsequent token. */
static void
consume(TokenType type, const char *message)
{
	if (type == parser.current.type) {
		advance();
		return;
	}

	errorAtCurrent(message);
}
//							//
// 	!Compiler's Front End	//



//	Compiler's Back End		//
//							//

/**
 * Assigns a given value to the bytecode chunk.
 */
static void
emitByte(uint32_t byte)
{
	writeBytecode(currentBytecode(), byte, parser.previous.line);
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
 * @returns uint32_t - an offset within Constant pool where the value is stored. */
static uint32_t
makeConstant(Value value)
{
	uint32_t offset = addConstant(currentBytecode(), value);
	return offset;
}

static void
emitConstant(Value value)
{
	uint32_t offset = makeConstant(value);
	uint8_t opcode = OP_CONSTANT;

	if (offset > UINT8_MAX)
		opcode++;	/* Write OP_CONSTANT_LONG */
	
	emitBytes(opcode, offset);
}

static void
parsePrecedence(Precedence precedence)
{

}

static void
expression(void)
{
	parsePrecedence(PREC_ASSIGNMENT);
}

static void
endCompiler(void)
{
	emitReturn();
}

static void
binary(void)
{
	TokenType operatorType = parser.previous.type;
	ParseRule *rule = getRule(operatorType);
	parsePrecedence((Precedence)(rule->precedence + 1));

	switch (operatorType) {
		case TOKEN_PLUS:	emitByte(OP_ADD);		break;
		case TOKEN_MINUS:	emitByte(OP_SUBTRACT);break;
		case TOKEN_STAR:	emitByte(OP_MULTIPLY);break;
		case TOKEN_SLASH:	emitByte(OP_DIVIDE);	break;
		default: return; // Unreachable
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
	Value value = strtod(parser.previous.start, NULL);
	emitConstant(value);
}

static void
unary(void)
{
	TokenType operatorType = parser.previous.type;
	
	// Compile the operand
	parsePrecedence(PREC_UNARY);

	// Emit the operator instruction
	switch (operatorType) {
		case TOKEN_MINUS: emitByte(OP_NEGATE); break;
		default: return; // Unreachable
	}
}

//	!Compiler's Back End	//

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
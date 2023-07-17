#include <stdio.h>
#include <stdlib.h>
#include <sys/endian.h>
#include <x86/_stdint.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
	Token current;
	Token previous;
	bool hadError;
	bool panicMode;
} Parser;

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
endCompiler(void)
{
	emitReturn();
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
expression(void)
{

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
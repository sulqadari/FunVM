#ifndef FUNVM_BYTECODE_H
#define FUNVM_BYTECODE_H

#include "value.h"

/* Opcode - a one-byte operation code which
 * designates a bytecode instruction we're dealing with -
 * add, subtract, look up variable, etc. */
typedef enum {
	OP_CONSTANT,		/* Load the constant from Value pool (1 byte  offset). */
	OP_CONSTANT_LONG,	/* Load the constant from Value pool (3 bytes offset). */
	OP_NIL,				/* Push NIL value on the stack (dedicated ins). */
	OP_TRUE,			/* Push TRUE value on the stack (dedicated ins). */
	OP_FALSE,			/* Push FALSE value on the stack (dedicated ins). */
	OP_POP,				/* Pops the top value off the stack and forgets it. */
	OP_GET_GLOBAL,
	OP_DEFINE_GLOBAL,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NOT,				/* Logical inversion (true<->false; zero<->non-zero). */
	OP_NEGATE,			/* Negate operand value (254<->(-254), etc). */
	OP_PRINT,
	OP_RETURN			/* Return from the current function. */
} Opcode;

/* Bytecode - a dynamic array of instructions (opcodes). */
typedef struct {
	uint32_t count;		/* How many elements in code arrya is. */
	uint32_t capacity;	/* Size of the code array. */
	uint8_t *code;		/* A chunk of instructions. */
	uint32_t *lines;
	ConstantPool const_pool;
} Bytecode;

void initBytecode(Bytecode *bytecode);
void freeBytecode(Bytecode *bytecode);
//void writeBytecode(Bytecode *bytecode, uint8_t opcode, uint32_t line);
void writeBytecode(Bytecode *bytecode, uint32_t opcode, uint32_t line);
uint32_t addConstant(Bytecode *bytecode, Value constant);

#endif // !FUNVM_BYTECODE_H
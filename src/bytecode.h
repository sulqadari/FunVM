#ifndef FUNVM_BYTECODE_H
#define FUNVM_BYTECODE_H

#include "common.h"
#include "constant_pool.h"

/* Opcode - a one-byte operation code which
 * designates a bytecode instruction we're dealing with -
 * add, subtract, look up variable, etc. */
typedef enum {
	OP_CONSTANT,		/* Load the constant from Constant pool. */
	OP_RETURN,			/* Return from the current function. */
} Opcode;

/* Bytecode - a dynamic array of instructions (opcodes). */
typedef struct {
	int32_t count;		/* How many elements in code arrya is. */
	int32_t capacity;	/* Size of the code array. */
	uint8_t *code;		/* A chunk of instructions. */
	int32_t *lines;
	ConstantPool const_pool;
} Bytecode;

void initBytecode(Bytecode *bytecode);
void freeBytecode(Bytecode *bytecode);
void writeBytecode(Bytecode *bytecode, uint8_t opcode, int32_t line);
int32_t addConstant(Bytecode *bytecode, Constant constant);

#endif // !FUNVM_BYTECODE_H
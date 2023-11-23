#ifndef FUNVM_BYTECODE_H
#define FUNVM_BYTECODE_H

#include "common.h"
#include "value.h"

/* Opcode - a one-byte operation code which
 * designates a bytecode instruction we're dealing with -
 * add, subtract, look up variable, etc. */
typedef enum {
	OP_CONSTANT,		/* Load the constant from Value pool (1 byte  offset). */
	OP_NIL,				/* Push NIL value on the stack (dedicated ins). */
	OP_TRUE,			/* Push TRUE value on the stack (dedicated ins). */
	OP_FALSE,			/* Push FALSE value on the stack (dedicated ins). */
	OP_POP,				/* Pops the top value off the stack and forgets it. */
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_DEFINE_GLOBAL,
	OP_SET_GLOBAL,
	OP_GET_GLOBAL,
	OP_GET_UPVALUE,
	OP_SET_UPVALUE,
	OP_GET_PROPERTY,
	OP_SET_PROPERTY,
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
	OP_PRINTLN,
	OP_JUMP,
	OP_JUMP_IF_FALSE,
	OP_LOOP,
	OP_CALL,
	OP_INVOKE,
	OP_CLOSURE,			/* take the function at the given Constant table's index. */
	OP_CLOSE_UPVALUE,
	OP_CLASS,
	OP_METHOD,
	OP_RETURN			/* Return from the current function. */
} Opcode;

/* Bytecode - a dynamic array of instructions (opcodes). */
typedef struct {
	FN_UWORD count;		/* Number of elements in code array. */
	FN_UWORD capacity;	/* Size of the code array. */
	FN_UBYTE* code;		/* A chunk of instructions. */
	FN_UWORD* lines;
	ConstantPool constPool;
} Bytecode;

void initBytecode(Bytecode* bytecode);
void freeBytecode(Bytecode* bytecode);
void writeBytecode(Bytecode* bytecode, FN_UBYTE opcode, FN_UWORD line);
FN_UWORD addConstant(Bytecode* bytecode, Value constant);

#endif // !FUNVM_BYTECODE_H
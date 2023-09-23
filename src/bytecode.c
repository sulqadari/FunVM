#include <stdint.h>
#include <stdlib.h>
#include "bytecode.h"
#include "memory.h"

void
initBytecode(Bytecode* bytecode)
{
	bytecode->count = 0;
	bytecode->capacity = 0;
	bytecode->code = NULL;
	bytecode->lines = NULL;
	initConstantPool(&bytecode->constPool);
}

void
writeBytecode(Bytecode* bytecode, FN_ubyte opcode, FN_uint line)
{
	/* Double bytecode array capacity if it doesn't have enough room. */
	if (bytecode->capacity < bytecode->count + 1) {
		FN_uint oldCapacity = bytecode->capacity;
		bytecode->capacity	= INCREASE_CAPACITY(oldCapacity);
		bytecode->code		= INCREASE_ARRAY(FN_ubyte, bytecode->code,
								oldCapacity, bytecode->capacity);
		bytecode->lines		= INCREASE_ARRAY(FN_uint, bytecode->lines,
								oldCapacity, bytecode->capacity);
	}

	bytecode->code[bytecode->count] = opcode;
	bytecode->lines[bytecode->count] = line;
	bytecode->count++;
}

void
freeBytecode(Bytecode* bytecode)
{
	FREE_ARRAY(FN_ubyte, bytecode->code, bytecode->capacity);
	FREE_ARRAY(FN_uint, bytecode->lines, bytecode->capacity);
	freeConstantPool(&bytecode->constPool);
	initBytecode(bytecode);
}

/**
 * Inserts the given Value into the constant pool of the bytecode chunk.
 * @returns FN_uint - index where the constant was appended.
*/
FN_uint
addConstant(Bytecode* bytecode, Value constant)
{
	writeConstantPool(&bytecode->constPool, constant);
	return (bytecode->constPool.count - 1);
}
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
	initConstantPool(&bytecode->const_pool);
}

void
writeBytecode(Bytecode* bytecode, uint8_t opcode, uint32_t line)
{
	/* Double bytecode array capacity if it doesn't have enough room. */
	if (bytecode->capacity < bytecode->count + 1) {
		uint32_t oldCapacity = bytecode->capacity;
		bytecode->capacity	= INCREASE_CAPACITY(oldCapacity);
		bytecode->code		= INCREASE_ARRAY(uint8_t, bytecode->code,
								oldCapacity, bytecode->capacity);
		bytecode->lines		= INCREASE_ARRAY(uint32_t, bytecode->lines,
								oldCapacity, bytecode->capacity);
	}

	bytecode->code[bytecode->count] = opcode;
	bytecode->lines[bytecode->count] = line;
	bytecode->count++;
}

void
freeBytecode(Bytecode* bytecode)
{
	FREE_ARRAY(uint8_t, bytecode->code, bytecode->capacity);
	FREE_ARRAY(uint32_t, bytecode->lines, bytecode->capacity);
	freeConstantPool(&bytecode->const_pool);
	initBytecode(bytecode);
}

/**
 * Inserts the given Value into the constant pool of the bytecode chunk.
 * @returns uint32_t - index where the constant was appended.
*/
uint32_t
addConstant(Bytecode* bytecode, Value constant)
{
	writeConstantPool(&bytecode->const_pool, constant);
	return (bytecode->const_pool.count - 1);
}
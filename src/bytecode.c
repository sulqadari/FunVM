#include <stdint.h>
#include <stdlib.h>
#include "bytecode.h"
#include "memory.h"

void
initBytecode(Bytecode *bytecode)
{
	bytecode->count = 0;
	bytecode->capacity = 0;
	bytecode->code = NULL;
	bytecode->lines = NULL;
	initConstantPool(&bytecode->const_pool);
}

void
writeBytecodeLong(Bytecode *bytecode, uint32_t opcode, uint32_t line)
{
	/* Double bytecode array capacity if it doesn't have enough room. */
	if (bytecode->capacity < bytecode->count + 3) {
		uint32_t oldCapacity = bytecode->capacity;
		bytecode->capacity = INCREASE_CAPACITY(oldCapacity);
		bytecode->code = INCREASE_ARRAY(uint8_t, bytecode->code,
										oldCapacity, bytecode->capacity);
		bytecode->lines = INCREASE_ARRAY(uint32_t, bytecode->lines,
										oldCapacity, bytecode->capacity);
	}

	uint32_t shift = 16;
	for (int i = 0; i < 3; ++i) {
		bytecode->code[bytecode->count] = (opcode >> shift);
		shift -= 8;
		bytecode->count++;
	}
	bytecode->lines[bytecode->count] = line;
}

void
writeBytecode(Bytecode *bytecode, uint8_t opcode, uint32_t line)
{
	/* Double bytecode array capacity if it doesn't have enough room. */
	if (bytecode->capacity < bytecode->count + 1) {
		uint32_t oldCapacity = bytecode->capacity;
		bytecode->capacity = INCREASE_CAPACITY(oldCapacity);
		bytecode->code = INCREASE_ARRAY(uint8_t, bytecode->code,
										oldCapacity, bytecode->capacity);
		bytecode->lines = INCREASE_ARRAY(uint32_t, bytecode->lines,
										oldCapacity, bytecode->capacity);
	}

	bytecode->code[bytecode->count] = opcode;
	bytecode->lines[bytecode->count] = line;
	bytecode->count++;
}

void
freeBytecode(Bytecode *bytecode)
{
	FREE_ARRAY(uint8_t, bytecode->code, bytecode->capacity);
	FREE_ARRAY(uint32_t, bytecode->lines, bytecode->capacity);
	freeConstantPool(&bytecode->const_pool);
	initBytecode(bytecode);
}

/**
 * @returns uint32_t - index where the constant was appended.
*/
uint32_t
addConstant(Bytecode *bytecode, Constant constant)
{
	writeConstantPool(&bytecode->const_pool, constant);
	return (bytecode->const_pool.count - 1);
}
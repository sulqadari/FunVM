#include <stdlib.h>
#include "bytecode.h"
#include "memory.h"

void
initBytecode(Bytecode *bytecode)
{
	bytecode->count = 0;
	bytecode->capacity = 0;
	bytecode->code = NULL;
}

void
writeBytecode(Bytecode *bytecode, uint8_t opcode)
{
	/* Double bytecode array capacity if it doesn't have enough room. */
	if (bytecode->capacity < bytecode->count + 1) {
		int32_t oldCapacity = bytecode->capacity;
		bytecode->capacity = INCREASE_CAPACITY(oldCapacity);
		bytecode->code = INCREASE_ARRAY(uint8_t, bytecode->code,
										oldCapacity, bytecode->capacity);
	}

	bytecode->code[bytecode->count] = opcode;
	bytecode->count++;
}

void
freeBytecode(Bytecode *bytecode)
{
	FREE_ARRAY(uint8_t, bytecode->code, bytecode->capacity);
	initBytecode(bytecode);
}
#include <stdint.h>
#include <stdlib.h>
#include "bytecode.h"
#include "memory.h"
#include "vm.h"

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
writeBytecode(Bytecode* bytecode, FN_UBYTE opcode, FN_UWORD line)
{
	/* Double bytecode array capacity if it doesn't have enough room. */
	if (bytecode->capacity < bytecode->count + 1) {
		FN_UWORD oldCapacity = bytecode->capacity;
		bytecode->capacity	= INCREASE_CAPACITY(oldCapacity);
		bytecode->code		= INCREASE_ARRAY(FN_UBYTE, bytecode->code,
								oldCapacity, bytecode->capacity);
		bytecode->lines		= INCREASE_ARRAY(FN_UWORD, bytecode->lines,
								oldCapacity, bytecode->capacity);
	}

	bytecode->code[bytecode->count] = opcode;
	bytecode->lines[bytecode->count] = line;
	bytecode->count++;
}

void
freeBytecode(Bytecode* bytecode)
{
	FREE_ARRAY(FN_UBYTE, bytecode->code, bytecode->capacity);
	FREE_ARRAY(FN_UWORD, bytecode->lines, bytecode->capacity);
	freeConstantPool(&bytecode->constPool);
	initBytecode(bytecode);
}

/**
 * Inserts the given Value into the constant pool of the bytecode chunk.
 * @returns FN_UWORD - index where the constant was appended.
*/
FN_UWORD
addConstant(Bytecode* bytecode, Value constant)
{
	push(constant);
	writeConstantPool(&bytecode->constPool, constant);
	pop();
	return (bytecode->constPool.count - 1);
}
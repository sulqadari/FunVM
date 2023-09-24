#include <stdio.h>
#include <stdint.h>

#include "debug.h"

static FN_UWORD
simpleInstruction(const char* name, FN_UWORD offset)
{
	printf("	%-16s\n", name);
	return offset + 1;
}

static FN_UWORD
byteInstruction(const char* name, Bytecode* bytecode, FN_UWORD offset)
{
	FN_UBYTE slot = bytecode->code[offset + 1];
	printf("	%-16s %4d\n", name, slot);
	return offset + 2;
}

static FN_UWORD
jumpInstruction(const char* name, FN_WORD sign, Bytecode* bytecode, FN_UWORD offset)
{
	FN_WORD jump = 0;

	jump |= (FN_WORD)((bytecode->code[offset] & 0xFF) << 8);
	jump |= (FN_WORD)(bytecode->code[offset + 1] & 0xFF);

	printf("	%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
	return offset + 3;
}

static FN_UWORD
constantInstruction(const char* name, Bytecode* bytecode, FN_UWORD offset)
{
	FN_UBYTE constant = bytecode->code[offset + 1];

	printf("	%-16s %4d : '", name, constant);
	printValue(bytecode->constPool.pool[constant]);
	printf("'\n");
	
	return offset + 2;
}

FN_UWORD
disassembleInstruction(Bytecode* bytecode, FN_UWORD offset)
{
	printf("%04d	", offset);

	/* If we aren't at the very beginning and the line of the current
	 * bytecode is the same as of the previous one, then just print '|'
	 * which means we are still on the same line of the source code. */
	if (offset > 0 &&
		bytecode->lines[offset] == bytecode->lines[offset - 1]) {
		printf("   | ");
	} else {
		printf("%4d ", bytecode->lines[offset]);
	}

	FN_UBYTE opcode = bytecode->code[offset];
	switch (opcode) {
		case OP_CONSTANT:
			return constantInstruction("OP_CONSTANT", bytecode, offset);
		case OP_NIL:
			return simpleInstruction("OP_NIL", offset);
		case OP_TRUE:
			return simpleInstruction("OP_TRUE", offset);
		case OP_FALSE:
			return simpleInstruction("OP_FALSE", offset);
		case OP_POP:
			return simpleInstruction("OP_POP", offset);
		case OP_GET_LOCAL:
			return byteInstruction("OP_GET_LOCAL", bytecode, offset);
		case OP_SET_LOCAL:
			return byteInstruction("OP_SET_LOCAL", bytecode, offset);
		case OP_DEFINE_GLOBAL:
			return constantInstruction("OP_DEFINE_GLOBAL", bytecode, offset);
		case OP_SET_GLOBAL:
			return constantInstruction("OP_SET_GLOBAL", bytecode, offset);
		case OP_GET_GLOBAL:
			return constantInstruction("OP_GET_GLOBAL", bytecode, offset);
		case OP_EQUAL:
			return simpleInstruction("OP_EQUAL", offset);
		case OP_GREATER:
			return simpleInstruction("OP_GREATER", offset);
		case OP_LESS:
			return simpleInstruction("OP_LESS", offset);
		case OP_ADD:
			return simpleInstruction("OP_ADD", offset);
		case OP_SUBTRACT:
			return simpleInstruction("OP_SUBTRACT", offset);
		case OP_MULTIPLY:
			return simpleInstruction("OP_MULTIPLY", offset);
		case OP_DIVIDE:
			return simpleInstruction("OP_DIVIDE", offset);
		case OP_NOT:
			return simpleInstruction("OP_NOT", offset);
		case OP_NEGATE:
			return simpleInstruction("OP_NEGATE", offset);
		case OP_PRINT:
			return simpleInstruction("OP_PRINT", offset);
		case OP_PRINTLN:
			return simpleInstruction("OP_PRINTLN", offset);
		case OP_JUMP:
			return jumpInstruction("OP_JUMP", 1, bytecode, offset);
		case OP_JUMP_IF_FALSE:
			return jumpInstruction("OP_JUMP_IF_FALSE", 1, bytecode, offset);
		case OP_LOOP:
			return jumpInstruction("OP_LOOP", -1, bytecode, offset);
		case OP_CALL:
			return byteInstruction("OP_CALL", bytecode, offset);
		case OP_RETURN:
			return simpleInstruction("OP_RETURN", offset);
		default:
			printf("Unknown opcode %d\n", opcode);
			return offset + 1;
	}
}

void
disassembleBytecode(Bytecode* bytecode, const char* name)
{
	printf("\n=== %s ===\n"
		"offset | line |    opcode    | cp_off : 'val'\n", name);
	for (FN_UWORD offset = 0; offset < bytecode->count; ) {
		offset = disassembleInstruction(bytecode, offset);
	}
}
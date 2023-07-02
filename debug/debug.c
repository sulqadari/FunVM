#include "debug.h"

void
disassembleBytecode(Bytecode *bytecode, const char *name)
{
	printf("=== %s ===\n", name);
	for (int offset = 0; offset < bytecode->count; ) {
		offset = disassembleInstruction(bytecode, offset);
	}
}

static int
simpleInstruction(const char *name, int offset)
{
	printf("%s\n", name);
	return offset + 1;
}

int
disassembleInstruction(Bytecode *bytecode, int offset)
{
	printf("%04d ", offset);

	uint8_t opcode = bytecode->code[offset];
	switch (opcode) {
		case OP_RETURN:
			return simpleInstruction("OP_RETURN", offset);
		default:
			printf("Unknown opcode %d\n", opcode);
			return offset + 1;
	}
}
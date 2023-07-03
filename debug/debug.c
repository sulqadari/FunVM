#include "debug.h"
#include "constant_pool.h"

void
disassembleBytecode(Bytecode *bytecode, const char *name)
{
	printf("=== %s ===\n"
		"offset | line | opcode name | const_pool offset : 'value'\n", name);
	for (int32_t offset = 0; offset < bytecode->count; ) {
		offset = disassembleInstruction(bytecode, offset);
	}
}

static int32_t
simpleInstruction(const char *name, int32_t offset)
{
	printf("	%-16s\n", name);
	return offset + 1;
}

static int32_t
constantInstruction(const char *name, Bytecode *bytecode, int32_t offset)
{
	uint8_t constant = bytecode->code[offset + 1];
	printf("	%-16s %4d : '", name, constant);
	printValue(bytecode->const_pool.pool[constant]);
	printf("'\n");
	return offset + 2;
}

int32_t
disassembleInstruction(Bytecode *bytecode, int32_t offset)
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

	uint8_t opcode = bytecode->code[offset];
	switch (opcode) {
		case OP_CONSTANT:
			return constantInstruction("OP_CONSTANT", bytecode, offset);
		case OP_RETURN:
			return simpleInstruction("OP_RETURN", offset);
		default:
			printf("Unknown opcode %d\n", opcode);
			return offset + 1;
	}
}
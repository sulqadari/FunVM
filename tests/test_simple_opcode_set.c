#include "funvmConfig.h"
#include "debug.h"

int
main(int32_t argc, char *argv[])
{
	Bytecode bytecode;
	uint32_t line = 1;
	uint32_t i = 0;

	initBytecode(&bytecode);

	for (i = 0; i < 256; ++i) {
		writeBytecode(&bytecode, OP_CONSTANT, line);
		writeBytecode(&bytecode, addConstant(&bytecode, i), line);

		if (i != 0 && (i % 5) == 0) {
			line++;
		}
	}

	for ( ; i < 513; ++i) {
		writeBytecode(&bytecode, OP_CONSTANT_LONG, line);
		writeBytecode(&bytecode, addConstant(&bytecode, i), line);

		if (i != 0 && (i % 5) == 0) {
			line++;
		}
	}

	writeBytecode(&bytecode, OP_RETURN, line);
	
	disassembleBytecode(&bytecode, "Test chunk");
	freeBytecode(&bytecode);

	return (0);
}
#include "funvmConfig.h"
#include "debug.h"

int32_t
main(int32_t argc, char *argv[])
{
	Bytecode bytecode;
	initBytecode(&bytecode);
	int line = 1;
	int i = 2;
	for (i = 0; i < 256; ++i) {
		writeBytecode(&bytecode, OP_CONSTANT, line);
		writeBytecode(&bytecode, addConstant(&bytecode, i), line);

		if (i != 0 && (i % 5) == 0) {
			line++;
		}
	}

	for ( ; i < 258; ++i) {
		writeBytecode(&bytecode, OP_CONSTANT_LONG, line);
		writeBytecodeLong(&bytecode, addConstant(&bytecode, i), line);

		if (i != 0 && (i % 5) == 0) {
			line++;
		}
	}

	writeBytecode(&bytecode, OP_RETURN, line);
	
	disassembleBytecode(&bytecode, "Test chunk");
	freeBytecode(&bytecode);

	return (0);
}
#include "funvmConfig.h"
#include "debug.h"

int
main(int argc, char *argv[])
{
	Bytecode bytecode;
	initBytecode(&bytecode);
	for (int i = 0; i < 16; ++i)
		writeBytecode(&bytecode, OP_RETURN);
	
	disassembleBytecode(&bytecode, "Test chunk");
	freeBytecode(&bytecode);

	return (0);
}
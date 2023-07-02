#include "funvmConfig.h"
#include "common.h"
#include "bytecode.h"

int
main(int argc, char *argv[])
{
	Bytecode bytecode;
	initBytecode(&bytecode);
	writeBytecode(&bytecode, OP_RETURN);
	freeBytecode(&bytecode);

	return (0);
}
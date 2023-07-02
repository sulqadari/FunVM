#include "funvmConfig.h"
#include "debug.h"

int32_t
main(int32_t argc, char *argv[])
{
	Bytecode bytecode;
	initBytecode(&bytecode);
	
	writeBytecode(&bytecode, OP_CONSTANT, 1);
	/* Write OP_CONSTANT operand which indicates the offset
	 * of constant value 1.2 in constant pool. */
	writeBytecode(&bytecode, addConstant(&bytecode, 1.2), 1);

	writeBytecode(&bytecode, OP_CONSTANT, 1);
	writeBytecode(&bytecode, addConstant(&bytecode, 3.14), 1);

	writeBytecode(&bytecode, OP_CONSTANT, 1);
	writeBytecode(&bytecode, addConstant(&bytecode, 42), 1);

	writeBytecode(&bytecode, OP_CONSTANT, 2);
	writeBytecode(&bytecode, addConstant(&bytecode, 0.42), 2);

	writeBytecode(&bytecode, OP_CONSTANT, 2);
	writeBytecode(&bytecode, addConstant(&bytecode, 0.0012), 2);

	writeBytecode(&bytecode, OP_RETURN, 3);
	
	disassembleBytecode(&bytecode, "Test chunk");
	freeBytecode(&bytecode);

	return (0);
}
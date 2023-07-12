#include "funvmConfig.h"
#include "debug.h"
#include "vm.h"

void case_1(void);
void case_2(void);
void case_3(void);
void case_4(void);
void case_5(void);
void case_6(void);

int
main(int argc, char *argv[])
{
	
	case_1();
	case_2();
	case_3();
	case_4();
	case_5();

	return (0);
}

void
case_1(void)
{
	Bytecode bytecode;
	uint32_t line = 1;

	initVM();
	initBytecode(&bytecode);

	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 1.2), line);
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 3.4), line);
	writeBytecode(&bytecode, OP_ADD, line);
	
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 5.6), line);
	writeBytecode(&bytecode, OP_DIVIDE, line++);

	writeBytecode(&bytecode, OP_NEGATE, line++);
	writeBytecode(&bytecode, OP_RETURN, line++);

	interpret(&bytecode);
	freeVM();
	freeBytecode(&bytecode);
}

/* 1 * 2 + 3 */
void
case_2(void)
{
	Bytecode bytecode;
	uint32_t line = 1;

	initVM();
	initBytecode(&bytecode);

	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 1), line);
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 2), line);
	writeBytecode(&bytecode, OP_MULTIPLY, line);
	
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 3), line);
	writeBytecode(&bytecode, OP_ADD, line);
	
	writeBytecode(&bytecode, OP_RETURN, line++);

	interpret(&bytecode);
	freeVM();
	freeBytecode(&bytecode);
}

/* 1 + 2 * 3 */
void
case_3(void)
{
	Bytecode bytecode;
	uint32_t line = 1;

	initVM();
	initBytecode(&bytecode);

	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 2), line);
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 3), line);
	writeBytecode(&bytecode, OP_MULTIPLY, line);
	
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 1), line);
	writeBytecode(&bytecode, OP_ADD, line);
	
	writeBytecode(&bytecode, OP_RETURN, line++);

	interpret(&bytecode);
	freeVM();
	freeBytecode(&bytecode);
}

/* 3 - 2 - 1 */
void
case_4(void)
{
	Bytecode bytecode;
	uint32_t line = 1;

	initVM();
	initBytecode(&bytecode);

	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 3), line);
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 2), line);
	writeBytecode(&bytecode, OP_SUBTRACT, line);
	
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 1), line);
	writeBytecode(&bytecode, OP_SUBTRACT, line);
	
	writeBytecode(&bytecode, OP_RETURN, line++);

	interpret(&bytecode);
	freeVM();
	freeBytecode(&bytecode);
}

/* 1 + 2 * 3 - 4 / 5 */
void
case_5(void)
{
	Bytecode bytecode;
	uint32_t line = 1;

	initVM();
	initBytecode(&bytecode);

	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 2), line);
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 3), line);
	writeBytecode(&bytecode, OP_MULTIPLY, line);
	
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 4), line);
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 5), line);
	writeBytecode(&bytecode, OP_NEGATE, line);
	writeBytecode(&bytecode, OP_DIVIDE, line);
	writeBytecode(&bytecode, OP_SUBTRACT, line);
	
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 1), line);
	writeBytecode(&bytecode, OP_ADD, line);

	writeBytecode(&bytecode, OP_RETURN, line++);

	interpret(&bytecode);
	freeVM();
	freeBytecode(&bytecode);
}

/* 4 - 3 * -2 without using OP_NEGATE */
void
case_6(void)
{
	Bytecode bytecode;
	uint32_t line = 1;

	initVM();
	initBytecode(&bytecode);

	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 3), line);
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 2), line);
	writeBytecode(&bytecode, OP_MULTIPLY, line);
	
	writeBytecode(&bytecode, OP_CONSTANT, line);
	writeBytecode(&bytecode, addConstant(&bytecode, 4), line);
	writeBytecode(&bytecode, OP_ADD, line);

	writeBytecode(&bytecode, OP_RETURN, line++);

	interpret(&bytecode);
	freeVM();
	freeBytecode(&bytecode);
}
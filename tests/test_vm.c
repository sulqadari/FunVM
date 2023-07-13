#include "funvmConfig.h"

#include <stdio.h>

#include "debug.h"
#include "vm.h"

void case_1(void);
void case_2(void);
void case_3(void);
void case_4(void);
void case_5(void);
void case_6(void);
void case_7(void);

int
main(int argc, char *argv[])
{
	
	case_1();
	case_2();
	case_3();
	case_4();
	case_5();
	case_6();
	case_7();
	return (0);
}

void
case_1(void)
{
	Bytecode bytecode;
	VM vm;
	uint32_t line = 1;

	printf("*** 1.2 + (3.4 / 5.6) ***\n");

	initVM(&vm);
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

	interpret(&vm, &bytecode);
	freeVM(&vm);
	freeBytecode(&bytecode);
}

void
case_2(void)
{
	Bytecode bytecode;
	VM vm;
	uint32_t line = 1;

	printf("*** (1 * 2) + 3 ***\n");

	initVM(&vm);
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

	interpret(&vm, &bytecode);
	freeVM(&vm);
	freeBytecode(&bytecode);
}

void
case_3(void)
{
	Bytecode bytecode;
	VM vm;
	uint32_t line = 1;

	printf("*** 1 + (2 * 3) ***\n");

	initVM(&vm);
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

	interpret(&vm, &bytecode);
	freeVM(&vm);
	freeBytecode(&bytecode);
}

void
case_4(void)
{
	Bytecode bytecode;
	VM vm;
	uint32_t line = 1;

	printf("*** 3 - 2 - 1 ***\n");

	initVM(&vm);
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

	interpret(&vm, &bytecode);
	freeVM(&vm);
	freeBytecode(&bytecode);
}

void
case_5(void)
{
	Bytecode bytecode;
	VM vm;
	uint32_t line = 1;

	printf("*** 1 + ((2 * 3) - (4 / (-5))) ***\n");

	initVM(&vm);
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

	interpret(&vm, &bytecode);
	freeVM(&vm);
	freeBytecode(&bytecode);
}

void
case_6(void)
{
	Bytecode bytecode;
	VM vm;
	uint32_t line = 1;

	printf("*** 4 - (3 * (- 2)) (sans OP_NEGATE) ***\n");

	initVM(&vm);
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

	interpret(&vm, &bytecode);
	freeVM(&vm);
	freeBytecode(&bytecode);
}

void
case_7(void)
{
	Bytecode bytecode;
	VM vm;
	uint32_t line = 1;

	printf("*** Increasing stack size. ***\n");

	initVM(&vm);
	initBytecode(&bytecode);

	for (int i = 0; i < 64; ++i) {
		writeBytecode(&bytecode, OP_CONSTANT, line);
		writeBytecode(&bytecode, addConstant(&bytecode, i), line);
	}
	
	writeBytecode(&bytecode, OP_RETURN, line++);

	interpret(&vm, &bytecode);
	freeVM(&vm);
	freeBytecode(&bytecode);
}
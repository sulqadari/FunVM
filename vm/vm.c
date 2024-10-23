#include "vm.h"

VM vm;

void
initVM(void)
{

}

void
freeVM(void)
{

}

static InterpretResult
run(void)
{

}

InterpretResult
interpret(ByteCode* bCode)
{
	vm.bCode = bCode;
	vm.ip = vm.bCode->code;
	return run();
}
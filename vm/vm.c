#include "vm.h"

/* Reads the byte currently pointed at by 'ip' and
 * then advances the instruction pointer. */
#define READ_BYTE()				\
	(*vm.ip++)

#define READ_SHORT()			\
	(((*vm.ip++) << 8) | (*vm.ip++))

#define READ_CONSTANT()			\
	(vm.bCode->constants.values[READ_BYTE()])

#define READ_CONSTANT_LONG()	\
	(vm.bCode->constants.values[READ_SHORT()])

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
	uint8_t ins;
	while (true) {
		ins = READ_BYTE();
		switch (ins) {
			case op_iconst: {
				i32 constant = READ_CONSTANT();
				printConstValue(constant);
				printf("\n");
			} break;
			case op_iconst_long: {
				i32 constant = READ_CONSTANT_LONG();
				printConstValue(constant);
				printf("\n");
			} break;
			case op_add:
			case op_sub:
			case op_mul:
			case op_div:
			case op_negate:
			case op_ret: {
				return INTERPRET_OK;
			}
		}
	}
}

InterpretResult
interpret(ByteCode* bCode)
{
	vm.bCode = bCode;
	vm.ip = vm.bCode->code;
	return run();
}
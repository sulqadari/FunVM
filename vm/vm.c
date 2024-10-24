#include "vm.h"

/* Reads the byte currently pointed at by 'ip' and
 * then advances the instruction pointer. */
#define READ_BYTE()				\
	(*vm.ip++)

// #define READ_SHORT()			
	// ((*vm.ip++ << 8) | (*vm.ip++))

#define READ_CONSTANT()			\
	(vm.bCode->constants.values[READ_BYTE()])

// #define READ_CONSTANT_LONG()	
	// (vm.bCode->constants.values[READ_SHORT()])

#define BINARY_OP(op)			\
	do {						\
		i32 b = pop();			\
		i32 a = pop();			\
		push(a op b);			\
	} while(false)

VM vm;

static void
resetStack(void)
{
	vm.stackTop = vm.stack;
}

void
initVM(void)
{
	resetStack();
}

void
freeVM(void)
{

}

void push(Value value)
{
	*vm.stackTop = value;
	vm.stackTop++;
}

Value pop(void)
{
	vm.stackTop--;
	return *vm.stackTop;
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
				push(constant);
			} break;
			case op_iconst_long: {
				// i32 constant = READ_CONSTANT_LONG();
				// push(constant);
			} break;
			case op_add: {
				BINARY_OP(+); 
			} break;
			case op_sub: {
				BINARY_OP(-); 
			} break;
			case op_mul: {
				BINARY_OP(*); 
			} break;
			case op_div: {
				BINARY_OP(/); 
			} break;
			case op_negate: {
				push(-pop());
			} break;
			case op_ret: {
				printConstValue(pop());
				printf("\n");
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
	InterpretResult result = run();
	return result;
}
#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

/* Makes stackTop point to the beginning of the stack,
 * that indicates that stack is empty. */
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

void
push(Value value)
{
	*vm.stackTop = value;	/* push the value onto the stack. */
	vm.stackTop++;				/* shift stackTop forward. */
}

Value
pop(void)
{
	vm.stackTop--;			/* move stack ptr back to the recent used slot. */
	return *vm.stackTop;	/* retrieve the value at that index in stack. */
}



/**
 * Reads and executes a single bytecode instruction.
 * This function is highly performance critical.
 * The best practices of dispatching the instrucitons
 * are: 'binary search', 'direct threaded code', 'jump table'
 * and 'computed goto'.
 */
static InterpretResult
run(void)
{
/* Read current byte, then advance ip. */
#define READ_BYTE() (*vm.ip++)

/* Read the next byte from the bytecode, treat it as an index,
 * and look up the corresponding Value in the bytecode's const_pool. */
#define READ_CONSTANT() (vm.bytecode->const_pool.pool[READ_BYTE()])


#define BINARY_OP(op)	\
	do {				\
		Value b = pop();\
		Value a = pop();\
		push(a op b);	\
	} while(false)

	for (;;) {
#ifdef FUNVM_DEBUG
		printf("		");
		for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
			printf("[ ");
			printValue(*slot);
			printf(" ]");
		}
		printf("\n");

		disassembleInstruction(vm.bytecode,
				(int32_t)(vm.ip - vm.bytecode->code));
#endif // !FUNVM_DEBUG
		uint8_t ins;
		switch (ins = READ_BYTE()) {
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				push(constant);
			} break;
			case OP_ADD:		BINARY_OP(+); break;
			case OP_SUBTRACT:	BINARY_OP(-); break;
			case OP_MULTIPLY:	BINARY_OP(*); break;
			case OP_DIVIDE:		BINARY_OP(/); break;
			case OP_NEGATE:		push(-pop()); break;
			case OP_RETURN: {
				printValue(pop());
				printf("\n");
				return IR_OK;
			}
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(Bytecode *bytecode)
{
	vm.bytecode = bytecode;
	vm.ip = vm.bytecode->code;
	return run();
}
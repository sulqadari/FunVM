#include "funvmConfig.h"
#include <stdio.h>
<<<<<<< HEAD
#include <stdlib.h>
// #include <sys/endian.h>
=======
>>>>>>> 72a1365 (Chapter 16 "Scanning on Demand")
#include "common.h"
#include "debug.h"
#include "vm.h"
#include "memory.h"

/* Makes stackTop point to the beginning of the stack,
 * that indicates that stack is empty. */
static void
resetStack(VM *vm)
{
	vm->stackTop = vm->stack;
}

void
initVM(VM *vm)
{
	vm->bytecode = NULL;
	vm->ip = NULL;
	vm->stack = NULL;
	vm->stackTop = NULL;
	vm->stackSize = 0;
	resetStack(vm);
}

void
freeVM(VM *vm)
{
	FREE_ARRAY(Value, vm->stack, vm->stackSize);
}

void
push(Value value, VM *vm)
{
	uint32_t stackOffset = ((vm->stackTop) - vm->stack);

	if (vm->stackSize < stackOffset + 1) {
		uint32_t oldSize = vm->stackSize;
		vm->stackSize = INCREASE_CAPACITY(oldSize);
		vm->stack = INCREASE_ARRAY(Value, vm->stack,
								oldSize, vm->stackSize);

		/* We could avoid this step if could be sure that 
		 * realloc() wouldn't cause vm->stack points to another
		 * memory block, but truth is that it can do so,
		 * thus vm->stackTop will point to unused block which
		 * will result in segmentation fault.
		 * Also we mind the correct offset within new block. */
		vm->stackTop = (vm->stack + stackOffset);
#ifdef FUNVM_DEBUG
		printf("old: %d\nnew: %d\nstackTop: %.2f\n", oldSize, vm->stackSize, *vm->stackTop);
#endif	// !FUNVM_DEBUG
	}
	
	*vm->stackTop = value;	/* push the value onto the stack. */
	vm->stackTop++;				/* shift stackTop forward. */
}

Value
pop(VM *vm)
{
	vm->stackTop--;			/* move stack ptr back to the recent used slot. */
	return *vm->stackTop;	/* retrieve the value at that index in stack. */
}



/**
 * Reads and executes a single bytecode instruction.
 * This function is highly performance critical.
 * The best practices of dispatching the instrucitons
 * are: 'binary search', 'direct threaded code', 'jump table'
 * and 'computed goto'.
 */
static InterpretResult
run(VM *vm)
{
/* Read current byte, then advance ip. */
#define READ_BYTE() (*vm->ip++)

/* Read the next byte from the bytecode, treat it as an index,
 * and look up the corresponding Value in the bytecode's const_pool. */
#define READ_CONSTANT() (vm->bytecode->const_pool.pool[READ_BYTE()])


#define BINARY_OP(op)		\
	do {					\
		Value b = pop(vm);	\
		Value a = pop(vm);	\
		push(a op b, vm);	\
	} while(false)

	for (;;) {
#ifdef FUNVM_DEBUG
		printf("		");
		for (Value *slot = vm->stack; slot < vm->stackTop; slot++) {
			printf("[ ");
			printValue(*slot);
			printf(" ]");
		}
		printf("\n");

		disassembleInstruction(vm->bytecode,
				(int32_t)(vm->ip - vm->bytecode->code));
#endif // !FUNVM_DEBUG
		uint8_t ins;
		switch (ins = READ_BYTE()) {
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				push(constant, vm);
			} break;
			case OP_ADD:		BINARY_OP(+); break;
			case OP_SUBTRACT:	BINARY_OP(-); break;
			case OP_MULTIPLY:	BINARY_OP(*); break;
			case OP_DIVIDE:		BINARY_OP(/); break;
//			case OP_NEGATE:		push(-pop()); break;

			/* vm->stackTop points to the next free block in stack,
			 * while a value to be negated resides one step back. */
			case OP_NEGATE: {
				*(vm->stackTop - 1) = -(*(vm->stackTop - 1));
			} break;
			case OP_RETURN: {
#ifdef FUNVM_DEBUG
				printValue(pop(vm));
				printf("\n");
#endif // !FUNVM_DEBUG
				return IR_OK;
			}
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(VM *vm, Bytecode *bytecode)
{
	vm->bytecode = bytecode;
	vm->ip = vm->bytecode->code;
	return run(vm);
}
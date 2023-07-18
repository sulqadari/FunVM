#include "funvmConfig.h"

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include "bytecode.h"
#include "common.h"
#include "vm.h"
#include "memory.h"
#include "compiler.h"
#ifdef FUNVM_DEBUG
#include "debug.h"
#endif

/* Makes stackTop point to the beginning of the stack,
 * that indicates that stack is empty. */
static void
resetStack(VM *vm)
{
	vm->stackTop = vm->stack;
}

static void
runtimeError(VM *vm, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	size_t instruction = (vm->ip - (vm->bytecode->code - 1));
	int line = vm->bytecode->lines[instruction];
	fprintf(stderr, "[line %d] in script\n", line);
	resetStack(vm);
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
		printf("old: %d\nnew: %d\nstackTop: %.2f\n", oldSize, vm->stackSize, NUMBER_UNPACK(*vm->stackTop));
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

static Value peek(VM *vm, int32_t offset)
{
	return vm->stackTop[((-1) - offset)];
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
/* Read current byte, then advance istruction pointer. */
#define READ_BYTE() (*vm->ip++)

/* Read the next byte from the bytecode, treat it as an index,
 * and look up the corresponding Value in the bytecode's const_pool. */
#define READ_CONSTANT() (vm->bytecode->const_pool.pool[READ_BYTE()])


#define BINARY_OP(valueType, op)									\
	do {															\
		if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1)))	{	\
			runtimeError(vm, "Operands must be numbers.");			\
			return IR_RUNTIME_ERROR;								\
		}															\
																	\
		double b = NUMBER_UNPACK(pop(vm));							\
		double a = NUMBER_UNPACK(pop(vm));							\
		push(valueType(a op b), vm);								\
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
			case OP_NIL:		push(NIL_PACK, vm);			break;
			case OP_TRUE:		push(BOOL_PACK(true), vm);	break;
			case OP_FALSE:		push(BOOL_PACK(false), vm);	break;
			case OP_ADD:		BINARY_OP(NUMBER_PACK, +); break;
			case OP_SUBTRACT:	BINARY_OP(NUMBER_PACK, -); break;
			case OP_MULTIPLY:	BINARY_OP(NUMBER_PACK, *); break;
			case OP_DIVIDE:		BINARY_OP(NUMBER_PACK, /); break;

			/* vm->stackTop points to the next free block in stack,
			 * while a value to be negated resides one step back. */
			case OP_NEGATE: {
				if (!IS_NUMBER(peek(vm, 0))) {
					runtimeError(vm, "Operand must be a number.");
					return IR_RUNTIME_ERROR;
				}

				double temp = NUMBER_UNPACK(vm->stackTop[-1]);
				vm->stackTop[-1] = NUMBER_PACK(-temp);
			} break;
			case OP_RETURN: {
#ifdef FUNVM_DEBUG
				printValue(pop(vm));
				printf("\n");
#else
				pop(vm);
#endif // !FUNVM_DEBUG
				return IR_OK;
			}
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult
interpret(VM *vm, const char *source)
{
	Bytecode bytecode;
	initBytecode(&bytecode);

	/* Fill in the bytecode with the instructions retrieved
	 * from source code. */
	if (!compile(source, &bytecode)) {
		freeBytecode(&bytecode);
		return IR_COMPILE_ERROR;
	}

	/* Initialize VM with bytecode. */
	vm->bytecode = &bytecode;

	/* Assign VM a source code.
	 * NOTE: current runtime highly depends on source code,
	 * actually if we just remove it from the middle of execution,
	 * the runtime will fail.
	 * Next improvements shall eliminate this case. */
	vm->ip = vm->bytecode->code;

	InterpretResult result = run(vm);
	freeBytecode(&bytecode);

	return result;
}
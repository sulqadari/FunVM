#include "funvmConfig.h"

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "bytecode.h"
#include "common.h"
#include "vm.h"
#include "memory.h"
#include "compiler.h"
#include "object.h"
#include "table.h"

#ifdef FUNVM_DEBUG
#include "debug.h"
#endif

static VM *vm;

/* Makes stackTop point to the beginning of the stack,
 * that indicates that stack is empty. */
static void
resetStack(void)
{
	vm->stackTop = vm->stack;
}

static void
runtimeError(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	size_t instruction = (vm->ip - (vm->bytecode->code - 1));
	int line = vm->bytecode->lines[instruction];
	fprintf(stderr, "[line %d] in script\n", line);
	resetStack();
}

void
initVM(VM *_vm)
{
	vm = _vm;
	vm->bytecode = NULL;
	vm->ip = NULL;
	vm->stack = NULL;
	vm->stackTop = NULL;
	vm->stackSize = 0;
	vm->objects = NULL;
	initTable(&vm->globals);
	initTable(&vm->interns);
	resetStack();
}

void
freeVM(VM *vm)
{
	/* Release stack. */
	FREE_ARRAY(Value, vm->stack, vm->stackSize);
	freeTable(vm->globals);
	freeTable(vm->interns);

	/* Release heap. */
	freeObjects(vm);
}

static void
push(Value value)
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
pop(void)
{
	vm->stackTop--;			/* move stack ptr back to the recent used slot. */
	return *vm->stackTop;	/* retrieve the value at that index in stack. */
}

static Value
peek(int32_t offset)
{
	return vm->stackTop[((-1) - offset)];
}

/** The 'falsiness' rule.
 * 'nil' and 'false' are falsey and any other value behaves like a true.
 */
static bool
isFalsey(Value value)
{
	return IS_NIL(value) ||
		(IS_BOOL(value) && !BOOL_UNPACK(value));
}

static void
concatenate()
{
	ObjString *b = STRING_UNPACK(pop());
	ObjString *a = STRING_UNPACK(pop());

	int32_t length = a->length + b->length;
	char *chars = ALLOCATE(char, length + 1);
	memcpy(chars, a->chars, a->length);
	memcpy(chars + a->length, b->chars, b->length);
	chars[length] = '\0';

	ObjString *result = takeString(chars, length);
	push(OBJECT_PACK(result));
}

/* Read current byte, then advance istruction pointer. */
#define READ_BYTE() (*vm->ip++)

/* Read the next byte from the bytecode, treat it as an index,
 * and look up the corresponding Value in the bytecode's const_pool. */
#define READ_CONSTANT() (vm->bytecode->const_pool.pool[READ_BYTE()])

#define READ_CONSTANT_LONG(offset) (vm->bytecode->const_pool.pool[offset])

#define READ_STRING() STRING_UNPACK(READ_CONSTANT())

#define BINARY_OP(valueType, op)									\
	do {															\
		if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1)))	{	\
			runtimeError("Operands must be numbers.");			\
			return IR_RUNTIME_ERROR;								\
		}															\
																	\
		double b = NUMBER_UNPACK(pop());							\
		double a = NUMBER_UNPACK(pop());							\
		push(valueType(a op b));								\
	} while(false)

#ifdef FUNVM_DEBUG
static void
logRun()
{
	printf("		");
	for (Value *slot = vm->stack; slot < vm->stackTop; slot++) {
		printf("[ ");
		printValue(*slot);
		printf(" ]");
	}
	printf("\n");

	disassembleInstruction(vm->bytecode,
			(int32_t)(vm->ip - vm->bytecode->code));
}
#endif // !FUNVM_DEBUG

/**
 * Reads and executes a single bytecode instruction.
 * This function is highly performance critical.
 * The best practices of dispatching the instrucitons
 * are: 'binary search', 'direct threaded code', 'jump table'
 * and 'computed goto'.
 */
static InterpretResult
run()
{
	for (;;) {

#ifdef FUNVM_DEBUG
		logRun();
#endif // !FUNVM_DEBUG

		uint8_t ins;
		switch (ins = READ_BYTE()) {
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				push(constant);
			} break;
			case OP_CONSTANT_LONG: {
				uint32_t offset = 0;
				offset |= (READ_BYTE() << 16);
				offset |= (READ_BYTE() << 8);
				offset |=  READ_BYTE();
				Value constant = READ_CONSTANT_LONG(offset);
				push(constant);
			} break;
			case OP_NIL:	push(NIL_PACK());		break;
			case OP_TRUE:	push(BOOL_PACK(true));	break;
			case OP_FALSE:	push(BOOL_PACK(false));	break;
			case OP_POP:	pop();					break;
			case OP_GET_GLOBAL: {

				// get the name of the variable from the constant pool
				ObjString *name = READ_STRING();
				Value value;
				if (!tableGet(vm->globals, name, &value)) {
					runtimeError("Undefined variable '%s'.", name->chars);
					return IR_RUNTIME_ERROR;
				}
				push(value);
			} break;
			case OP_DEFINE_GLOBAL: {
				
				// get the name of the variable from the constant pool
				ObjString *name = READ_STRING();

				// Take the value from the top of the stack and
				// store it in a hash table with that name as the key
				// NOTE: there is no check to see if the key is already in the
				// table. This allows redefine global variables without error.
				tableSet(vm->globals, name, peek(0));
				
				// In the end, pop the value from the stack.
				pop();
			} break;
			case OP_SET_GLOBAL: {
				// get the name of the variable from the constant pool
				ObjString *name = READ_STRING();

				/* Assign a value to an existing variable.
				 * If tableSet() returns TRUE, than mean that we didn't overwrite
				 * an existing variable, but have created a new one.
				 * Thus, delete it and throw the runtime error, because current
				 * implementation doesn't support implicit variable declarations. */
				if (tableSet(vm->globals, name, peek(0))) {
					tableDelete(vm->globals, name);
					runtimeError("Undefined variable '%s'.", name->chars);
					return IR_RUNTIME_ERROR;
				}

				/* The pop() function is not called due to assignment is an expression,
				 * so it needs to leave that value on the stack in case the assignment
				 * is nested inside some larger expression.
				 */
			} break;
			case OP_EQUAL: {
				Value b = pop();
				Value a = pop();
				push(BOOL_PACK(valuesEqual(a, b)));
			} break;
			case OP_GREATER:	BINARY_OP(BOOL_PACK, >);	break;
			case OP_LESS:		BINARY_OP(BOOL_PACK, <);	break;

			//case OP_ADD:		BINARY_OP(NUMBER_PACK, +);	break;
			case OP_ADD: {
				if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
					concatenate(vm);
				} else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
					double b = NUMBER_UNPACK(pop());
					double a = NUMBER_UNPACK(pop());
					push(NUMBER_PACK(a + b));
				} else {
					runtimeError("Operands must be two numbers or two strings.");
					return IR_RUNTIME_ERROR;
				}
			} break;
			case OP_SUBTRACT:	BINARY_OP(NUMBER_PACK, -);	break;
			case OP_MULTIPLY:	BINARY_OP(NUMBER_PACK, *);	break;
			case OP_DIVIDE:		BINARY_OP(NUMBER_PACK, /);	break;
			case OP_NOT: {
				//push(BOOL_PACK(isFalsey(pop())), vm);
				vm->stackTop[-1] = BOOL_PACK(isFalsey(vm->stackTop[-1]));
			} break;

			/* vm->stackTop points to the next free block in the stack,
			 * while a value to be negated resides one step back. */
			case OP_NEGATE: {
				if (!IS_NUMBER(peek(0))) {
					runtimeError("Operand must be a number.");
					return IR_RUNTIME_ERROR;
				}

				double temp = NUMBER_UNPACK(vm->stackTop[-1]);
				vm->stackTop[-1] = NUMBER_PACK(-temp);
			} break;
			case OP_PRINT: {
				printValue(pop());
				printf("\n");
			} break;
			case OP_RETURN: {
				return IR_OK;
			}
		}
	}
}

InterpretResult
interpret(const char *source)
{
	Bytecode bytecode;
	initBytecode(&bytecode);
	objModuleVmRef(vm);

	/* Fill in the bytecode with the instructions retrieved
	 * from source code. */
	if (compile(source, &bytecode)) {
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
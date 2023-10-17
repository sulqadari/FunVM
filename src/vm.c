#include "funvmConfig.h"

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

static VM* vm;

/**
 * Returns the elapsed type since the program started running, in seconds.
 */
static Value
clockNative(FN_BYTE argCount, Value* args)
{
	return NUMBER_PACK((FN_FLOAT)clock() / CLOCKS_PER_SEC);
}

/* Makes stackTop point to the beginning of the stack,
 * that indicates that stack is empty. */
static void
resetStack(void)
{
	vm->stackTop = vm->stack;
	vm->frameCount = 0;
}

static void
runtimeError(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	for (FN_WORD i = vm->frameCount - 1; i >= 0; --i) {

		CallFrame* frame = &vm->frames[i];
		ObjFunction* function = frame->function;

		/* (-1) is because the frame->ip is already sitting on the next 
		 * instruction to be executed but we want the stack trace to point to
		 * the previous failed instruction. */
		FN_UWORD instruction = (frame->ip - function->bytecode.code - 1);
		fprintf(stderr, "[line %d] in ", function->bytecode.lines[instruction]);

		if (NULL == function->name)
			fprintf(stderr, "script\n");
		else
			fprintf(stderr, "%s()\n", function->name->chars);
	}

	resetStack();
}

static void
push(Value value)
{
	*vm->stackTop = value;	/* push the value onto the stack. */
	vm->stackTop++;				/* shift stackTop forward. */
}

Value
pop(void)
{
	vm->stackTop--;			/* move stack ptr back to the recent used slot. */
	return* vm->stackTop;	/* retrieve the value at that index in stack. */
}

/**
 * Returns a values from the stack at one index behind the given offset.
*/
static Value
peek(FN_WORD offset)
{
	return vm->stackTop[(-1) - offset];
}

/**
 * Foreign function interface is used to provide a users with
 * capability to define their own native functions.
 * This function takes the pointer to a C function and the name it will
 * be known as in FunVM.
 */
static void
defineNative(const char* name, NativeFn function)
{
	/* Store the name of the native function on top of the stack. */
	push(OBJECT_PACK(copyString(name, (FN_UWORD)strlen(name))));

	/* Wrap the function in an ObjNative struct and push onto the stack. */
	push(OBJECT_PACK(newNative(function)));

	/* Store in a global variable with the given name. */
	tableSet(vm->globals, STRING_UNPACK(vm->stack[0]), vm->stack[1]);
	pop();
	pop();
}

void
initVM(VM* _vm)
{
	vm = _vm;
	vm->stackTop = NULL;
	vm->objects = NULL;
	initTable(&vm->globals);
	initTable(&vm->interns);

	resetStack();
	objSetVM(vm);
	defineNative("clock", clockNative);
}

void
freeVM(VM* vm)
{
	freeTable(vm->globals);
	freeTable(vm->interns);

	/* Release heap. */
	freeObjects(vm);
}

/**
 * Initializes the next CallFrame on the stack to be called.
 * This function stores a pointer to the function being called and points
 * the frame's 'ip' to the beginning of the function's bytecode.
 * It sets up the 'slots' pointer to give the frame its own window into the stack.
 * @param ObjFunction*: contains the compiled code.
 */
static bool
call(ObjFunction* function, FN_WORD argCount)
{
	if (argCount != function->arity) {
		runtimeError("Expected %d arguments, but got %d.",
										function->arity, argCount);
		return false;
	}

	if (vm->frameCount >= FRAMES_MAX) {
		runtimeError("Stack overflow.");
		return false;
	}

	/* Fetch subsequent frame slot from the CallFrame array. */
	CallFrame* frame = &vm->frames[vm->frameCount++];

	/* Reference the function being called. */
	frame->function = function;

	/* get pointer to the beginning of the bytecode
	 * dedicated for this frame. */
	frame->ip = function->bytecode.code;

	/* Set up stack window (aka "Frame pointer").
	 * The following expression resolves the level of indirection.
	 * Consider example:
	 * vm->stackTop = 1;
	 * argCount = 0;
	 * then, the stack window starts from the zeroth slot.
	 * the (-1) is to account for stack slot zero, which the
	 * compiler set aside for when we add methods. */
	frame->slots = vm->stackTop - argCount - 1;

	return true;
}

static bool
callValue(Value callee, FN_BYTE argCount)
{
	if (IS_OBJECT(callee)) {
		
		switch (OBJECT_TYPE(callee)) {
			
			case OBJ_FUNCTION:
				return call(FUNCTION_UNPACK(callee), argCount);
			
			/* If the object being called is a native function, we invoke
			 * the C function right then and there. The value returned by
			 * this call is stored onto the stack. */
			case OBJ_NATIVE: {
				NativeFn native = NATIVE_UNPACK(callee);
				Value result = native(argCount, vm->stackTop - argCount);

				/* Discard */
				vm->stackTop -= argCount + 1;
				push(result);
				return true;
			}
			default:
				/* Non-callabe object type. */
			break;
		}
	}

	runtimeError("Can only call functions and classes.");
	return false;
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
	ObjString* b = STRING_UNPACK(pop());
	ObjString* a = STRING_UNPACK(pop());

	FN_UWORD length = a->length + b->length;
	char* chars = ALLOCATE(char, length + 1);

	memcpy(chars, a->chars, a->length);
	memcpy(chars + a->length, b->chars, b->length);
	chars[length] = '\0';

	ObjString* result = takeString(chars, length);
	push(OBJECT_PACK(result));
}

/* Read current byte, then advance istruction pointer. */
#define READ_BYTE() \
	(*frame->ip++)

#define READ_SHORT()	\
	(frame->ip += 2, (FN_UWORD) ((frame->ip[-2] <<  8) | ((frame->ip[-1]) & 0xFF) ))

/* Read the next byte from the bytecode, treat it as an index,
 * and look up the corresponding Value in the bytecode's constPool. */
#define READ_CONSTANT() \
	(frame->function->bytecode.constPool.pool[READ_BYTE()])


#define READ_STRING() \
	STRING_UNPACK(READ_CONSTANT())

#define BINARY_OP(valueType, op)							\
	do {													\
		if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1)))	{	\
			runtimeError("Operands must be numbers.");		\
			return IR_RUNTIME_ERROR;						\
		}													\
															\
		FN_FLOAT b = NUMBER_UNPACK(pop());					\
		FN_FLOAT a = NUMBER_UNPACK(pop());					\
		push(valueType(a op b));							\
	} while(false)


#ifdef FUNVM_DEBUG
static void
logRun(CallFrame* frame)
{
	disassembleInstruction(&frame->function->bytecode,
			(FN_UWORD)(frame->ip - frame->function->bytecode.code));

	printf("		");
	for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
		printf("[ ");
		printValue(*slot);
		printf(" ]\n");
	}
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
	FN_UBYTE ins;

	/* Store the topmost CallFrame. Its 'ip' field will be used for initial
	 * access to the bytecode instruction set. */
	register CallFrame* frame = &vm->frames[vm->frameCount - 1];

#ifdef FUNVM_DEBUG
	printf( "\n************************************\n"
			"    Firing up Virtual Machine"
			"\n************************************\n");
#endif // !FUNVM_DEBUG

	for (;;) {

#ifdef FUNVM_DEBUG
		logRun(frame);
#endif // !FUNVM_DEBUG

		switch (ins = READ_BYTE()) {

			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				push(constant);
			} break;
			
			case OP_NIL:	push(NIL_PACK());		break;
			case OP_TRUE:	push(BOOL_PACK(true));	break;
			case OP_FALSE:	push(BOOL_PACK(false));	break;
			case OP_POP:	pop();					break;

			/* Read the given local slot from the current frame's
			 * 'slots' array. The slot is read relative to the beginning
			 * of that frame, and the level of indirection is calculated
			 * at compile time. */
			case OP_GET_LOCAL: {
				FN_UBYTE slot = READ_BYTE();
				push(frame->slots[slot]);
			} break;

			case OP_SET_LOCAL: {
				FN_UBYTE slot = READ_BYTE();
				frame->slots[slot] = peek(0);
			} break;

			case OP_DEFINE_GLOBAL: {
				// get the name of the variable from the constant pool.
				// Why not use 'Value val = pop()' instead? See NOTE.
				ObjString* name = READ_STRING();

				/* Take the value from the top of the stack and
				 * store it in a hash table with that name as the key.
				 * Throw an exception if variable have alredy been declared. */
				if (!tableSet(vm->globals, name, peek(0))) {
					runtimeError("Variable '%s' is already defined.",
														name->chars);
					return IR_RUNTIME_ERROR;
				}

				/* NOTE: we don't pop the value until *after* we add it to the
				 * hash table. This ensures the VM can still find the value if a
				 * GC is triggered right in the middle of adding it to the
				 * hash table.*/
				pop();
			} break;

			case OP_SET_GLOBAL: {
				// get the name of the variable from the constant pool
				ObjString* name = READ_STRING();

				/* Assign a value to an existing variable.
				 * If tableSet() returns TRUE means that instead of overwriting
				 * an existing variable we have created a new one.
				 * Thus, delete it and throw the runtime error, because current
				 * implementation doesn't support implicit variable declarations. */
				if (tableSet(vm->globals, name, peek(0))) {
					tableDelete(vm->globals, name);
					runtimeError("Undefined variable '%s'.", name->chars);
					return IR_RUNTIME_ERROR;
				}
				/* The pop() function is not called due to assignment is an expression,
				 * so it needs to leave that value on the stack in case the assignment
				 * is nested inside some larger expression. */
			} break;
			
			case OP_GET_GLOBAL: {
				// get the name of the variable from the constant pool
				ObjString* name = READ_STRING();
				Value value;

				/* Report a error if tableGet() can't find the given entry. */
				if (!tableGet(vm->globals, name, &value)) {
					runtimeError("Undefined variable '%s'.", name->chars);
					return IR_RUNTIME_ERROR;
				}
				push(value);
			} break;
			
			case OP_EQUAL: {
				Value b = pop();
				Value a = pop();
				push(BOOL_PACK(valuesEqual(a, b)));
			} break;
			
			case OP_GREATER:	BINARY_OP(BOOL_PACK, >);	break;
			case OP_LESS:		BINARY_OP(BOOL_PACK, <);	break;

			case OP_ADD: {
				if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
					concatenate();
				} else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
					FN_FLOAT b = NUMBER_UNPACK(pop());
					FN_FLOAT a = NUMBER_UNPACK(pop());
					push(NUMBER_PACK(a + b));
				} else {
					runtimeError("Operands must be two numbers or two strings.");
					return IR_RUNTIME_ERROR;
				}
			} break;
			
			case OP_SUBTRACT:	BINARY_OP(NUMBER_PACK, -);	break;
			case OP_MULTIPLY:	BINARY_OP(NUMBER_PACK, *);	break;
			case OP_DIVIDE:		BINARY_OP(NUMBER_PACK, /);	break;

			/* This code substitutes the sequence like this:
			 * push(BOOL_PACK(isFalsey(pop()))).
			 *
			 * Here we extract the value from the stack, processing it and
			 * pushing back onto the stack. Instead of juggling the Value
			 * back and forth to the stack, we just use more low-level 
			 * stack indexing operations, eliminating redundant code. */
			case OP_NOT: {
				vm->stackTop[-1] = BOOL_PACK(isFalsey(vm->stackTop[-1]));
			} break;

			/* vm->stackTop points to the next free block in the stack,
			 * while a value to be negated resides one step back. */
			case OP_NEGATE: {
				if (!IS_NUMBER(peek(0))) {
					runtimeError("Operand must be a number.");
					return IR_RUNTIME_ERROR;
				}

				FN_FLOAT temp = NUMBER_UNPACK(vm->stackTop[-1]);
				vm->stackTop[-1] = NUMBER_PACK(-temp);
			} break;

			case OP_PRINT: {
				printValue(pop());
			} break;
			
			case OP_PRINTLN: {
				printValue(pop());
				printf("\n");
			} break;

			case OP_JUMP: {
				FN_UWORD offset = READ_SHORT();
				frame->ip += offset;
			} break;

			case OP_JUMP_IF_FALSE: {
				FN_UWORD offset = READ_SHORT();

				/* Check the condition value which resides on top of the stack.
				 * Apply this jump offset to frame->ip if it's falsey. */
				if (isFalsey(peek(0)))
					frame->ip += offset;

			} break;

			case OP_LOOP: {
				FN_UWORD offset = READ_SHORT();
				frame->ip -= offset;
			} break;

			case OP_CALL: {
				
				/* Fetch the number of arguments from
				 * the operand of OP_CALL instruction. */
				FN_BYTE argCount = READ_BYTE();
				/* Use this number as an offset to the function being called. */
				Value callee = peek((argCount & 0x000000FF));

				/* Among other important things this function call does is
				 * incrementing vm->frameCount variable. */
				if (!callValue(callee, argCount)) {
					return IR_RUNTIME_ERROR;
				}

				/* On successful call of callValue(), there will be a new frame on the
				 * CallFrame stack for the called function.
				 * Since the bytecode dispatch loop reads from the 'frame' variable,
				 * when the VM goes to execute the next instruction, it will read the 'ip'
				 * from the newly called function's CallFrame and jump to its code. */
				frame = &vm->frames[vm->frameCount - 1];
			} break;

			case OP_RETURN: {

				/* We're about to discard the called function's entire stack window,
				 * so we pop that return value off and hand on to it. */
				Value result = pop();

				/* discard the CallFrame for the returning function. Previously
				 * it was incremented in call() function. */
				vm->frameCount--;

				/* Complete execution if the frame we have just returned from
				 * is the last one (i.e. the top-level script). */
				if (0 == vm->frameCount) {
					
					pop();	/* Pop the main script function from the stack. */
					return IR_OK;
				}

				/* Point the top of the stack to the beginning fo the returning
				 * function's stack window. This results in discarding all of
				 * the slots the callee was using for its params and locals. */
				vm->stackTop = frame->slots;

				/* Push the return value back onto the stack at the lower location. */
				push(result);

				/* Update the run() function's cached pointer to the current frame. */
				frame = &vm->frames[vm->frameCount - 1];
			} break;
		}
	}
}

InterpretResult
interpret(const char* source)
{
	/* Pass the source code to the compiler which in turn
	 * returns a new ObjFunction* containing the top-level code. */
	ObjFunction* function = compile(source);
	if (NULL == function)
		return IR_COMPILE_ERROR;

	/* Store the function on the stack. */
	push(OBJECT_PACK(function));

	/* Set up the first frame for executing the top-level function. */
	call(function, 0);

	return run();
}
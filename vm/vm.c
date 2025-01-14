#include <stdarg.h>
#include <time.h>
#include "vm.h"
#include "object.h"
#include "globals.h"

static CallFrame* frame;


static void
defineNative(const char* name, NativeFn function)
{
	push(OBJ_PACK(copyString(name, (int32_t)strlen(name))));
	push(OBJ_PACK(newNative(function)));
	tableSet(&vm.globals, STRING_UNPACK(vm.stack[0]), vm.stack[1]);
	pop();
	pop();
}

static Value
clockNative(int32_t argCount, Value* args)
{
	return NUM_PACK((float)clock() / CLOCKS_PER_SEC);
}

static void
resetStack(void)
{
	vm.stackTop = vm.stack;
	vm.stackStart = vm.stack - 1;
	vm.stackEnd = vm.stack + STACK_SIZE;
}

void
initVM(void)
{
	resetStack();
	initTable(&vm.strings);
	initTable(&vm.globals);
	defineNative("clock", clockNative);
}

void
freeVM(void)
{
	freeTable(&vm.strings);
	freeTable(&vm.globals);
	freeObjects();
}

static void
runtimeError(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	for (int32_t i = vm.frameCount - 1; i >= 0; --i) {
		
		CallFrame* frame  = &vm.frames[i];
		ObjFunction* func = frame->function;
		// size_t ins        = frame->ip - func->bCode.code - 1;
		
		fprintf(stderr, "[line %d] in ", 0);
		if (func->name == NULL) {
			fprintf(stderr, "script\n");
		} else {
			fprintf(stderr, "%s()\n", func->name->chars);
		}
	}

	resetStack();
}

void
push(Value value)
{
	if (vm.stackTop >= vm.stackEnd) {
		runtimeError("Stack overflow.");
		exit(1);
	}
	*vm.stackTop = value;
	vm.stackTop++;
}

Value
pop(void)
{
	vm.stackTop--;
	if (vm.stackTop <= vm.stackStart) {
		runtimeError("Stack underflow.");
		exit(1);
	}
	return *vm.stackTop;
}

void
popN(uint16_t count)
{
	vm.stackTop = vm.stackTop - count;
	if (vm.stackTop <= vm.stackStart) {
		runtimeError("Stack underflow.");
		exit(1);
	}
}

static Value
peek(int distance)
{
	return vm.stackTop[-1 - distance];
}


static bool
call(ObjFunction* function, uint8_t argCount)
{
	if (argCount != function->arity) {
		runtimeError("Expected %d arguments but got %d.",
					function->arity, argCount);
		return false;
	}

	if (vm.frameCount == FRAMES_MAX) {
		runtimeError("Exceeded the maximum depth of function calls.");
		return false;
	}

	CallFrame* frame = &vm.frames[vm.frameCount++];
	frame->function  = function;
	frame->ip        = function->bCode.code;
	// points to the window of this frame into the stack.
	frame->slots     = vm.stackTop - argCount - 1;
	return true;
}

static bool
callValue(Value callee, uint8_t argCount)
{
	if (IS_OBJ(callee)) {

		switch (OBJ_TYPE(callee)) {
			case obj_func: {
				bool result = call(FUNC_UNPACK(callee), argCount);
				return result;
			}
			case obj_native: {
				NativeFn native = NATIVE_UNPACK(callee);
				Value result    = native(argCount, vm.stackTop - argCount);
				vm.stackTop    -= argCount + 1;
				
				push(result);
				return true;
			}

			default:	// Non-callable object type
			break;
		}
	}
	runtimeError("Can only call functions and classes.");
	return false;
}

static bool
isFalsey(Value value)
{
	return IS_NULL(value) || (IS_BOOL(value) && !BOOL_UNPACK(value));
}

static void
concatenate(void)
{
	ObjString* b = STRING_UNPACK(pop());
	ObjString* a = STRING_UNPACK(pop());

	uint32_t len = a->len + b->len;
	char* chars = ALLOCATE(char, len + 1);
	memcpy(chars, a->chars, a->len);
	memcpy(chars + a->len, b->chars, b->len);
	chars[len] = '\0';

	ObjString* result = takeString(chars, len);
	push(OBJ_PACK(result));
}

/* Reads the byte currently pointed at by 'ip' and
 * then advances the instruction pointer. */
static inline uint8_t
readByteCode(void)
{
	return *frame->ip++;
}

static inline uint16_t
readShortCode(void)
{
	uint16_t idx1 = readByteCode();
	uint16_t idx2 = readByteCode();
	return ((idx1 << 8) | (idx2 & 0x00FF));
}

static Value
readConst(OpCode ins)
{
	uint16_t idx;
	if (ins == op_iconst)
		idx = readByteCode();
	else
		idx = readShortCode();

	return frame->function->bCode.constants.values[idx];
}

static uint16_t
readLocalVarOffset(OpCode ins)
{
	if (op_get_locvar || op_set_locvar)
		return readByteCode();
	else
		return readShortCode();
}

static ObjString*
readString(OpCode ins)
{
	Value str;
	if (ins == op_def_gvar || ins == op_get_gvar || ins == op_set_gvar)
		str = readConst(op_iconst);
	else
		str = readConst(op_iconstw);

	return STRING_UNPACK(str);
}

static bool
binaryOp(OpCode opType)
{
	if ((opType == op_add) && IS_STRING(peek(0)) && IS_STRING(peek(1))) {
		concatenate();
		return true;
	}
	else if (!IS_NUM(peek(0)) || !IS_NUM(peek(1))) {
		runtimeError("Operands must be numbers.");
		return false;
	}


	float b = NUM_UNPACK(pop());
	float a = NUM_UNPACK(pop());

	switch (opType) {
		case op_gt:  push(BOOL_PACK(a > b)); break;
		case op_lt:  push(BOOL_PACK(a < b)); break;
		case op_add: push(NUM_PACK(a + b));  break;
		case op_sub: push(NUM_PACK(a - b));  break;
		case op_mul: push(NUM_PACK(a * b));  break;
		case op_div: push(NUM_PACK(a / b));  break;
		default: return false;
	}
	return true;
}

static InterpretResult
run(void)
{
	frame = &vm.frames[vm.frameCount - 1];
	OpCode ins;
	while (true) {
		ins = readByteCode();
		switch (ins) {
			case op_iconst:
			case op_iconstw:
			{
				Value constant = readConst(ins);
				push(constant);
			} break;
			case op_null:  push(NULL_PACK);        break;
			case op_true:  push(BOOL_PACK(true));  break;
			case op_false: push(BOOL_PACK(false)); break;
			case op_eq:
			{
				Value b = pop();
				Value a = pop();
				push(BOOL_PACK(valuesEqual(a, b)));
			} break;
			case op_gt:
			case op_lt:
			case op_add:
			case op_sub:
			case op_mul:
			case op_div:
			{
				if(!binaryOp(ins))
					return INTERPRET_RUNTIME_ERROR;
			} break;
			case op_not:
			{
				push(BOOL_PACK(isFalsey(pop())));
			} break;
			case op_negate:
			{
				if (!IS_NUM(peek(0))) {
					runtimeError("Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}

				push(NUM_PACK(-NUM_UNPACK(pop())));
			} break;
			case op_print:
			{
				printValue(pop());
				printf("\n");
			} break;
			case op_pop:
			{
				pop();
			} break;
			case op_popn:
			{
				uint16_t count = readShortCode();
				popN(count);
			} break;
			case op_def_gvar:
			case op_def_gvarw:
			{
				ObjString* name = readString(ins);
				if (!tableSet(&vm.globals, name, peek(0))) {
					runtimeError("Global variable '%s' has already been defined.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				pop();
			} break;
			case op_get_gvar:
			case op_get_gvarw:
			{
				ObjString* name = readString(ins);
				Value value;
				if (!tableGet(&vm.globals, name, &value)) {
					runtimeError("Global variable '%s' is not defined.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
			} break;
			case op_set_gvar:
			case op_set_gvarw:
			{
				ObjString* name = readString(ins);
				if (tableSet(&vm.globals, name, peek(0))) {
					tableDelete(&vm.globals, name);
					runtimeError("Global variable '%s' is not defined.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
			} break;
			case op_get_locvar:
			case op_get_locvarw:
			{
				uint16_t slot = readLocalVarOffset(ins);
				push(frame->slots[slot]);
			}
			break;
			case op_set_locvar:
			case op_set_locvarw:
			{
				uint16_t slot = readLocalVarOffset(ins);
				frame->slots[slot] = peek(0);
			}
			break;
			case op_jmp_false:
			{
				uint16_t offset = readShortCode();
				if (isFalsey(peek(0))) {
					frame->ip += offset;
				}
			}
			break;
			case op_jmp:
			{
				uint16_t offset = readShortCode();
				frame->ip += offset;
			}
			break;
			case op_loop:
			{
				uint16_t offset = readShortCode();
				frame->ip -= offset;
			}
			break;
			case op_call:
			{
				uint8_t argCount = readByteCode();

				if (!callValue(peek(argCount), argCount)) {
					return INTERPRET_RUNTIME_ERROR;
				} else {
					frame = &vm.frames[vm.frameCount - 1];	// Update the current frame
				}
			} break;
			case op_ret:
			{
				Value retVal = pop();		// A value, returned by a function.
				vm.frameCount--;			// Discard the call frame for the returning function.
				if (vm.frameCount == 0) {	// Is this is the very last call frame?
					pop();					// If so, pop the main script from the stack
					return INTERPRET_OK;	// exit the interpreter.
				}

				vm.stackTop = frame->slots;	// Otherwise, discard callee's call frame by means of setting VM's
											// stack top at the beginning of the returning function's stack window.
				push(retVal);
				frame = &vm.frames[vm.frameCount - 1];	// Update current frame
			} break;
		}
	}
}

InterpretResult
interpret(ObjFunction* topLevel)
{
	push(OBJ_PACK(topLevel));
	call(topLevel, 0);			// Top level script has no arguments.
	return run();
}
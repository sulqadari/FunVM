#include <stdarg.h>
#include "vm.h"

static VM vm;

static void
resetStack(void)
{
	vm.stackTop = vm.stack;
}

static void
runtimeError(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);
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
	*vm.stackTop = value;
	vm.stackTop++;
}

Value
pop(void)
{
	vm.stackTop--;
	return *vm.stackTop;
}

static Value
peek(int distance)
{
	return vm.stackTop[-1 - distance];
}

static bool
isFalsey(Value value)
{
	return IS_NULL(value) || (IS_BOOL(value) && !BOOL_UNPACK(value));
}

/* Reads the byte currently pointed at by 'ip' and
 * then advances the instruction pointer. */
static uint8_t
readByteCode(void)
{
	return *vm.ip++;
}

static uint16_t
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

	return vm.bCode->constants.values[idx];
}

static ObjString*
readObjString(OpCode ins)
{
	uint16_t idx;

	if (ins == op_obj_str)
		idx = readByteCode();
	else
		idx = readShortCode();

	ObjString* str = (ObjString*)&vm.bCode->objects.values[idx];
	str->chars = (char*)&vm.bCode->objects.values[idx + sizeof(ObjString)];
	return str;
}

static bool
binaryOp(OpCode opType)
{
	if (!IS_NUM(peek(0)) || !IS_NUM(peek(1))) {
		runtimeError("Operands must be numbers.");
		return false;
	}

	i32 b = NUM_UNPACK(pop());
	i32 a = NUM_UNPACK(pop());

	switch (opType) {
		case op_add: push(NUM_PACK(a + b));  break;
		case op_sub: push(NUM_PACK(a - b));  break;
		case op_mul: push(NUM_PACK(a * b));  break;
		case op_div: push(NUM_PACK(a / b));  break;
		case op_gt:  push(BOOL_PACK(a > b)); break;
		case op_lt:  push(BOOL_PACK(a < b)); break;
		default: return false;
	}
	return true;
}

static InterpretResult
run(void)
{
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
			case op_obj_str:
			case op_obj_strw: {
				ObjString* str = readObjString(ins);
				push(OBJ_PACK(str));
			} break;
			case op_null:  push(NULL_PACK());      break;
			case op_true:  push(BOOL_PACK(true));  break;
			case op_false: push(BOOL_PACK(false)); break;
			case op_eq: {
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
			case op_not: {
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
			case op_ret:
			{
				printValue(pop());
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
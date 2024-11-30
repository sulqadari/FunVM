#include <stdarg.h>
#include "vm.h"
#include "const_pool.h"

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

static Value
readConst(void)
{
	uint8_t idx = readByteCode();
	return vm.bCode->constants.values[idx];
}

static Value
readConstLong(void)
{
	uint16_t idx1 = readByteCode();
	uint16_t idx2 = readByteCode();
	return vm.bCode->constants.values[(idx1 << 8) | idx2];
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
			{
				Value constant = readConst();
				push(constant);
			} break;
			case op_iconstw:
			{
				Value constant = readConstLong();
				push(constant);
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
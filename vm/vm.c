#include "vm.h"
#include "values.h"
#include <stdarg.h>

static VM vm;

/* Reads the byte currently pointed at by 'ip' and
 * then advances the instruction pointer. */
static uint8_t
readByteCode(void)
{
	return *vm.ip++;
}

static int32_t
readConst(void)
{
	uint8_t idx = readByteCode();
	return vm.bCode->constants.values[idx];
}

static int32_t
readConstLong(void)
{
	uint16_t idx1 = readByteCode();
	uint16_t idx2 = readByteCode();
	return vm.bCode->constants.values[(idx1 << 8) | idx2];
}

static void
binaryOp(OpCode opType)
{
	i32 b = pop();
	i32 a = pop();

	switch (opType) {
		case op_add: push(a + b); break;
		case op_sub: push(a - b); break;
		case op_mul: push(a * b); break;
		case op_div: push(a / b); break;
		case op_gt:  push(a > b); break;
		case op_lt:  push(a < b); break;
		default: break; // unreachable
	}
}

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

	resetStack();
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
push(i32 value)
{
	*vm.stackTop = value;
	vm.stackTop++;
}

i32
pop(void)
{
	vm.stackTop--;
	return *vm.stackTop;
}

static i32
peek(uint32_t distance)
{
	return vm.stackTop[-1 - distance];
}

static bool
isFalsey(i32 value)
{
	return value == 0;
}

static InterpretResult
run(void)
{
	uint8_t ins;
	while (true) {
		ins = readByteCode();
		switch (ins) {
			case op_iconst:
			case op_sconst:
			case op_bconst: {
				i32 constant = readConst();
				push(constant);
			} break;
			case op_iconst_long:
			case op_sconst_long:
			case op_bconst_long: {
				i32 constant = readConstLong();
				push(constant);
			} break;
			case op_null:  {
				push(null);
			} break;
			case op_true:  {
				push(True);
			} break;
			case op_false: {
				push(False);
			} break;
			case op_eq: {
				i32 b = pop();
				i32 a = pop();
				push(valuesEqual(a, b));
			} break;
			case op_gt: {
				binaryOp(op_gt);
			} break;
			case op_lt: {
				binaryOp(op_lt);
			} break;
			case op_add: {
				binaryOp(op_add);
			} break;
			case op_sub: {
				binaryOp(op_sub);
			} break;
			case op_mul: {
				binaryOp(op_mul);
			} break;
			case op_div: {
				binaryOp(op_div);
			} break;
			case op_not: {
				push(isFalsey(pop()));
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
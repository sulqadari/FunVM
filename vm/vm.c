#include "vm.h"
#include "values.h"
#include <stdarg.h>

static VM vm;

typedef struct {
	OpCode code;
	const char* str;
} OpStr;

OpStr opStrArray[] = {
	{op_iconst, "op_iconst"},
	{op_sconst, "op_sconst"},
	{op_bconst, "op_bconst"},
	{op_iconst_long, "op_iconst_long"},
	{op_sconst_long, "op_sconst_long"},
	{op_bconst_long, "op_bconst_long"},
	{op_null, "op_null"},
	{op_true, "op_true"},
	{op_false, "op_false"},
	{op_eq, "op_eq"},
	{op_neq, "op_neq"},
	{op_gt, "op_gt"},
	{op_gteq, "op_gteq"},
	{op_lt, "op_lt"},
	{op_lteq, "op_lteq"},
	{op_add, "op_add"},
	{op_sub, "op_sub"},
	{op_mul, "op_mul"},
	{op_div, "op_div"},
	{op_not, "op_not"},
	{op_negate, "op_negate"},
	{op_ret, "op_ret"},
};

static const char*
opToStr(OpCode opcode)
{
	return opStrArray[opcode].str;
}

// forward declaration.
static void runtimeError(const char* format, ...);

static InterpretResult
printOnReturn(OpCode previous)
{
	switch (previous) {
		case op_iconst:
		case op_sconst:
		case op_bconst:
		case op_iconst_long:
		case op_sconst_long:
		case op_bconst_long:
		case op_negate:      printValue(pop(), val_int);  break;
		case op_null:        printValue(pop(), val_null); break;
		case op_true:
		case op_false:
		case op_eq:
		case op_neq:
		case op_gt:
		case op_gteq:
		case op_lt:
		case op_lteq:        printValue(pop(), val_bool); break;
		case op_obj_long:
		case op_obj:         printValue(pop(), val_obj); break;
		default:
			runtimeError("unprintable opcode '%s'.", opToStr(previous));
		return INTERPRET_RUNTIME_ERROR;
	}
	printf("\n");
	return INTERPRET_OK;
}

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

static boolean
isFalsey(i32 value)
{
	if (((uint32_t)null == value) || (False == value)) {
		return True;
	}
	return False;
}

static InterpretResult
run(void)
{
	OpCode ins = 0;
	OpCode previous = 0;
	while (true) {
		previous = ins;
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
				push((uint32_t)null);
			} break;
			case op_true:  {
				push(True);
			} break;
			case op_false: {
				push(False);
			} break;
			case op_eq:
			case op_neq:
			case op_gt:
			case op_gteq:
			case op_lt:
			case op_lteq: {
				i32 b = pop();
				i32 a = pop();
				push(valuesEquality(a, b, ins));
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
			case op_obj: {
				i32 constant = readConst();
				push(constant);
			} break;
			case op_obj_long: {
				i32 constant = readConstLong();
				push(constant);
			} break;
			case op_ret: {
				return printOnReturn(previous);
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
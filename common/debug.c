#include "common.h"
#include "bytecode.h"

static void
parseOpcodes(ByteCode* bCode)
{
	char* opStr = NULL;
	OpCode opCode = 0;
	int32_t constVal = 0;

	for (uint32_t i = 0; i < bCode->count; ++i) {
		opCode = bCode->code[i];
		
		switch (opCode) {
			case op_iconst: opStr = "iconst"; break;
			case op_iconstw: opStr = "iconst_long"; break;
			case op_add: opStr = "add"; break;
			case op_sub: opStr = "sub"; break;
			case op_mul: opStr = "mul"; break;
			case op_div: opStr = "div"; break;
			case op_negate: opStr = "negate"; break;
			case op_ret: opStr = "ret"; break;
		}

		constVal = bCode->constants.values[bCode->code[i + 1]];
		printf("%s | %d\n", opStr, constVal);
	}
	printf("\n");
}

void
printByteCode(ByteCode* bCode)
{
	printf(
		"bytecode:\n"
		"\tcount: %02d\n"
		"\tcapacity: %02d\n",
		bCode->count, bCode->capacity
	);

	parseOpcodes(bCode);
}
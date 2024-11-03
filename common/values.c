#include "values.h"
#include "bytecode.h"
#include "object.h"

const boolean True  = 0xa5a5a5a5;
const boolean False = 0x5a5a5a5a;
const Null* null = (void*)0;

boolean
valuesEquality(i32 a, i32 b, uint8_t operation)
{
	switch (operation) {
		case op_eq:   return a == b ? True : False;
		case op_neq:  return a != b ? True : False;
		case op_gt:   return a  > b ? True : False;
		case op_gteq: return a >= b ? True : False;
		case op_lt:   return a  < b ? True : False;
		case op_lteq: return a <= b ? True : False;
		default: return -1; // unreachable
	}
}

void
printValue(i32 value, ValueType type)
{
	switch (type) {
		case val_bool:
			printf("%s", ((value == True) ? "true" : "false"));
		break;
		case val_null:
			printf("null");
		break;
		case val_int:
		case val_short:
		case val_byte:
			printf("%d", value);
		break;
		case val_obj:
			printObject(value);
		break;
	}
}
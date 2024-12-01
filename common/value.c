#include "value.h"
#include "object.h"

bool
valuesEqual(Value a, Value b)
{
	if (a.type != b.type)
		return false;
	
	switch (a.type) {
		case val_bool: return BOOL_UNPACK(a) == BOOL_UNPACK(b);
		case val_nil:  return true;
		case val_num:  return NUM_UNPACK(a) == NUM_UNPACK(b);
		case val_obj: {
			ObjString* aStr = STRING_UNPACK(a);
			ObjString* bStr = STRING_UNPACK(b);
			bool res = (aStr->len == bStr->len) && (memcmp(aStr->chars, bStr->chars, aStr->len) == 0);
			return res;
		} break;
		default: return false;
	}
}



void
printValue(Value value)
{
	switch(value.type) {
		case val_nil:
			printf("null");
		break;
		case val_bool:
			printf(BOOL_UNPACK(value) ? "true" : "false");
		break;
		case val_num:
			printf("%d", NUM_UNPACK(value));
		break;
		case val_obj:
			printObject(value);
		break;
	}
}
#include "value.h"

bool
valuesEqual(Value a, Value b)
{
	if (a.type != b.type)
		return false;
	
	switch (a.type) {
		case val_bool: return BOOL_UNPACK(a) == BOOL_UNPACK(b);
		case val_nil:  return true;
		case val_num:  return NUM_UNPACK(a) == NUM_UNPACK(b);
		default: return false;
	}
}
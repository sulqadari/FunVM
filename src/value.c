#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

void
initConstantPool(ConstantPool* constantPool)
{
	constantPool->count = 0;
	constantPool->capacity = 0;
	constantPool->pool = NULL;
}

void
freeConstantPool(ConstantPool* constantPool)
{
	FREE_ARRAY(Value, constantPool->pool, constantPool->capacity);
	initConstantPool(constantPool);
}

void
writeConstantPool(ConstantPool* constantPool, Value constant)
{
	/* Double constantPool array capacity if it doesn't have enough room. */
	if (constantPool->capacity < constantPool->count + 1) {
		uint16_t oldCapacity = constantPool->capacity;
		constantPool->capacity = INCREASE_CAPACITY(oldCapacity);
		constantPool->pool = INCREASE_ARRAY(Value, constantPool->pool,
										oldCapacity, constantPool->capacity);
	}

	constantPool->pool[constantPool->count] = constant;
	constantPool->count++;
}

void
printValue(Value value)
{
	switch (value.type) {
		case VAL_BOOL:		printf(BOOL_UNPACK(value) ? "true" : "false"); break;
		case VAL_NIL:		printf("nil"); break;
		case VAL_NUMBER:	printf("%g", NUMBER_UNPACK(value)); break;
		case VAL_OBJECT:	printObject(value); break;
	}
}

bool
valuesEqual(Value a, Value b)
{
	if (a.type != b.type)	/* The values are of different types (e.g. number and string) */
		return false;
	
	switch (a.type) {
		case VAL_BOOL:
			return BOOL_UNPACK(a) == BOOL_UNPACK(b);
		case VAL_NIL:
			return true;
		case VAL_NUMBER:
			return NUMBER_UNPACK(a) == NUMBER_UNPACK(b);
		case VAL_OBJECT:
			return OBJECT_UNPACK(a) == OBJECT_UNPACK(b);
		default: return false; // Unreachable.
	}
}
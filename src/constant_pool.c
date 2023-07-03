#include <stdio.h>

#include "memory.h"
#include "constant_pool.h"

void
initConstantPool(ConstantPool *constantPool)
{
	constantPool->count = 0;
	constantPool->capacity = 0;
	constantPool->pool = NULL;
}

void
freeConstantPool(ConstantPool *constantPool)
{
	FREE_ARRAY(Constant, constantPool->pool, constantPool->capacity);
	initConstantPool(constantPool);
}

void
writeConstantPool(ConstantPool *constantPool, Constant constant)
{
	/* Double constantPool array capacity if it doesn't have enough room. */
	if (constantPool->capacity < constantPool->count + 1) {
		uint32_t oldCapacity = constantPool->capacity;
		constantPool->capacity = INCREASE_CAPACITY(oldCapacity);
		constantPool->pool = INCREASE_ARRAY(Constant, constantPool->pool,
										oldCapacity, constantPool->capacity);
	}

	constantPool->pool[constantPool->count] = constant;
	constantPool->count++;
}

void
printValue(Constant value)
{
	printf("%g", value);
}
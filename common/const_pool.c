#include "memory.h"
#include "const_pool.h"

void
initConstPool(ConstPool* cPool)
{
	cPool->values = NULL;
	cPool->capacity = 0;
	cPool->count = 0;
}

void
freeConstPool(ConstPool* cPool)
{
	FREE_ARRAY(i32, cPool->values, cPool->capacity);
	initConstPool(cPool);
}

void
writeConstPool(ConstPool* cPool, i32 value)
{
	do {
		if (cPool->capacity >= cPool->count + 1)
			break;
		
		uint32_t oldCap = cPool->capacity;
		cPool->capacity = GROW_CAPACITY(oldCap);
		cPool->values = GROW_ARRAY(i32, cPool->values, oldCap, cPool->capacity);
	} while(0);

	cPool->values[cPool->count++] = value;
}
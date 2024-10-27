#ifndef FUNVM_CONST_POOL_H
#define FUNVM_CONST_POOL_H

#include "common.h"
#include "values.h"

typedef struct {
	uint32_t count;
	uint32_t capacity;
	i32* values;
} ConstPool;

bool valuesEqual(i32 a, i32 b);
void initConstPool(ConstPool* cPool);
void freeConstPool(ConstPool* cPool);
void writeConstPool(ConstPool* cPool, i32 value);

#if defined(FUNVM_ARCH_x64)
void printConstValue(i32 value);
#endif /* FUNVM_DEBUG */

#endif /* FUNVM_CONST_POOL_H */
#ifndef FUNVM_CONST_POOL_H
#define FUNVM_CONST_POOL_H

#include "common.h"

typedef int32_t i32;

typedef struct {
	uint32_t capacity;
	uint32_t count;
	i32* values;
} ConstPool;

void initConstPool(ConstPool* cPool);
void freeConstPool(ConstPool* cPool);
void writeConstPool(ConstPool* cPool, i32 value);

#if defined(FUNVM_ARCH_x64)
void printConstValue(i32 value);
#endif /* FUNVM_DEBUG */

#endif /* FUNVM_CONST_POOL_H */
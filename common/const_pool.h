#ifndef FUNVM_CONST_POOL_H
#define FUNVM_CONST_POOL_H

#include "common.h"
#include "value.h"

typedef struct {
	uint32_t count;
	uint32_t capacity;
	Value* values;
} ConstPool;

void initConstPool(ConstPool* cPool);
void freeConstPool(ConstPool* cPool);
void writeConstPool(ConstPool* cPool, Value value);

void printConstValue(Value value);

#endif /* FUNVM_CONST_POOL_H */
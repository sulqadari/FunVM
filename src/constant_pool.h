#ifndef FUNVM_VALUE_H
#define FUNVM_VALUE_H

#include "common.h"

typedef double Value;

typedef struct {
	uint32_t capacity;
	uint32_t count;
	Value *pool;
} ConstantPool;

void initConstantPool(ConstantPool *constantPool);
void writeConstantPool(ConstantPool *constantPool, Value constant);
void freeConstantPool(ConstantPool *constantPool);
void printValue(Value value);

#endif // !FUNVM_VALUE_H
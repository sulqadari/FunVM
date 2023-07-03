#ifndef FUNVM_VALUE_H
#define FUNVM_VALUE_H

#include "common.h"

typedef double Constant;

typedef struct {
	int32_t capacity;
	int32_t count;
	Constant *pool;
} ConstantPool;

void initConstantPool(ConstantPool *constantPool);
void writeConstantPool(ConstantPool *constantPool, Constant constant);
void freeConstantPool(ConstantPool *constantPool);
void printValue(Constant value);

#endif // !FUNVM_VALUE_H
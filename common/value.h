#ifndef FUNVM_VALUE_H
#define FUNVM_VALUE_H

#include "common.h"

typedef int32_t i32;

typedef struct {
	uint32_t capacity;
	uint32_t count;
	i32* values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, i32 value);
void freeValueArray(ValueArray* array);

#endif /* FUNVM_VALUE_H */
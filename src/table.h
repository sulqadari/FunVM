#ifndef FUNVM_TABLE_H
#define FUNVM_TABLE_H

#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "value.h"

typedef struct {
	ObjString *key;
	Value value;
} Entry;

typedef struct {
	int32_t count;
	int32_t capacity;
	Entry *entries;
} Table;

void initTable(Table *table);
void freeTable(Table *table);
bool tableSet(Table *table, ObjString *key, Value value);

#endif // !FUNVM_TABLE_H
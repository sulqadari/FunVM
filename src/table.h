#ifndef FUNVM_TABLE_H
#define FUNVM_TABLE_H

#include "common.h"
#include "value.h"

/* The Entry - a simple key:value pair. */
typedef struct {
	ObjString *key;
	Value value;
} Entry;

/* The HashTable - an array of entries. */
typedef struct {
	int32_t count;
	int32_t capacity;
	Entry *entries;
} Table;

void initTable(Table *table);
void freeTable(Table *table);

#endif // !FUNVM_TABLE_H
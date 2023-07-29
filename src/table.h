#ifndef FUNVM_TABLE_H
#define FUNVM_TABLE_H

#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "value.h"

/* The current implementation assumes that the 
 * key is always the string, but this restriction
 * might be loosened. */
typedef struct {
	ObjString *key;
	Value value;
} Entry;

/* The hash table is represented as an array of entries.
 * The ratio of count to capacity is exactly the load factor
 * of the hash table. */
typedef struct {
	int32_t count;
	int32_t capacity;
	Entry *entries;
} Table;

void initTable(Table *table);
void freeTable(Table *table);
bool tableGet(Table *table, ObjString *key, Value *value);
bool tableSet(Table *table, ObjString *key, Value value);
bool tableDelete(Table *table, ObjString *key);
void tableAddAll(Table *from, Table *to);

#endif // !FUNVM_TABLE_H
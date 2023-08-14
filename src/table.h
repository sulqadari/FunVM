#ifndef FUNVM_TABLE_H
#define FUNVM_TABLE_H

#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "value.h"
#include "object.h"

/* Since the key is always a string, we store the ObjString
 * pointer directly instead of wrapping it in a Value. */
typedef struct {
	ObjString *key;
	Value value;
} Bucket;

typedef struct {
	int32_t count;
	int32_t capacity;
	Bucket *buckets;
} Table;

void initTable(Table *table);
void freeTable(Table *table);
bool tableGet(Table *table, ObjString *key, Value *value);
bool tableSet(Table *table, ObjString *key, Value value);
bool tableDelete(Table *table, ObjString *key);
void tableAddAll(Table *from, Table *to);
ObjString* tableFindString(Table *table, const char *chars,
							const int32_t length, const uint32_t hash);

#endif // !FUNVM_TABLE_H
#ifndef FUNVM_TABLE_H
#define FUNVM_TABLE_H

#include <stdint.h>
#include <stdbool.h>

#include "value.h"
#include "object.h"

/* Since the key is always a string, we store the ObjString
 * pointer directly instead of wrapping it in a Value. */
typedef struct {
	ObjString* key;
	Value value;
} Bucket;

typedef struct Table {
	FN_UWORD count;
	FN_UWORD capacity;
	Bucket* buckets;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars,
							const FN_UWORD length, const FN_UWORD hash);
void tableRemoveWhite(Table* table);
void markTable(Table* table);
#endif // !FUNVM_TABLE_H
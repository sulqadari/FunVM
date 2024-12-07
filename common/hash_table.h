#ifndef FUNVM_HASH_TABLE_H
#define FUNVM_HASH_TABLE_H

#include "common.h"
#include "value.h"

typedef struct {
	ObjString* key;
	Value      value;
} Entry;

typedef struct {
	uint32_t count;
	uint32_t capacity;
	Entry*   entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, uint32_t length, uint32_t hash);

#endif /* FUNVM_HASH_TABLE_H */
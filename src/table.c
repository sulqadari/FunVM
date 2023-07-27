#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void
initTable(Table *table)
{
	table->count = 0;
	table->capacity = 0;
	table->entries = NULL;
}

void
freeTable(Table *table)
{
	FREE_ARRAY(Entry, table->entries, table->capacity);
	initTable(table);
}

/**
 * The core function of the hash table.
 * It takes a key and an array of buckets and figures sout which bucket
 * the entry belongs in.
 * This function is also where linear probing and collision handling come
 * into play.
 * Because the array is grow every time it gets close to being full,
 * we know there will always be empty bucket.
 * @returns Entry* a pointer to bucket or NULL.
 */
static Entry*
findEntry(Entry *entries, int32_t capacity, const ObjString *key)
{
	/* Map the key's hash code to an index within the array's bounds. */
	uint32_t index = key->hash % capacity;
	for (;;) {

		Entry *entry = &entries[index];
		if ((key == entry->key) || (NULL == entry->key))
			return entry;

		/* Well, seems like a collision accured. Let linear probing
		 * do its work. */
		index = (index + 1) % capacity;
	}
}

static void
adjustCapacity(Table *table, int32_t capacity)
{
	Entry *entries = ALLOCATE(Entry, capacity);

	for (int32_t i = 0; i < capacity; ++i) {
		entries[i].key = NULL;
		entries[i].value = NIL_PACK();
	}

	/* Simple realloc() and subsequent copying elements doesn't
	 * work for hash table because to choose the bucket for each
	 * entry, we take its hash key module the array size, which means
	 * that when the array size changes, entries may end up in
	 * different buckets. Thus, we have to rebuild the table
	 * from scratch. */
	for (int32_t i = 0; i < table->capacity; ++i) {
		
		Entry *entry = &table->entries[i];
		if (NULL == entry->key)
			continue;
		
		Entry *dest = findEntry(entries, capacity, entry->key);
		dest->key = entry->key;
		dest->value = entry->value;
	}
	
	FREE_ARRAY(Entry, table->entries, table->capacity);
	table->entries = entries;
	table->capacity = capacity;
}
/**
 * Adds the given key/value pair to the given hash table.
 * If an bucket for that key is already present, the new value
 * overwrites the old one.
 * @returns bool: TRUE if the given key is the new one. False if
 *			key is already present.
 */
bool
tableSet(Table *table, ObjString *key, Value value)
{
	if ((table->count + 1) > (table->capacity * TABLE_MAX_LOAD)) {
		int32_t capacity = INCREASE_CAPACITY(table->capacity);
		adjustCapacity(table, capacity);
	}

	Entry *bucket = findEntry(table->entries, table->capacity, key);
	
	bool isEmpty = (bucket->key == NULL);
	if (isEmpty)
		table->count++;
	
	bucket->key = key;
	bucket->value = value;

	return isEmpty;
}

/**
 * Used in method inheritance.
*/
void tableAddAll(Table *from, Table *to)
{
	for (int32_t i = 0; i < from->capacity; ++i) {

		Entry *entry = &from->entries[i];
		if (NULL != entry->key)
			tableSet(to, entry->key, entry->value);
	}	
}
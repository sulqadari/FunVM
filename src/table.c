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
 * It takes a key and an array of buckets and figures out which bucket
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
	Entry *tombstone = NULL;

	// for (;;) {

	// 	Entry *bucket = &entries[index];
	// 	/* BUGFIX: instead of comparing values at a given memory locations
	// 	 * pointed by 'bucket->key' and 'key' pointers, the values (addresses)
	// 	 * of pointers are being compared. This must be fixed.*/
	// 	if ((key == bucket->key) || (NULL == bucket->key))
	// 		return bucket;

	// 	/* Well, seems like a collision accured. Let linear probing
	// 	 * do its work. */
	// 	index = (index + 1) % capacity;
	// }

	for (;;) {

		Entry *bucket = &entries[index];
		if (key == bucket->key)
			return bucket;			// We found the key.
		
		if (NULL != bucket->key)
			goto _skip;

		if (IS_NIL(bucket->value))	// empty entry.
			return (tombstone != NULL) ? tombstone : bucket;

		if (NULL == tombstone)		// We found a tombstone.
			tombstone = bucket;
		
_skip:
		/* Well, seems like a collision accured. Let linear probing
		 * do its work. */
		index = (index + 1) % capacity;
	}

}

/**
 * Looks up an entry in the given table using a key.
 * @param Table*: hash table to look up in.
 * @param ObjString*: the key value used in matching
 * @param Value*: points to the resulting value
 * @returns bool: true if entry is found. False otherwise.
*/
bool
tableGet(Table *table, ObjString *key, Value *value)
{
	if (0 == table->count)
		return false;

	Entry *entry = findEntry(table->entries, table->capacity, key);
	if (NULL == entry->key)
		return false;

	*value = entry->value;
	return true;
}

static void
adjustCapacity(Table *table, int32_t capacity)
{
	/* Create a bucket array of size 'capacity'. */
	Entry *newTable = ALLOCATE(Entry, capacity);

	/* Initialize every element in hash table with NULL values. */
	for (int32_t i = 0; i < capacity; ++i) {
		newTable[i].key = NULL;
		newTable[i].value = NIL_PACK();
	}

	/* Simple realloc() and subsequent copying elements doesn't
	 * work for hash table because to choose the entry for each
	 * bucket we take its hash key module the array size, which means
	 * that when the array size changes, entries may end up in
	 * different buckets. Thus, we have to rebuild the table
	 * from scratch and rearrange positions of the entries in that
	 * new hash table. */
	for (int32_t i = 0; i < table->capacity; ++i) {
		
		/* Walk through the old array front to back, searching
		 * non-empty bucket. */
		Entry *bucket = &table->entries[i];
		if (NULL == bucket->key)
			continue;			/* Skip empty bucket. */
		
		/* Passing to findEntry() the below mentioned arguments
		 * results in storing in 'dest' variable a pointer to the index
		 * in newTable.*/
		Entry *dest = findEntry(newTable, capacity, bucket->key);
		
		/* Fill in the pointer to the index in newTable with the values
		 * pointed by 'bucket' which refers to the previous hash table entry. */
		dest->key = bucket->key;
		dest->value = bucket->value;
	}

	/* Release the memory for the old array. */
	FREE_ARRAY(Entry, table->entries, table->capacity);

	/* Store new array of buckets and its capacity in Table struct. */
	table->entries = newTable;
	table->capacity = capacity;
}

/**
 * Adds the given key/value pair to the given hash table.
 * If an bucket for that key is already present, the new value
 * overwrites the old one.
 * @returns bool: TRUE if the given key is the new one. False if
 * key is already present (only the value field will be updated).
 */
bool
tableSet(Table *table, ObjString *key, Value value)
{
	if ((table->count + 1) > (table->capacity * TABLE_MAX_LOAD)) {
		int32_t capacity = INCREASE_CAPACITY(table->capacity);
		adjustCapacity(table, capacity);
	}

	Entry *bucket = findEntry(table->entries, table->capacity, key);
	
	bool isEmpty = (NULL == bucket->key);
	if (isEmpty && IS_NIL(bucket->value))
		table->count++;
	
	bucket->key = key;
	bucket->value = value;

	return isEmpty;
}

bool tableDelete(Table *table, ObjString *key)
{
	if (0 == table->count)
		return false;
	
	Entry *entry = findEntry(table->entries, table->capacity, key);
	if (NULL == entry->key)
		return false;
	
	/* Place a tomstone in the entry. */
	entry->key = NULL;
	entry->value = BOOL_PACK(true);

	return true;
}

/**
 * Used in method inheritance.
*/
void
tableAddAll(Table *from, Table *to)
{
	for (int32_t i = 0; i < from->capacity; ++i) {

		Entry *bucket = &from->entries[i];
		if (NULL != bucket->key)
			tableSet(to, bucket->key, bucket->value);
	}	
}
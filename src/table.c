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

	for (;;) {

		Entry *bucket = &entries[index];
		if (key == bucket->key)
			return bucket;	// We've found the key we have been looking for.
		
		// The key isn't NULL and didn't pass the previous check point.
		// If this check returns "true" then the current bucket is occupied.
		// Skip it and iterate again.
		if (NULL != bucket->key)
			goto _skip;

		// The key is NULL, then it is likely it might be the tombstone.
		// Check the value, if it's NULL too, then return either current
		// (empty) bucket or the last encountered tombstone.
		if (IS_NIL(bucket->value))
			return (tombstone != NULL) ? tombstone : bucket;

		// Well, the key is NULL, but value isn't. This is the tombstone.
		if (NULL == tombstone)
			tombstone = bucket;		// Store it in local variable.

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

	/* WE don't copy the tombstones left unused (i.e. not substituted
	 * with new values), thus we need to recalculate the count since
	 * it may change during a resize.*/
	table->count = 0;

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
		/* Increment count each time we find a non-tombstone entry. */
		table->count++;
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
	
	/* Current tableDelete() implementation implies that count is no
	 * longer the number of entries in the hash table, it's the number
	 * of entries plus tombstones.  Thus, increment the count during
	 * insertion only if the new entry goes into an entirely nepty bucket. */
	bool isEmpty = ((NULL == bucket->key) && IS_NIL(bucket->value));

	/* Increase counter if and only if we occupy a brand new bucket, not one
	 * containing tombstone (where the only key has NULL value). */
	if (isEmpty)
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
	
	/* Place the entry with a tombstone. */
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

ObjString*
tableFindString(Table *table, const char *chars, int32_t length, uint32_t hash)
{
	if (0 == table->count)
		return NULL;
	
	uint32_t index = hash % table->capacity;
	for (;;) {
		Entry *entry = &table->entries[index];
		// if (NULL == entry->key) {
		// 	// Stop if we find an empty non-tombstone entry
		// 	if (IS_NIL(entry->value))
		// 		return NULL;
		// } else if (	entry->key->length == length &&
		// 			entry->key->hash == hash &&
		// 			memcmp(entry->key->chars, chars, length) == 0) {
		// 	// We found it
		// 	return entry->key;
		// }

		if ((entry->key->hash == hash) &&
			(entry->key->length == length) &&
			(memcmp(entry->key->chars, chars, length) == 0)) {
			// We found it
			return entry->key;
		}

		if (NULL != entry->key)
			goto _skip;
		
		if (IS_NIL(entry->value))
			return NULL;

_skip:
		index = (index + 1) % table->capacity;
	}
}
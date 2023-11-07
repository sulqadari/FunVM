#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

/* Used in table's load factor management.
 * The hash table's size shall be increased
 * when it becomes at least 75% full. */
#define TABLE_MAX_LOAD 0.75

void
initTable(Table** table)
{
	*table = ALLOCATE(Table, 1);
	(*table)->count = 0;
	(*table)->capacity = 0;
	(*table)->buckets = NULL;
}

void
freeTable(Table* table)
{
	FREE_ARRAY(Bucket, table->buckets, table->capacity);
	FREE(Table, table);
}

/**
 * The core function of the hash table.
 * It accepts an array of buckets and a key and figures out
 * which bucket the entry belongs in.
 * This function is also where linear probing and collision handling
 * come into play.
 * It's used both to lookup existing intries in the hash table and to
 * decide where to insert new ones.
 * 
 * The for loop is used to perform the linear probing, which
 * eventualy ends up with finding an empty bucket. It will never fall into
 * infinite loop because load factor of the hash table insists that there is
 * always be at least 25% of unused indexes in the array.
 * Thus, the assumption is we'll eventually hit an empty bucket.
 * 
*/
static Bucket*
findEntry(Bucket* buckets, FN_UWORD capacity, const ObjString* key)
{
	/* Use key's hash code module the array size to choose the bucket for the
	 * given entry. */
	FN_UWORD index = key->hash % capacity;
	Bucket* tombstone = NULL;

	for (;;) {
		Bucket* bucket = &buckets[index];

		if (bucket->key == NULL) {

			/* So, the key is NULL. If it contains no value, then return the bucket.
			 * Special case: if this is not the first iteration, i.e. bucket->key was NULL,
			 * but bucket->value contained a value, then we had been encountered a tombstone.
			 * Thus, return the latter. */
			if (IS_NIL(bucket->value)) {
				// Empty bucket.
				return tombstone != NULL ? tombstone : bucket;
			} else {
				// We found a tombstone.
				if (tombstone == NULL) tombstone = bucket;
			}
		
		  /* bucket->key wasn't NULL, but bucket->value contained a value (true).
		   * This is the fingerprint of tombstone. Store the first encountered one. */
		} else if (bucket->key == key) {
			/* we've found the entry, just return it. */
			return bucket;
		}
		/* Manage collisions by means of linear probing algorithm. */
		index = (index + 1) % capacity;
	}
}

/**
 * Search in hash table a value assosiated with the key.
 * @param Table*: hash table to search key-value pair into;
 * @param ObjString*: a key to perform a search;
 * @param Value*: points to the value being found
 * @return bool: true if value is found. False otherwise.
 * 
*/
bool
tableGet(Table* table, ObjString* key, Value* value)
{
	if (0 == table->count)
		return false;
	
	Bucket* bucket = findEntry(table->buckets, table->capacity, key);
	if (NULL == bucket->key)
		return false;
	
	*value = bucket->value;
	return true;
}

/**
 * Creates a bucket array with 'capacity' entries. After array allocation,
 * it initializes every element to be an empty bucket and then stores the array
 * in the hash table's main struct.
 * 
 * When this function is about to reallocate the existing hash table, it rebuilds
 * it from scratch and inserts entries under new indexes.
 * Note that we don't copy tombstones over.
*/
static void
adjustCapacity(Table* table, FN_UWORD capacity)
{
	/* Create new array. */
	Bucket* newTable = ALLOCATE(Bucket, capacity);

	/* Reset */
	for (FN_UWORD i = 0; i < capacity; ++i) {
		newTable[i].key = NULL;
		newTable[i].value = NIL_PACK();
	}

	/* During resizing we discard the tombstones and take into
	 * account only those buckets, containing active key:value pairs. */
	table->count = 0;
	for (FN_UWORD i = 0; i < table->capacity; ++i) {

		/* Get subsequent entry from the OLD array. */
		Bucket* oldBucket = &table->buckets[i];

		/* Just skip current iteration if the key in OLD entry is NULL. */
		if (NULL == oldBucket->key)
			continue;

		/* Otherwise, calculate the index for the key:value in the NEW Table. */
		Bucket* newBucket = findEntry(newTable, capacity, oldBucket->key);

		/* perform the assignment. */
		newBucket->key = oldBucket->key;
		newBucket->value = oldBucket->value;
		table->count++;
	}

	FREE_ARRAY(Bucket, table->buckets, table->capacity);
	table->buckets = newTable;
	table->capacity = capacity;
}

/**
 * Insert a key/value pair into hash table.
 * If an entry for the given key is already present,
 * the new value overwrites previous one.
 * @return bool - true if a new entry was added. False otherwise, i.e.
 * key is already present and the only thing is done - its value has
 * been overwritten.
*/
bool
tableSet(Table* table, ObjString* key, Value value)
{
	if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
		FN_UWORD capacity = INCREASE_CAPACITY(table->capacity);
		adjustCapacity(table, capacity);
	}

	Bucket* bucket = findEntry(table->buckets, table->capacity, key);

	/* Increment the count only if the new [key:value] entry goes
	 * into an entirely empty bucket. */
	bool isNewKey = ((NULL == bucket->key) && IS_NIL(bucket->value));
	if (isNewKey)
		table->count++;

	bucket->key = key;
	bucket->value = value;

	return isNewKey;
}

/**
 * Deletes the key-value pair from the given hash table.
 * Instead of actual removing an entry, this function simply
 * substitutes its value with the 'tombstone'.
*/
bool
tableDelete(Table* table, ObjString* key)
{
	if (0 == table->count)
		return false;
	
	/* Find the entry to delete. */
	Bucket* bucket = findEntry(table->buckets, table->capacity, key);
	if (NULL == bucket->key)
		return false;
	
	/* Replace the entry with the tombstone. */
	bucket->key = NULL;
	bucket->value = BOOL_PACK(true);
	return true;
}

void
tableAddAll(Table* from, Table* to)
{
	for (FN_UWORD i = 0; i < from->capacity; ++i) {

		Bucket* bucket = &from->buckets[i];
		if (NULL != bucket->key) {
			tableSet(to, bucket->key, bucket->value);
		}
	}
}

ObjString*
tableFindString(Table* table, const char* chars,
				const FN_UWORD length, const FN_UWORD hash)
{
	if (0 == table->count)
		return NULL;
	
	FN_UWORD index = hash % table->capacity;

	for (;;) {
		Bucket* bucket = &table->buckets[index];

		if (NULL == bucket->key) {
			/* Stop if an empty non-tombstone entry is found. */
			if (IS_NIL(bucket->value))
					return NULL;
		}
		else if ((length == bucket->key->length) &&
				 (hash == bucket->key->hash) &&
				 (memcmp(bucket->key->chars, chars, length) == 0)) {
			return bucket->key;
		}

		index = (index + 1) % table->capacity;
	}
}

void
markTable(Table* table)
{
	for (FN_UWORD i = 0; i < table->capacity; ++i) {
		Bucket* bucket = &table->buckets[i];
		markObject((Object*)bucket->key);
		markValue(bucket->value);
	}
}
#include "hash_table.h"
#include "memory.h"
#include "object.h"

#define TABLE_MAX_LOAD 0.75	/*<! Hash Table's load factor. */

void
initTable(Table* table)
{
	table->count = 0;
	table->capacity = 0;
	table->entries = NULL;
}

void
freeTable(Table* table)
{
	FREE_ARRAY(Entry, table->entries, table->capacity);
	initTable(table);
}

static Entry*
findEntry(Entry* entries, uint32_t capacity, ObjString* key)
{
	/* The tombstone - is a bucket which was deleted previously, and used
	 * just to support 'open addressing' anticollision mechanism. When we find
	 * one, we should return it to the user. This way we reduce the memory usage.*/
	Entry* tombstone = NULL;

	// Use modulo to map hash code to an index within entries[] array.
	uint32_t index = key->hash % capacity;

	for (;;) {
		Entry* entry = &entries[index];

		// If key is null, then the bucket is empty, i.e.:
		// 1. If this is look up in the hash table, this means it isn't there.
		// 2. If this is inserting, it means we've found a place to add the new entry.
		if (entry->key == NULL) {
			
			if (IS_NULL(entry->value)) {
				// Recycle the tombstone's bucket instead of the next empty one
				return tombstone != NULL ? tombstone : entry;
			} else {
				if (tombstone == NULL) // we found a tombstone.
					tombstone = entry; // keep it.
			}

		// If the key in the bucket is equal to the key user looking for, then that key is already present, i.e.:
		// 1. If this is look up, then the bucket is found.
		// 2. If this is inserting, then we'll be replacing the value of the key, instead of adding a new entry.
		} else if (entry->key == key) {
			return entry; // we found the key
		}

		// If we've reached this point, that mean that the bucket at 'entries[index]' has an entry, but with a different key.
		// Proceed to addressing of collision.
		index = (index + 1) % capacity;
	}
}

/**
 * @param Table* a table to search in.
 * @param ObjString* a key we're looking for.
 * @param Value* pointer to a value, associated with the key, to be returned.
 */
bool
tableGet(Table* table, ObjString* key, Value* value)
{
	if (table->count == 0)
		return false;
	
	Entry* entry = findEntry(table->entries, table->capacity, key);
	if (entry->key == NULL)
		return false;
	
	*value = entry->value;
	return true;
}

static void
adjustCapacity(Table* table, uint32_t capacity)
{
	Entry* entries = ALLOCATE(Entry, capacity);
	for (uint32_t i = 0; i < capacity; ++i) {
		entries[i].key = NULL;
		entries[i].value = NULL_PACK;
	}

	// This field counts tombstones too, but in resized array we don't need to take
	// them into account anymore. Thus, reset its value.
	table->count = 0;

	// When the array size changes, entries may end up in the different buckets.
	// Thus, walk through the old array and recalculate their indexes.
	for (uint32_t i = 0; i < table->capacity; ++i) {
		
		// get subsequent entry.
		Entry* entry = &table->entries[i];
		if (entry == NULL)
			continue;		// nothing to do with empty bucket.
		
		// Fetch the reference to an appropriate place in the new array. 
		Entry* dest = findEntry(entries, capacity, entry->key);
		
		// insert the entry from the old table into the new array.
		dest->key = entry->key;
		dest->value = entry->value;
		table->count++;
	}

	// Release the memory for the old array.
	FREE_ARRAY(Entry, table->entries, table->capacity);
	table->entries = entries;
	table->capacity = capacity;
}

/**
 * Adds the given key/value pair to the given hash table. If an entry for that key
 * is already present, the new value overwrites the old one.
 * @returns bool: true if a new entry was added.
 */
bool
tableSet(Table* table, ObjString* key, Value value)
{
	if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
		uint32_t capacity = GROW_CAPACITY(table->capacity);
		adjustCapacity(table, capacity);
	}

	Entry* entry  = findEntry(table->entries, table->capacity, key);
	
	// A reference returned by findEntry() may be null, which means the 'key'
	// doesn't present in the table, i.e. a key which was passed over as an argument, is brand new one.
	bool isNewKey = entry->key == NULL;

	// Increment only if the new entry goes into an entirely empty bucket (not into tombstone).
	if (isNewKey && IS_NULL(entry->value))
		table->count++;
	
	// The 'entry' points to an entity inside the 'table->entries'.
	// The first call assigns to this field a value carried by the 'key' argument.
	// Subsequent calls with the same 'key' will have no effect on this field.
	entry->key = key;

	// Might be updated everytime this function is called.
	entry->value = value;

	return isNewKey;
}

bool
tableDelete(Table* table, ObjString* key)
{
	if (table->count == 0)
		return false;
	
	Entry* entry = findEntry(table->entries, table->capacity, key);

	if (entry->key == NULL)
		return false;
	
	entry->key = NULL;
	entry->value = BOOL_PACK(true); // this is the 'tombstone' sentinel
	return true;
}

/**
 * Copies all of the entries of one hash table into another. Primarily used
 * to support method inheritance.
 */
void
tableAddAll(Table* from, Table* to)
{
	for (uint32_t i = 0; i < from->capacity; ++i) {
		Entry* entry = &from->entries[i];
		if (entry->key != NULL) {
			tableSet(to, entry->key, entry->value);
		}

	}
}

ObjString*
tableFindString(Table* table, const char* chars, uint32_t length, uint32_t hash)
{
	if (table->count == 0)
		return NULL;
	
	uint32_t index = hash % table->capacity;
	for (;;) {

		Entry* entry = &table->entries[index];
		if (entry->key == NULL) {
			// Stop if we find an empty non-tombstone entry.
			if (IS_NULL(entry->value))
				return NULL;
			
		} else if (entry->key->len == length 
				&& entry->key->hash == hash
				&& memcmp(entry->key->chars, chars, length) == 0)
		{	// We found it.
			return entry->key;
		}

		index = (index + 1) % table->capacity;
	}
}
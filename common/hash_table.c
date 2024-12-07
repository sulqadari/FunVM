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
	uint32_t index = key->hash % capacity;
	Entry* tombstone = NULL;

	for (;;) {
		Entry* entry = &entries[index];

		if (entry->key == NULL) {
			if (IS_NULL(entry->value)) {
				// Recycle the tombstone's bucket instead of the next empty one
				return tombstone != NULL ? tombstone : entry;
			} else {
				if (tombstone == NULL) // we found a tombstone.
					tombstone = entry; // keep it.
			}
		} else if (entry->key == key) {
			return entry; // we found the key
		}

		index = (index + 1) % capacity;
	}
}

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
		entries[i].value = NULL_PACK();
	}

	table->count = 0;
	/* Rearrange key/value entries in accordance with the new capacity of the table. */
	for (uint32_t i = 0; i < table->capacity; ++i) {
		// get subsequent entry.
		Entry* entry = &table->entries[i];
		if (entry == NULL)
			continue;
		
		// Get the reference to an appropriate place in hash table being resized.
		Entry* dest = findEntry(entries, capacity, entry->key);
		
		// insert subsequent entry from the old table into the new array.
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
	bool isNewKey = entry->key == NULL;

	// Increment only if the new entry goes into an entirely empty bucket (not into tombstone).
	if (isNewKey && IS_NULL(entry->value))
		table->count++;
	
	entry->key   = key;
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
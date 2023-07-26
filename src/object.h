#ifndef FUNVM_OBJECT_H
#define FUNVM_OBJECT_H

#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "value.h"
#include "vm.h"

/* Helper macro to obtain Object's type. */
#define OBJECT_TYPE(value)		(OBJECT_UNPACK(value)->type)

/* Safety check macro. */
#define IS_STRING(value)		isObjType(value, OBJ_STRING)

/* Returns the (ObjString*) from the heap. */
#define STRING_UNPACK(value)	((ObjString*)OBJECT_UNPACK(value))

/* Returns the (ObjString*).chars from the heap. */
#define CSTRING_UNPACK(value)	(((ObjString*)OBJECT_UNPACK(value))->chars)

typedef enum {
	OBJ_STRING,
} ObjType;

/**
 * The root object of the overall object's hierarchy.
 * 
 * Each object type shall declare this struct as the very first
 * field, making it possible to use a 'type punning' pattern: a king of
 * single-inheritance in object-oriented languages.
 * Providing that, it's possible to convert, say, a pointer to ObjString
 * to Object and get access to its fields.
 * 
 * Any code that wants to work with all objects can treat them as base Object*
 * and ignore any other fields that may happen to follow.
 * 
 * Object's implementation differs from Value one in that, there is no
 * single structure gathering all types in union. Instead, each type
 * has its own structure with the 'Object object' as the first field.
*/
struct Object {
	ObjType type;
	struct Object *next;
};

/* String object aimed to keep payload. */
struct ObjString {
	Object object;
	int32_t length;
	char *chars;
	
	/* The cached hash code of the 'chars'.
	 * Used in hash table to avoid redundant hash computing every time
	 * we lookig for a given key. */
	uint32_t hash;
};

void initHeap(VM *_vm);
ObjString* takeString(char *chars, int32_t length);
ObjString* copyString(const char *chars, int32_t length);
void printObject(Value value);

/** Safety check before downcasting from 'Object*' to one of the child object type.
 *
 * The reason for this kind of checks is that, the stack keeps data
 * as 'Value' type which in case of, say, 'ObjString' expands to 'Value::Object*', i.e.,
 * before any kind of object to be inserted to the stack, it have to be 
 * "upcasted" to 'Object*'.
 * Thus, to handle 'Object*' as an 'ObjString*' we have to downcast it and be sure,
 * that the target entiry is exactly of type 'ObjString*'. */
static inline bool
isObjType(Value value, ObjType type)
{
	return IS_OBJECT(value) && OBJECT_UNPACK(value)->type == type;
}

#endif // !FUNVM_OBJECT_H
#ifndef FUNVM_OBJECT_H
#define FUNVM_OBJECT_H

#include "common.h"
#include "constant_pool.h"
#include <stddef.h>
#include <stdint.h>

/* Helper macro to obtain Object's type. */
#define OBJECT_TYPE(value)		(OBJECT_UNPACK(value)->type)

/* Safety check macro for downcasting from 'Object*' to 'ObjString*'. */
#define IS_STRING(value)	isObjType(value, OBJ_STRING)

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
 */
struct Object {
	ObjType type;
};

struct ObjString {
	Object object;
	int32_t length;
	char *chars;
};

ObjString* takeString(char *chars, int32_t length);
ObjString* copyString(const char *chars, int32_t length);
void printObject(Value value);

static inline bool
isObjType(Value value, ObjType type)
{
	return IS_OBJECT(value) && OBJECT_UNPACK(value)->type == type;
}

#endif // !FUNVM_OBJECT_H
#ifndef FUNVM_OBJECT_H
#define FUNVM_OBJECT_H

#include "common.h"
#include "constant_pool.h"
#include <stddef.h>

/* Helper macro to obtain Obj's type. */
#define OBJ_TYPE(value)		(OBJ_UNPACK(value)->type)

/* Downcast from 'Obj*' to 'ObjString*' safety check macro. */
#define IS_STRING(value)	isObjType(value, OBJ_STRING)

/* Returns the (ObjString*) from the heap. */
#define STRING_UNPACK(value)	((ObjString*)OBJ_UNPACK(value))
/* Returns the (ObjString*).chars from the heap. */
#define CSTRING_UNPACK(value)	(((ObjString*)OBJ_UNPACK(value))->chars)

typedef enum {
	OBJ_STRING,
} ObjType;

struct Obj {
	ObjType type;
};

struct ObjString {
	Obj obj;
	int32_t length;
	char *chars;
};

ObjString* copyString(const char *chars, int32_t length);
void printObject(Value value);

static inline bool
isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && OBJ_UNPACK(value)->type == type;
}

#endif // !FUNVM_OBJECT_H
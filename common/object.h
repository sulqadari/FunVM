#ifndef FUNVM_OBJECT_H
#define FUNVM_OBJECT_H

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value)        (OBJ_UNPACK(value)->type)
#define IS_STRING(value)       isObjType(value, obj_string);
#define STRING_UNPACK(value)   ((ObjString*)OBJ_UNPACK(value))
#define CSTRING_UNPACK(value)  (((ObjString*)OBJ_UNPACK(value))->chars)

typedef enum {
	obj_string
} ObjType;

struct Obj {
	ObjType type;
};

struct ObjString {
	Obj         obj;
	uint32_t    len;
	const char* chars;
};

ObjString* copyString(const char* chars, uint32_t length);
void printObject(Value value);

static inline bool
isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && OBJ_UNPACK(value)->type == type;
}

#endif /* FUNVM_OBJECT_H */
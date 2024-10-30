#ifndef FUNVM_OBJECT_H
#define FUNVM_OBJECT_H

#include "common.h"
#include "values.h"

typedef enum {
	obj_string
} ObjType;

struct Object {
	ObjType type;
};

struct ObjString {
  Object obj;
  uint32_t length;
  char* chars;
};

#define OBJ_TYPE(value)		(OBJ_UNPACK(value)->type)
#define IS_STRING(value)	isObjType(value, obj_string)

static inline bool
isObjType(Value value, ObjType type)
{
	return IS_OBJECT(value) && OBJ_UNPACK(value)->type == type;
}

#endif /* FUNVM_OBJECT_H */
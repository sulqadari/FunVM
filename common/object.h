#ifndef FUNVM_OBJECT_H
#define FUNVM_OBJECT_H

#include "common.h"
#include "values.h"

typedef enum {
	obj_raw,
	obj_string,
	obj_array
} ObjType;

struct Object {
	ObjType type;
};

struct ObjString {
  Object obj;
  uint32_t length;
  char* chars;
};

struct ObjArray {
  Object obj;
  uint32_t length;
  void* array;
};

#define IS_OBJECT(value)	((value).type == obj_raw)
#define IS_STRING(value)	((value).type == obj_string)
#define IS_ARRAY(value)		((value).type == obj_array)

ObjString* copyString(const char* chars, uint32_t length);

#endif /* FUNVM_OBJECT_H */
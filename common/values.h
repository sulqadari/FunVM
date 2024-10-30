#ifndef FUNVM_VALUES_H
#define FUNVM_VALUES_H

#include "common.h"

typedef int32_t i32;
typedef int32_t i16;
typedef int32_t i8;
typedef int32_t boolean;
typedef int32_t Null;
typedef struct Object Object;
typedef struct ObjString ObjString;

extern const boolean True;
extern const boolean False;
extern const Null null;

typedef enum {
	val_obj
} ValueType;

typedef struct {
	ValueType type;
	union 
	{
		Object* obj;
	} as;	
} Value;

#define IS_OBJECT(value)	((value).type == val_obj)
#define OBJ_UNPACK(value)	((value).as.obj)
#define OBJ_PACK(value)		((Value){VAL_OBJ, {.obj = (Obj*)object}})

#endif /* FUNVM_VALUES_H */
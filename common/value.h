#ifndef FUNVM_VALUES_H
#define FUNVM_VALUES_H

#include "common.h"

typedef int32_t i32;
typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
	val_nil,
	val_bool,
	val_num,
	val_obj		/* The payload of this type is stored on the heap. */
} ValueType;

typedef struct {
	ValueType type;
	union {
		bool boolean;
		i32 number;
		Obj* obj;
	} as;
} Value;

// Original naming is: BOOL_VAL, NIL_VAL and NUMBER_VAL
#define BOOL_PACK(value) ((Value){val_bool, {.boolean = value}})
#define NULL_PACK()      ((Value){val_nil,  {.number  = 0}})
#define NUM_PACK(value)  ((Value){val_num,  {.number  = value}})
#define OBJ_PACK(value)  ((Value){val_obj,  {.obj  = (Obj*)value}})

// Original naming: AS_BOOL, AS_NUMBER
#define BOOL_UNPACK(value) ((value).as.boolean)
#define NUM_UNPACK(value)  ((value).as.number)
#define OBJ_UNPACK(value)  ((value).as.obj)

#define IS_BOOL(value) ((value).type == val_bool)
#define IS_NULL(value) ((value).type == val_nil)
#define IS_NUM(value)  ((value).type == val_num)
#define IS_OBJ(value)  ((value).type == val_obj)

bool valuesEqual(Value a, Value b);
void printValue(Value value);

#endif /* FUNVM_VALUES_H */
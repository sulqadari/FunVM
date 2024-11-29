#ifndef FUNVM_VALUES_H
#define FUNVM_VALUES_H

#include "common.h"

typedef int32_t i32;

typedef enum {
	val_nil,
	val_bool,
	val_num
} ValueType;

typedef struct {
	ValueType type;
	union {
		bool boolean;
		i32 number;
	} as;
} Value;

// Original naming is: BOOL_VAL, NIL_VAL and NUMBER_VAL
#define BOOL_PACK(value) ((Value){val_bool, {.boolean = value}})
#define NULL_PACK()      ((Value){val_nil,  {.number  = 0}})
#define NUM_PACK(value)  ((Value){val_num,  {.number  = value}})

// Original naming: AS_BOOL, AS_NUMBER
#define BOOL_UNPACK(value) ((value).as.boolean)
#define NUM_UNPACK(value)  ((value).as.number)

#define IS_BOOL(value) ((value).type == val_bool)
#define IS_NULL(value) ((value).type == val_nil)
#define IS_NUM(value) ((value).type == val_num)


#endif /* FUNVM_VALUES_H */
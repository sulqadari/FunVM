#ifndef FUNVM_VALUES_H
#define FUNVM_VALUES_H

#include "common.h"

typedef uint32_t u32;
typedef int32_t  i32;
typedef int32_t  i16;
typedef int32_t  i8;
typedef int32_t  boolean;
typedef int32_t  Null;
typedef struct Object Object;
typedef struct ObjString ObjString;
typedef struct ObjArray ObjArray;

typedef enum {
	val_bool,
	val_null,
	val_int,
	val_short,
	val_byte,
	val_obj,
} ValueType;

extern const boolean True;
extern const boolean False;
extern const Null* null;

boolean valuesEquality(i32 a, i32 b, uint8_t operation);
void printValue(i32 value, ValueType type);

#endif /* FUNVM_VALUES_H */
#ifndef FUNVM_VALUES_H
#define FUNVM_VALUES_H

#include "common.h"


typedef int32_t i32;
typedef int32_t i16;
typedef int32_t i8;
typedef int32_t boolean;
typedef int32_t Null;

extern const boolean True;
extern const boolean False;
extern const Null null;

typedef enum {
	val_bool,
	val_null,
	val_i32,
} ValueType;

typedef struct {
	ValueType type;
	union 
	{
		i32     _int;
		i16     _short;
		i8      _byte;
		boolean _bool;
		Null    _null;
	} as;
	
} Value;

#endif /* FUNVM_VALUES_H */
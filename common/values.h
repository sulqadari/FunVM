#ifndef FUNVM_VALUES_H
#define FUNVM_VALUES_H

#include "common.h"

typedef enum {
	val_bool,
	val_null,
	val_i32,
} ValueType;

typedef int32_t i32;
#endif /* FUNVM_VALUES_H */
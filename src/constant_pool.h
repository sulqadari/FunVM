#ifndef FUNVM_VALUE_H
#define FUNVM_VALUE_H

#include "common.h"

typedef enum {
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER
} ValueType;

/** Memory layout prepresentation is as follows:
 * ___________________type
 * |	  |
 * |	  |___________padding
 * |	  ||	  |
 * |	  ||	  |_________________as
 * |	  ||	  ||______________|
 * |	  ||	  ||__bool		  |
 * |	  ||	  || _____double__|
 * |	  ||	  || |			  |
 * [][][][][][][][][][][][][][][][]
 */
typedef struct {
	ValueType type;
	union {
		bool boolean;
		double number;
	} as;
} Value;

#define IS_BOOL(value)		((value).type == VAL_BOOL)
#define IS_NIL(value)		((value).type == VAL_NIL)
#define IS_NUMBER(value)	((value).type == VAL_NUMBER)

#define BOOL_PACK(value)	((Value){VAL_BOOL,	 {.boolean = value}})
#define NIL_PACK			((Value){VAL_NIL,	 {.number = 0}})
#define NUMBER_PACK(value)	((Value){VAL_NUMBER, {.number = value}})

#define BOOL_UNPACK(value)		((value).as.boolean)
#define NUMBER_UNPACK(value)	((value).as.number)

typedef struct {
	uint32_t capacity;
	uint32_t count;
	Value *pool;
} ConstantPool;

void initConstantPool(ConstantPool *constantPool);
void writeConstantPool(ConstantPool *constantPool, Value constant);
void freeConstantPool(ConstantPool *constantPool);
void printValue(Value value);

#endif // !FUNVM_VALUE_H
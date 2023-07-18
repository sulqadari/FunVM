#ifndef FUNVM_VALUE_H
#define FUNVM_VALUE_H

#include "common.h"

/** Forward declaration. */
typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
	VAL_OBJ
} ValueType;

/** Memory layout prepresentation is as follows:
 *   ___________________type (4 bytes)
 *  |	     |
 *  |	     |   ___________padding (4 bytes)
 *  |	     |  |	     |
 *  |	     |  |	     |   
 *  |	     |  |	     |   _________as_________
 *  |	     |  |	     |  |_bool		  		 |
 *  |	     |  |	     |  |   _____double______|
 *  |	     |  |	     |  |  |			     |
 * [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
 */
typedef struct {
	ValueType type;
	union {
		bool boolean;
		double number;
		Obj *obj;
	} as;
} Value;

#define IS_BOOL(value)		((value).type == VAL_BOOL)
#define IS_NIL(value)		((value).type == VAL_NIL)
#define IS_NUMBER(value)	((value).type == VAL_NUMBER)
#define IS_OBJ(value)		((value).type == VAL_OBJ)

#define BOOL_PACK(value)	((Value){VAL_BOOL,	 {.boolean = value}})
#define NIL_PACK			((Value){VAL_NIL,	 {.number = 0}})
#define NUMBER_PACK(value)	((Value){VAL_NUMBER, {.number = value}})
#define OBJ_PACK(value)		((Value){VAL_OBJ, {.obj = (Obj*)object}})

#define BOOL_UNPACK(value)		((value).as.boolean)
#define NUMBER_UNPACK(value)	((value).as.number)
#define OBJ_UNPACK(value)		((value).as.obj)

typedef struct {
	uint32_t capacity;
	uint32_t count;
	Value *pool;
} ConstantPool;

bool valuesEqual(Value a, Value b);
void initConstantPool(ConstantPool *constantPool);
void writeConstantPool(ConstantPool *constantPool, Value constant);
void freeConstantPool(ConstantPool *constantPool);
void printValue(Value value);

#endif // !FUNVM_VALUE_H
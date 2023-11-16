#ifndef FUNVM_VALUE_H
#define FUNVM_VALUE_H

#include "common.h"

/** Forward declaration. */
typedef struct Object Object;
typedef struct ObjString ObjString;

/* Value's type enumeration.
 * This is the VM's notion of "type", not the user's. */
typedef enum {
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
	VAL_OBJECT		/* Heap-allocated types. */
} ValueType;

/**
 * Value representation using 'tagged union'.
 * This structure contains two parts: a type "tag", and a payload
 * for the actual value.
 *  
 * Memory layout prepresentation in 64-bit architecture is as follows:
 *   ___________type (4 bytes)
 *  |	     |
 *  |	     |   ___________padding (4 bytes)
 *  |	     |  |	     |   
 *  |	     |  |	     |   _______________________as (8 bytes)
 *  |	     |  |	     |  |__bool		  		 |
 *  |	     |  |	     |  |_______double_______|
 *  |	     |  |	     |  |_______Object_______|
 *  |	     |  |	     |  |  			         |
 * [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
 */
typedef struct {
	ValueType type;
	union {
		bool boolean;
		FN_FLOAT number;
		Object* object;	/* Pointer to the heap memory. */
	} as;
} Value;

/* Value's type checking macros. */
#define IS_BOOL(value)		((value).type == VAL_BOOL)
#define IS_NUMBER(value)	((value).type == VAL_NUMBER)
#define IS_OBJECT(value)	((value).type == VAL_OBJECT)
#define IS_NIL(value)		((value).type == VAL_NIL)


/* The four macros below promote a native 'C' value to FunVM's 'Value' represenation */
#define BOOL_PACK(value)	((Value){VAL_BOOL,	 {.boolean = value}})
#define NUMBER_PACK(value)	((Value){VAL_NUMBER, {.number = value}})
#define OBJECT_PACK(value)	((Value){VAL_OBJECT, {.object = (Object*)value}})
#define NIL_PACK()			((Value){VAL_NIL,	 {.number = 0}})

/* These macros unpack FunVM's 'Value' back to 'C' native represenation. */
#define BOOL_UNPACK(value)		((value).as.boolean)
#define NUMBER_UNPACK(value)	((value).as.number)
#define OBJECT_UNPACK(value)	((value).as.object)

typedef struct {
	FN_UWORD capacity;
	FN_UWORD count;
	Value* pool;
} ConstantPool;

bool valuesEqual(Value a, Value b);
void initConstantPool(ConstantPool* constantPool);
void writeConstantPool(ConstantPool* constantPool, Value constant);
void freeConstantPool(ConstantPool* constantPool);
void printValue(Value value);

#endif // !FUNVM_VALUE_H
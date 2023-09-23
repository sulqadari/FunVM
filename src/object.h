#ifndef FUNVM_OBJECT_H
#define FUNVM_OBJECT_H

#include <stddef.h>
#include <stdint.h>

#include "value.h"
#include "bytecode.h"

/* Helper macro to obtain Object's type. */
#define OBJECT_TYPE(value)		(OBJECT_UNPACK(value)->type)

#define IS_FUNCTION(value)		isObjType(value, OBJ_FUNCTION)

#define IS_NATIVE(value)		isObjType(value, OBJ_NATIVE)
/* Safety check macro. */
#define IS_STRING(value)		isObjType(value, OBJ_STRING)

#define FUNCTION_UNPACK(value)	((ObjFunction*)OBJECT_UNPACK(value))

#define NATIVE_UNPACK(value)	(((ObjNative*)OBJECT_UNPACK(value))->function)

/* Returns the (ObjString*) from the heap. */
#define STRING_UNPACK(value)	((ObjString*)OBJECT_UNPACK(value))

/* Returns the (ObjString*).chars field from the heap. */
#define CSTRING_UNPACK(value)	(((ObjString*)OBJECT_UNPACK(value))->chars)

typedef enum {
	OBJ_STRING,
	OBJ_NATIVE,
	OBJ_FUNCTION
} ObjType;

/**
 * The root object of the overall object's hierarchy.
 * 
 * Each object type shall declare this struct as the very first
 * field, making it possible to use a 'type punning' pattern: a king of
 * single-inheritance in object-oriented languages.
 * Providing that, it's possible to convert, say, a pointer to ObjString
 * to Object and get access to its fields.
 * 
 * Any code that wants to work with all objects can treat them as base Object*
 * and ignore any other fields that may happen to follow.
 * 
 * Object's implementation differs from Value one in that, there is no
 * single structure gathering all types in union. Instead, each type
 * has its own structure with the 'Object object' as the first field.
*/
struct Object {
	ObjType type;
	struct Object* next;
};

/**
 * Function object structure.
 * The functions are first-class, thus ObjFunction has the same
 * 'Object' header that all object types share.
*/
typedef struct {
	Object		object;
	uint8_t		arity;		/* the number of params. */
	Bytecode	bytecode;	/* function's own bytecode. */
	ObjString*	name;		/* Function's name. */
} ObjFunction;

/** Typedef for pointer to the C function.
 * @param uint32_t: argument count.
 * @param Value*: pointer to the first argument on the stack.
 * @returns Value: the result value.
 */
typedef Value (*NativeFn)(uint8_t argCount, Value* args);

/**
 * Native function representation.
 */
typedef struct {
	Object object;
	NativeFn function;
} ObjNative;

/**
 * String object aimed to keep payload. 
 * The cached hash code of the 'chars' is used in hash table
 * to avoid redundant hash computation every time we lookig for a given key.
 */
struct ObjString {
	Object object;
	uint32_t length;
	char* chars;
	uint32_t hash;
};

ObjFunction* newFunction(void);
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, uint32_t length);
ObjString* copyString(const char* chars, uint32_t length);
void printObject(Value value);

/** Safety check before downcasting from 'Object*' to one of the child object type.
 *
 * The reason for this kind of checks is that, the stack keeps data
 * as 'Value' type which in case of, say, 'ObjString' expands to 'Value::Object*', i.e.,
 * before any kind of object to be inserted to the stack, it have to be 
 * "upcasted" to 'Object*'.
 * Thus, to handle 'Object*' as an 'ObjString*' we have to downcast it and be sure,
 * that the target entiry is exactly of type 'ObjString*'. */
static inline bool
isObjType(Value value, ObjType type)
{
	return (IS_OBJECT(value) && (OBJECT_UNPACK(value)->type == type));
}

#endif // !FUNVM_OBJECT_H
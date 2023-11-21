#ifndef FUNVM_OBJECT_H
#define FUNVM_OBJECT_H

#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "value.h"
#include "bytecode.h"

/* Helper macro to obtain Object's type. */
#define OBJECT_TYPE(value)		(OBJECT_UNPACK(value)->type)

#define IS_CLASS(value)			isObjType((value), OBJ_CLSAS)
#define IS_CLOSURE(value)		isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)		isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)		isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)		isObjType(value, OBJ_STRING)

#define CLASS_UNPACK(value)		((ObjClass*)	OBJECT_UNPACK(value))
#define CLOSURE_UNPACK(value)	((ObjClosure*)	OBJECT_UNPACK(value))
#define FUNCTION_UNPACK(value)	((ObjFunction*)	OBJECT_UNPACK(value))
#define NATIVE_UNPACK(value)	(((ObjNative*)	OBJECT_UNPACK(value))->function)
#define STRING_UNPACK(value)	((ObjString*)	OBJECT_UNPACK(value))
#define CSTRING_UNPACK(value)	(((ObjString*)	OBJECT_UNPACK(value))->chars)

typedef enum {
	OBJ_CLASS,
	OBJ_STRING,
	OBJ_NATIVE,
	OBJ_FUNCTION,
	OBJ_CLOSURE,
	OBJ_UPVALUE
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
	bool isMarked;
	struct Object* next;
};

/**
 * Function object structure.
 * The functions are first-class, thus ObjFunction has the same
 * 'Object' header that all object types share.
*/
typedef struct {
	Object		object;
	FN_UBYTE	arity;		/* the number of params. */
	FN_WORD		upvalueCount;
	Bytecode	bytecode;	/* function's own bytecode. */
	ObjString*	name;		/* Function's name. */
} ObjFunction;

/** Typedef for pointer to the C function.
 * @param FN_BYTE: argument count.
 * @param Value*: pointer to the first argument on the FunVM stack.
 * @returns Value: the result value.
 */
typedef Value (*NativeFn)(FN_BYTE argCount, Value* args);

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
	FN_UWORD length;
	char* chars;
	FN_UWORD hash;
};

/**
 * Runtime representation for upvalues.
 * Everytime a closed-over local variable is hoisted onto the heap,
 * "Value* location" will store the reference to it as long as needed,
 * even after the function the variable was declared in, has returned.
 *
 * The instance of this object is assigned to actual local variable,
 * not a copy.
 *
 * Because multiple closures can close over the same variable,
 * the instance of ObjUpvalue doesn't own the variable it references.
 */
typedef struct ObjUpvalue {
	Object object;
	Value* location;	/* Points to the closed-over variable. */
	Value closed;
	struct ObjUpvalue* next;	/* linked list of upvalues. */	
} ObjUpvalue;

/**
 * Wraps the function but does not own it because there may be multiple
 * closures that all reference the same function, and none of them claims
 * any special privilege over it.
 *
 * There are two implementation strategies for local variables: those
 * that aren't used in closures, are leaved as the are on the stack.
 * When local is captured by a closure, it will be lifted onto the heap
 * where it can live as long as needed.
 *
 * The runtime representation of the closure captures the local variable
 * surrounding the function as they exist when the function declaration
 * *is executed*, not just when it is compiled.
 */
typedef struct {
	Object object;
	ObjFunction* function;	/* pointer to the underlined function. */
	ObjUpvalue** upvalues;
	FN_WORD upvalueCount;
} ObjClosure;

typedef struct {
	Object object;
	ObjString* name;
} ObjClass;

ObjClass* newClass(ObjString* name);
ObjClosure* newClosure(ObjFunction* function);
ObjUpvalue* newUpvalue(Value* slot);
ObjFunction* newFunction(void);
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, FN_UWORD length);
ObjString* copyString(const char* chars, FN_UWORD length);
void printObject(Value value);

#ifdef FUNVM_DEBUG_GC
char* stringifyObjType(ObjType type);
#endif

/**
 * Safety check before downcasting from 'Object*' to one of the child object type.
 * 
 * The reason for this kind of checks is that, the stack keeps data
 * as 'Value' type which in case of, say, 'ObjString' expands to 'Value::Object*', i.e.,
 * before any kind of object to be inserted to the stack, it have to be 
 * "upcasted" to 'Object*'.
 * Thus, to handle 'Object*' as an 'ObjString*' we have to downcast it and be sure,
 * that the target entry is exactly of type 'ObjString*'. */
static inline bool
isObjType(Value value, ObjType type)
{
	return (IS_OBJECT(value) && (OBJECT_UNPACK(value)->type == type));
}

#endif // !FUNVM_OBJECT_H
#ifndef FUNVM_OBJECT_H
#define FUNVM_OBJECT_H

#include "common.h"
#include "value.h"
#include "bytecode.h"

#define OBJ_TYPE(value)        (OBJ_UNPACK(value)->type)

#define IS_FUNC(value)         isObjType(value, obj_func)
#define FUNC_UNPACK(value)     ((ObjFunction*)OBJ_UNPACK(value))

#define IS_STRING(value)       isObjType(value, obj_string)
#define STRING_UNPACK(value)   ((ObjString*)OBJ_UNPACK(value))
#define CSTRING_UNPACK(value)  (((ObjString*)OBJ_UNPACK(value))->chars)

#define IS_NATIVE(value)       isObjType(value, obj_native)
#define NATIVE_UNPACK(value)   (((ObjNative*)OBJ_UNPACK(value))->function)

typedef Value (*NativeFn)(int32_t argCount, Value* args);

typedef enum {
	obj_string,
	obj_func,
	obj_native
} ObjType;

struct Obj {
	ObjType type;
	Obj*    next;
};

struct ObjString {
	Obj         obj;
	uint32_t    len;
	uint32_t    hash;
	const char* chars;
};

typedef struct {
	Obj        obj;
	int32_t    arity;	/*<! Number of params. */
	ByteCode   bCode;	/*<! function's bytecode. */
	ObjString* name;	/*<! for debugging. */
} ObjFunction;

typedef struct {
	Obj obj;
	NativeFn function;
} ObjNative;

ObjFunction* newFunction(void);
ObjNative* newNative(NativeFn function);
ObjString* takeString(const char* chars, uint32_t length);
ObjString* copyString(const char* chars, uint32_t length);
void printObject(Value value);

static inline bool
isObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && OBJ_UNPACK(value)->type == type;
}

#endif /* FUNVM_OBJECT_H */
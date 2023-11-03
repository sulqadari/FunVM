#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

static VM* vm;

/** 
 * Note that the declaration of this function can be found in vm.h.
 * This weird decision is aimed to address the cross-dependency 
 * issue between vm.h and object.h header files.
*/
void
objSetVM(VM* _vm)
{
	vm = _vm;
}

/* call to ALLOCATE_OBJ() is kind of calling the "base class" constructor
 * to initialize the Object state. */
#define ALLOCATE_OBJ(type, objectType)	\
	(type*)allocateObject(sizeof(type), objectType)

/**
 * Allocates an object of the given type on the heap.
 * 
 * Note that the size in NOT just the size of the Object itself. The
 * caller passes in the number of bytes so that there is room for the
 * extra payload fields needed by the specific object type being created.
 * @param size_t size: the size needed for specific class.
 * @param ObjType type: a value to be assigned to Object::type field.
 * @returns Object*: pointer to the object allocated on the heap.
 */
static Object*
allocateObject(size_t size, ObjType type)
{
	/* Note the size which encompasses not only Object type,
	 * but extra bytes for ObjString struct. */
	Object* object = (Object*)reallocate(NULL, 0, size);
	
	/* Initialize the base class */
	object->type = type;
	object->next = vm->objects;
	vm->objects = object;

	return object;
}

/**
 * Creates closure.
 * Takes a pointer to the ObjFunction it wraps and returns a pointer
 * to closure just created.
 */
ObjClosure*
newClosure(ObjFunction* function)
{
	ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
	for (FN_WORD i = 0; i < function->upvalueCount; ++i)
		upvalues[i] = NULL;

	ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
	closure->function = function;

	closure->upvalues = upvalues;
	closure->upvalueCount = function->upvalueCount;
	
	return closure;
}

/**
 * @param Value* an address of the slot where the closed-over variable lives.
 */
ObjUpvalue*
newUpvalue(Value* slot)
{
	ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
	upvalue->location = slot;
	upvalue->next = NULL;
	return upvalue;
}

/**
 * Creates an instance of ObjFunction.
 * The initial state is left blank which will be filled in later.
 */
ObjFunction*
newFunction(void)
{
	ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
	function->arity = 0;
	function->upvalueCount = 0;
	function->name = NULL;
	initBytecode(&function->bytecode);
	
	return function;
}

/**
 * Takes a C function pointer and wraps it in an ObjNative by setting up
 * ObjNative's header 'OBJ_NATIVE' identifier and storing pointer to the
 * argument value in ObjNative's function field.
 */
ObjNative*
newNative(NativeFn function)
{
	ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
	native->function = function;
	return native;
}

/**
 * Creates a new ObjString on the heap and then initializes its fields.
 * It's sort of like a constructor in an OOP language.
 */
static ObjString*
allocateString(char* chars, FN_UWORD length, FN_UWORD hash)
{
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	/* Initialize the ObjString class */
	string->length = length;
	string->chars = chars;
	string->hash = hash;

	/* The keys are the strings we are care about, so just
	 * uses NIL for the values. 
	 * Note: here we're using the table more like 'hash set',
	 * rather than 'hash table'. */
	tableSet(vm->interns, string, NIL_PACK());

	return string;
}

static FN_UWORD
hashString(const char* key, FN_UWORD length)
{
	FN_UWORD hash = 2166136261U;	// 81 1C 9D C5
	
	for (FN_UWORD i = 0; i < length; ++i) {
		hash ^= (FN_UBYTE)key[i];
		hash *= 16777619;			// 01 00 01 93
	}

	return hash;
}

/**
 * Compared to copyString() function, this one handles a string
 * which have already been created on the heap, and only thing is left
 * is to assign it to the new ObjString entity.
*/
ObjString*
takeString(char* chars, FN_UWORD length)
{
	FN_UWORD hash = hashString(chars, length);

	/* Before taking the ownership over the string, look it up in the string set first. */
	ObjString* interned = tableFindString(vm->interns, chars, length, hash);
	if (NULL != interned) {
		FREE_ARRAY(char, chars, length + 1);	/* Free memory for the passed in string. */
		return interned;						/* return interned string. */
	}

	return allocateString(chars, length, hash);
}

/**
 * Copies the given char array from the source code and
 * assgins it to the ObjString.
 * 
 * To do that it allocates a new char array on the heap,
 * instantiates 'ObjString' and initializes its fields.
*/
ObjString*
copyString(const char* chars, FN_UWORD length)
{
	FN_UWORD hash = hashString(chars, length);

	if (NULL == vm->interns) {
		printf("vm->interns is NULL\n");
		exit(1);
	}
	/* Before copying the string, look it up in the string set first. */
	ObjString* interned = tableFindString(vm->interns, chars, length, hash);
	if (NULL != interned)
		return interned;

	char* heapChars = ALLOCATE(char, length + 1);

	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';
	return allocateString(heapChars, length, hash);
}

static void
printFunction(ObjFunction* function)
{
	if (NULL == function->name) {
		printf("<script>");
		return;
	}

	printf("<fn %s>", function->name->chars);
}

void
printObject(Value value)
{
	switch (OBJECT_TYPE(value)) {
		case OBJ_STRING:
			printf("%s", CSTRING_UNPACK(value));
		break;
		case OBJ_NATIVE: {
			printf("<native fn>");
		} break;
		case OBJ_FUNCTION:
			printFunction(FUNCTION_UNPACK(value));
		break;
		case OBJ_CLOSURE:
			printFunction(CLOSURE_UNPACK(value)->function);
		break;
		case OBJ_UPVALUE:
			printf("upvalue\n");
		break;
	}
}
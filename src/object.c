#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
setVM(VM* _vm)
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
 * Creates an instance of ObjFunction.
 * The initial state is left blank which will be filled in at
 * the function creation stage.
 */
ObjFunction*
newFunction(void)
{
	ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
	function->arity = 0;
	function->name = NULL;
	initBytecode(&function->bytecode);
	
	return function;
}

/**
 * Creates a new ObjString on the heap and then initializes its fields.
 * It's sort of like a constructor in an OOP language.
 */
static ObjString*
allocateString(char* chars, int32_t length, uint32_t hash)
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

static uint32_t
hashString(const char* key, int32_t length)
{
	uint32_t hash = 2166136261U;	// 81 1C 9D C5
	
	for (int32_t i = 0; i < length; ++i) {
		hash ^= (uint8_t)key[i];
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
takeString(char* chars, int32_t length)
{
	uint32_t hash = hashString(chars, length);

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
copyString(const char* chars, int32_t length)
{
	uint32_t hash = hashString(chars, length);

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
		case OBJ_FUNCTION:
			printFunction(FUNCTION_UNPACK(value));
		break;
	}
}
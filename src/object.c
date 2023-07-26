#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <x86/_stdint.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

static VM *vm;

void
initHeap(VM *_vm)
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
	Object *object = (Object*)reallocate(NULL, 0, size);
	/* Initialize the base class */
	object->type = type;
	object->next = vm->objects;
	vm->objects = object;

	return object;
}

static ObjString*
allocateString(char *chars, int32_t length, uint32_t hash)
{
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	/* Initialize the ObjString class */
	string->length = length;
	string->chars = chars;
	string->hash = hash;

	return string;
}

static uint32_t
hashString(const char *key, int32_t length)
{
	uint32_t hash = 2166136261U;	// 81 1C 9D C5
	
	for (int32_t i = 0; i < length; ++i) {
		hash ^= (uint8_t)key[i];
		hash *= 16777619;			// 01 00 01 93
	}

	return hash;
}

/**
 * Used in vm::concatenate() to allocate new instance of ObjString
 * onto the heap.
 * Its value is created from a two ObjStrings that about to be
 * concatenated.
*/
ObjString*
takeString(char *chars, int32_t length)
{
	uint32_t hash = hashString(chars, length);
	return allocateString(chars, length, hash);
}

/**
 * Copies the given char array from the source code into the String object.
 * 
 * To do that it allocates a new char array on the heap,
 * instantiates 'ObjString' and initializes its fields.
*/
ObjString*
copyString(const char *chars, int32_t length)
{
	uint32_t hash = hashString(chars, length);
	char *heapChars = ALLOCATE(char, length + 1);

	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';
	return allocateString(heapChars, length, hash);
}

void printObject(Value value)
{
	switch (OBJECT_TYPE(value)) {
		case OBJ_STRING: printf("%s", CSTRING_UNPACK(value)); break;
	}
}
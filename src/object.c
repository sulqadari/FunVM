#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "constant_pool.h"
#include "vm.h"

/* call to ALLOCATE_OBJ() is kind of calling the "base class" constructor
 * to initialize the Object state. */
#define ALLOCATE_OBJ(type, objectType)	\
	(type*)allocateObject(sizeof(type), objectType)

/**
 * Allocates an object of the given type.
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
	object->type = type;
	return object;
}

static ObjString*
allocateString(char *chars, int32_t length)
{
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	return string;
}

ObjString*
takeString(char *chars, int32_t length)
{
	return allocateString(chars, length);
}

ObjString*
copyString(const char *chars, int32_t length)
{
	char *heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';
	return allocateString(heapChars, length);
}

void printObject(Value value)
{
	switch (OBJECT_TYPE(value)) {
		case OBJ_STRING: printf("%s", CSTRING_UNPACK(value)); break;
	}
}
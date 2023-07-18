#include <stdio.h>
#include <string.h>
#include <x86/_stdint.h>

#include "memory.h"
#include "object.h"
#include "constant_pool.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType)	\
	(type*)allocateObject(sizeof(type), objectType)

static Obj*
allocateObject(size_t size, ObjType type)
{
	Obj *object = (Obj*)reallocate(NULL, 0, size);
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
copyString(const char *chars, int32_t length)
{
	char *heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';
	return allocateString(heapChars, length);
}

void printObject(Value value)
{
	switch (OBJ_TYPE(value)) {
		case OBJ_STRING: printf("%s", CSTRING_UNPACK(value)); break;
	}
}
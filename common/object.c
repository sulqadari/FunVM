#include "common.h"
#include "memory.h"
#include "object.h"
#include "values.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objType)	\
	(type*)allocateObject(sizeof(type), objType)

static Object*
allocateObject(size_t size, ObjType type)
{
	Object* obj = (Object*)reallocate(NULL, 0, size);
	obj->type = type;
	return obj;
}

static ObjString*
allocateString(char* chars, uint32_t length)
{
	ObjString* string = ALLOCATE_OBJ(ObjString, obj_string);
	string->length = length;
	string->chars = chars;
	return string;
}

ObjString*
copyString(const char* chars, uint32_t length)
{
	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';
	return allocateString(heapChars, length);
}
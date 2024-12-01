#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(objTypeSize, objType)  \
	(objTypeSize*)allocateObject(sizeof(objTypeSize), objType)

static Obj*
allocateObject(size_t size, ObjType objType)
{
	Obj* object = (Obj*)reallocate(NULL, 0 , size);
	object->type = objType;
	return object;
}

static ObjString*
allocateString(const char* heapChars, uint32_t length)
{
	ObjString* string = ALLOCATE_OBJ(ObjString, obj_string);
	string->len = length;
	string->chars = heapChars;
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

void
printObject(Value value)
{
	switch (OBJ_TYPE(value)) {
		case obj_string:
			printf("%s", CSTRING_UNPACK(value));
		break;
	}
}
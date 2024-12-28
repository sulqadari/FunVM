#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "hash_table.h"
#include "globals.h"

#define ALLOCATE_OBJ(objStruct, objType)  \
	(objStruct*)allocateObject(sizeof(objStruct), objType)

static Obj*
allocateObject(size_t size, ObjType objType)
{
	Obj* object  = (Obj*)reallocate(NULL, 0 , size);
	object->type = objType;
	object->next = vm.objects;
	vm.objects   = object;
	
	return object;
}

static uint32_t
hashString(const char* key, uint32_t length)
{
	uint32_t hash = 2166136261u; // 0x811C9DC5
	for (uint32_t i = 0; i < length; ++i) {
		hash ^= (uint8_t)key[i];
		hash *= 16777619;        // 0x01000193
	}

	return hash;
}

static ObjString*
allocateString(const char* heapChars, uint32_t length, uint32_t hash)
{
	ObjString* string = ALLOCATE_OBJ(ObjString, obj_string);
	string->len   = length;
	string->hash  = hash;
	string->chars = heapChars;

	// Add this new string into 'interns' hash table
	tableSet(&vm.strings, string, NULL_PACK);
	return string;
}

ObjString*
copyString(const char* chars, uint32_t length)
{
	uint32_t hash = hashString(chars, length);
	
	// Look up a given string in 'interns table' of the VM.
	ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
	if (interned != NULL)
		return interned;

	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';
	return allocateString(heapChars, length, hash);
}

ObjString*
takeString(const char* chars, uint32_t length)
{
	uint32_t hash = hashString(chars, length);
	
	// Look up a given string in 'interns table' of the VM.
	ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
	if (interned != NULL) {
		FREE_ARRAY(const char, (void*)chars, length + 1);
		return interned;
	}

	return allocateString(chars, length, hash);
}

ObjFunction*
newFunction(void)
{
	ObjFunction* function = ALLOCATE_OBJ(ObjFunction, obj_func);
	function->arity = 0;			// All fields of the function will get filled in later
	function->name = NULL;			// after the function is created.
	initByteCode(&function->bCode);
	return function;
}

static void
printFunction(ObjFunction* function)
{
	if (function->name == NULL) {
		printf("<script>");
		return;
	}

	printf("<fn %s>", function->name->chars);
}

void
printObject(Value value)
{
	switch (OBJ_TYPE(value)) {
		case obj_string:
			printf("%s", CSTRING_UNPACK(value));
		break;
		case obj_func:
			printFunction(FUNC_UNPACK(value));
		break;
	}
}
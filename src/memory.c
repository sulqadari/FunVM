#include <stdlib.h>
#include <stdio.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

/*
 * Cases under examination:
 * allocate new block	- newCap > 0,  oldCap == 0.
 * free allocation		- newCap == 0, oldCap is meaningless.
 * shrink capacity		- newCap < oldCap
 * increase capacity	- newCap > oldCap. */
void*
reallocate(void* array, size_t oldCap, size_t newCap)
{
	if (0 == newCap) {
		free(array);
		return NULL;
	}

	void* result = realloc(array, newCap);
	if (NULL == result) {
		printf("in reallocate(): failed to allocate memory.\n");
		exit(1);
	}

	return result;
}

static void
freeObject(Object* object)
{
	switch (object->type) {
		case OBJ_STRING: {
			ObjString* string = (ObjString*)object;
			FREE_ARRAY(char, string->chars, string->length + 1);
			FREE(ObjString, object);
		} break;

		/* Note that function's name is an instance of ObjString
		 * which will be garbage collected once we delete the function. */
		case OBJ_FUNCTION: {
			ObjFunction* function = (ObjFunction*)object;
			freeBytecode(&function->bytecode);
			FREE(ObjFunction, object);
		} break;
	}
}

void
freeObjects(VM* vm)
{
	Object* object = vm->objects;
	while (object != NULL) {
		Object*  next = object->next;
		freeObject(object);
		object = next;
	}
}
#include "memory.h"
#include "object.h"
#include "globals.h"
#include "object_pool.h"

void*
reallocate(void* ptr, size_t oldSize, size_t newSize)
{
	void* result = NULL;
	do {
		if (newSize == 0) {
			fvm_free(ptr);
			break;
		}

		result = fvm_realloc(ptr, newSize);
		if (result == NULL) {
			fprintf(stderr, "ERROR: not enough memory\nfile: %s\nline: %d\n", __FILE__, __LINE__);
			exit(1);
		}
	} while(0);

	return result;
}

static void
freeObject(Obj* object)
{
	switch(object->type) {
		case obj_string: {
			ObjString* str = (ObjString*)object;
			writeObjString(str);

			FREE_ARRAY(char, (char*)str->chars, str->len + 1);
			FREE(ObjString, object);
		} break;
		case obj_func: {
			ObjFunction* function = (ObjFunction*)object;
			writeObjFunction(function);
			freeByteCode(&function->bCode);
			FREE(ObjFunction, object);
		}break;
		case obj_native: {
			FREE(ObjNative, object);
		}break;
	}
}

void
freeObjects(void)
{
	Obj* object = objPool->objList;
	while (object != NULL) {
		Obj* next = object->next;
		freeObject(object);
		object = next;
	}
}
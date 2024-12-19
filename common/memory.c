#include "memory.h"
#include "object.h"
#include "globals.h"
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
			FREE_ARRAY(char, (char*)str->chars, str->len + 1);
			FREE(ObjString, object);
		} break;
	}
}

void
freeObjects(void)
{
	Obj* object = vm.objects;
	while (object != NULL) {
		Obj* next = object->next;
		freeObject(object);
		object = next;
	}
}
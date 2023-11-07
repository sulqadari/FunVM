#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#ifdef FUNVM_DEBUG_GC
#include "debug.h"
#endif

static VM* vm;

/** 
 * Note that the declaration of this function can be found in vm.h.
 * This weird decision is aimed to address the cross-dependency 
 * issue between vm.h and memory.h header files.
*/
void
memorySetVM(VM* _vm)
{
	vm = _vm;
}

/*
 * Cases under examination:
 * allocate new block	- newCap > 0,  oldCap == 0.
 * free allocation		- newCap == 0, oldCap is meaningless.
 * shrink capacity		- newCap < oldCap
 * increase capacity	- newCap > oldCap. */
void*
reallocate(void* array, size_t oldCap, size_t newCap)
{
	if (newCap > oldCap) {
#ifdef FUNVM_DEBUG_GC_STRESS
		collectGarbage();
#endif
	}

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
#ifdef FUNVM_DEBUG_GC
	printf("%p free memory for type %d\n", (void*)object, size, type);
#endif

	switch (object->type) {
		case OBJ_STRING: {
			ObjString* string = (ObjString*)object;
			FREE_ARRAY(char, string->chars, string->length + 1);
			FREE(ObjString, object);
		} break;

		case OBJ_NATIVE: {
			FREE(ObjNative, object);
		} break;

		/* Cannot be deleted until all objects referencing it are gone.
		 * 
		 * Note that function's name is an instance of ObjString
		 * which will be garbage collected once we delete the function. */
		case OBJ_FUNCTION: {
			ObjFunction* function = (ObjFunction*)object;
			freeBytecode(&function->bytecode);
			FREE(ObjFunction, object);
		} break;

		case OBJ_CLOSURE: {
			ObjClosure* closure = (ObjClosure*)object;
			FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);	
			FREE(ObjClosure, object);
		} break;
		
		case OBJ_UPVALUE: {
			FREE(ObjUpvalue, object);
		} break;
	}
}

void
markObject(Object* object)
{
	if (NULL == object)
		return;

#ifdef FUNVM_DEBUG_GC
	printf("%p mark ", (void*)object);
	printValue(OBJECT_PACK(object));
	printf("\n");
#endif
	object->isMarked = true;

	/* When an object is marked, add it to the worklist.
	 * The memory for grayStack isn't managed by the GC. If we used
	 * realloc() implementation, then GC would start a new GC recursevily. */
	if (vm->grayCapacity < vm->grayCount + 1) {
		vm->grayCapacity = INCREASE_CAPACITY(vm->grayCapacity);
		vm->grayStack = (Object**)realloc(vm->grayStack,
									sizeof(Object*) * vm->grayCapacity);
		
		if (NULL == vm->grayStack) {
			printf("Failed to allocate grayStack.\n");
			exit(1);
		}
	}

	vm->grayStack[vm->grayCount++] = object;
}

void
markValue(Value value)
{
	if (IS_OBJECT(value))
		markObject(OBJECT_UNPACK(value));
}

static void
markRoots(void)
{
	/* First, traverse the local variables and temporaries on the VM's stack. */
	for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
		markValue(*slot);
	}

	/* Keep in mind an objects which inaccessible for user, but
	 * intensively used by VM own, e.g. CallFrame stack and the
	 * pointer to the closure being called, which is used
	 * to access constants and upvalues. */
	for (FN_UWORD i = 0; i < vm->frameCount; ++i) {
		markObject((Object*)vm->frames[i].closure);
	}

	/* The open upvalue list is another set of values that
	 * the VM can directly reach. */
	for (ObjUpvalue* upvalue = vm->openUpvalues;
			upvalue != NULL; upvalue = upvalue->next) {

		markObject((Object*)upvalue);
	}

	/* Then find roots among global variables and mark them too. */
	markTable(vm->globals);
	markCompilerRoots();
}

void
collectGarbage(void)
{
#ifdef FUNVM_DEBUG_GC
	printf("-- gc begin.\n");
#endif

	markRoots();

#ifdef FUNVM_DEBUG_GC
	printf("-- gc end.\n");
#endif
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

	free(vm->grayStack);
}
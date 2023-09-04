#ifndef FUNVM_MEMORY_H
#define FUNVM_MEMORY_H

#include "value.h"
#include "object.h"
#include "vm.h"

/* Allocate a space on the heap. */
#define ALLOCATE(type, count)							\
	(type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer)								\
	reallocate(pointer, sizeof(type), 0)

#define INCREASE_ARRAY(type, array, oldCap, newCap)		\
	(type*)reallocate(array, sizeof(type) * (oldCap),	\
						sizeof(type) * (newCap))

#define FREE_ARRAY(type, array, oldCap)					\
	reallocate(array, sizeof(type) * (oldCap), 0)

/* At the very first call (when the capacity of the bytecode array is zero),
 * this macro intializes the capacity vaiable with value '8'. */
#define INCREASE_CAPACITY(capacity)						\
	(((capacity) < 8) ? 8 : ((capacity) * 2))

void* reallocate(void* array, size_t oldCap, size_t newCap);
void freeObjects(VM* vm);

#endif // !FUNVM_MEMORY_H
#ifndef FUNVM_MEMORY_H
#define FUNVM_MEMORY_H

#include "common.h"

/* At the very first call (when the capacity of the bytecode array is zero),
 * this macro intializes the capacity vaiable with value '8'. */
#define INCREASE_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * 2)

#define INCREASE_ARRAY(type, array, oldcap, newCap) \
	(type*)reallocate(array, sizeof(type) * (oldCap), \
						sizeof(type) * (newCap))

#define FREE_ARRAY(type, array, oldCap) \
	reallocate(array, sizeof(type) * (oldCap), 0)

void* reallocate(void *array, size_t oldCap, size_t newCap);

#endif // !FUNVM_MEMORY_H
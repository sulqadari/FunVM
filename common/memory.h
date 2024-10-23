#ifndef FUNVM_MEMORY_H
#define FUNVM_MEMORY_H

#include <stdlib.h>
#include "common.h"

#if defined(FUNVM_ARCH_x64)
#	define heapRealloc realloc
#	define heapFree free
#else
#	define heapRealloc fvm_realloc
#	define heapFree fvm_free
#endif /* FUNVM_ARCH_x64 */

#define GROW_CAPACITY(cap)		\
	((cap) < 8 ? 8 : (cap) * 1.5)

#define GROW_ARRAY(type, ptr, oldCap, newCap)	\
	(type*)reallocate(ptr, sizeof(type) * (oldCap), sizeof(type) * (newCap))

#define FREE_ARRAY(type, ptr, oldCap)	\
	reallocate(ptr, sizeof(type) * (oldCap), 0)

void* reallocate(void* ptr, size_t oldSize, size_t newSize);

#endif /* FUNVM_MEMORY_H */
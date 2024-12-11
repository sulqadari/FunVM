#ifndef FUNVM_MEMORY_H
#define FUNVM_MEMORY_H

#include "common.h"

#if defined(FUNVM_MEM_MANAGER)
#	include "heap.h"
#	define fvm_alloc heapAlloc
#	define fvm_realloc heapRealloc
#	define fvm_free heapFree
#else
#	define fvm_realloc realloc
#	define fvm_free free
#endif /* FUNVM_MEM_MANAGER */

#define GROW_CAPACITY(cap)						\
	((cap) < 8 ? 8 : (cap) * 1.5)

#define ALLOCATE(type, count)					\
	(type*)reallocate(NULL, 0, sizeof(type) * count)

#define FREE(type, ptr)							\
	(type*)reallocate(ptr, sizeof(type), 0)

#define GROW_ARRAY(type, ptr, oldCap, newCap)	\
	(type*)reallocate(ptr, sizeof(type) * (oldCap), sizeof(type) * (newCap))

#define FREE_ARRAY(type, ptr, oldCap)			\
	reallocate(ptr, sizeof(type) * (oldCap), 0)

void* reallocate(void* ptr, size_t oldSize, size_t newSize);
void freeObjects(void);

#endif /* FUNVM_MEMORY_H */
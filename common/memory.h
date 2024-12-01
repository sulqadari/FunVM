#ifndef FUNVM_MEMORY_H
#define FUNVM_MEMORY_H

#include "common.h"

#if defined(FUNVM_MEM_MANAGER)
#	define fvm_realloc _realloc
#	define fvm_free _free
#else
#	define fvm_realloc realloc
#	define fvm_free free
#endif /* FUNVM_MEM_MANAGER */

#define ALLOCATE(type, count)   \
	(type*)reallocate(NULL, 0, sizeof(type) * count)

#define GROW_CAPACITY(cap)		\
	((cap) < 8 ? 8 : (cap) * 1.5)

#define GROW_ARRAY(type, ptr, oldCap, newCap)	\
	(type*)reallocate(ptr, sizeof(type) * (oldCap), sizeof(type) * (newCap))

#define FREE_ARRAY(type, ptr, oldCap)	\
	reallocate(ptr, sizeof(type) * (oldCap), 0)

void* reallocate(void* ptr, size_t oldSize, size_t newSize);

#endif /* FUNVM_MEMORY_H */
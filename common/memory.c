#include "memory.h"

void*
reallocate(void* ptr, size_t oldSize, size_t newSize)
{
	void* result = NULL;
	do {
		if (newSize == 0) {
			heapFree(ptr);
			break;
		}

		result = heapRealloc(ptr, newSize);
	} while(0);

	return result;
}
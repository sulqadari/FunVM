#include <stdlib.h>
#include "memory.h"

/*
 * Cases under examination:
 * allocate new block	- newCap > 0,  oldCap == 0.
 * free allocation		- newCap == 0, oldCap is meaningless.
 * shrink capacity		- newCap < oldCap
 * increase capacity	- newCap > oldCap. */
void*
reallocate(void *array, size_t oldCap, size_t newCap)
{
	if (0 == newCap) {
		free(array);
		return NULL;
	}

	void *result = realloc(array, newCap);
	if (NULL == result)
		exit(1);

	return result;
}
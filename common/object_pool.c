#include "common.h"
#include "memory.h"
#include "object_pool.h"
#include "globals.h"
void
initObjPool(void)
{
	objPool.count = 0;
	objPool.values = NULL;
}

void
freeObjPool(void)
{
	FREE_ARRAY(char, objPool.values, objPool.count);
	initObjPool();
}

uint32_t
writeObjPool(uint8_t* data, uint32_t length)
{
	uint32_t offset = objPool.count;
	objPool.count += length;

	objPool.values = GROW_ARRAY(char, objPool.values, offset, objPool.count);
	memcpy(objPool.values + offset, data, length - 1);
	objPool.values[objPool.count - 1] = '\0';
	return offset;
}
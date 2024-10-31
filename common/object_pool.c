#include "memory.h"
#include "object_pool.h"

void
initObjPool(ObjPool* objPool)
{
	objPool->values = NULL;
	objPool->capacity = 0;
	objPool->count = 0;
}

void
freeObjPool(ObjPool* objPool)
{
	FREE_ARRAY(i32, objPool->values, objPool->capacity);
	initConstPool(objPool);
}

void
writeObjPool(ObjPool* objPool, void* value, ObjType type)
{
	if (type == obj_string) {
		for (uint32_t i = 0; i < UINT16_MAX; ++i) {
			((char*)objPool->values)[i] = CSTRING_UNPACK(value)[i];
		}
	}
}
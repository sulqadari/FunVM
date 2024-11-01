#include "memory.h"
#include "object_pool.h"

void
initObjPool(ObjPool* objPool)
{
	objPool->values = NULL;
	objPool->offset = 0;
	objPool->size = 0;
}

void
freeObjPool(ObjPool* objPool)
{
	FREE_ARRAY(i32, objPool->values, objPool->size);
	initObjPool(objPool);
}

void
writeObjPool(ObjPool* objPool, void* obj)
{
	ObjType type = OBJECT_TYPE(obj);

	if (type == obj_string) {
		
		uint32_t oldSize = objPool->size;
		uint32_t dataLength = OBJECT_STRING(obj)->length;
		objPool->size += dataLength;
		objPool->values = GROW_ARRAY(void, objPool->values, oldSize, objPool->size);
		
		for (uint32_t i = oldSize + 1; i < oldSize + dataLength + 1; ++i) {
			OBJPOOL_AS_STRING(objPool)[i] = OBJECT_CSTRING(obj)[i];
		}
	}
}
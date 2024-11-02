#include "memory.h"
#include "object_pool.h"

void
initObjPool(ObjPool* objPool)
{
	objPool->values = NULL;
	objPool->size = 0;
}

void
freeObjPool(ObjPool* objPool)
{
	FREE_ARRAY(i32, objPool->values, objPool->size);
	initObjPool(objPool);
}

int32_t
writeObjPool(ObjPool* objPool, void* obj)
{
	ObjType type = OBJECT_TYPE(obj);

	if (type == obj_string) {
		
		uint32_t offset = objPool->size;
		objPool->size += sizeof(ObjString) + OBJECT_STRING(obj)->length;
		
		objPool->values = GROW_ARRAY(uint8_t, objPool->values, offset, objPool->size);
		
		memcpy(objPool->values, obj, sizeof(ObjString));
		memcpy(objPool->values + sizeof(ObjString), OBJECT_CSTRING(obj), OBJECT_STRING(obj)->length);

		return offset;
	}

	return -1;
}
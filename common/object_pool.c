#include "value.h"
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
	uint32_t offset;
	ObjType type = ((Obj*)obj)->type;

	if (type == obj_string) {
		ObjString* str = (ObjString*)obj;
		offset = objPool->size;

		objPool->size += sizeof(ObjString) + str->len;
		objPool->values = GROW_ARRAY(uint8_t, objPool->values, offset, objPool->size);
		memcpy(objPool->values + offset, str, sizeof(ObjString));
		memcpy(objPool->values + sizeof(ObjString) + offset, str->chars, str->len);

		return offset;
	}

	return -1;
}
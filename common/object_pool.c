#include "common.h"
#include "memory.h"
#include "object_pool.h"
#include "globals.h"
void
initObjPool(void)
{
	objPool.count = 0;
	objPool.values = NULL;
	objPool.objects = NULL;
}

void
freeObjPool(void)
{
	FREE_ARRAY(uint8_t, objPool.values, objPool.count);
	initObjPool();
}

uint32_t
writeObjString(ObjString* string)
{
	uint32_t index = objPool.count;
	uint32_t offset = index;
	objPool.count  += sizeof(ObjString) + string->len + 1;

	objPool.values = GROW_ARRAY(uint8_t, objPool.values, index, objPool.count);

	memcpy(objPool.values + offset, (uint8_t*)string, sizeof(ObjString));
	offset += sizeof(ObjString);

	memcpy(objPool.values + offset, string->chars, string->len - 1);
	offset += string->len;
	objPool.values[offset - 1] = '\0';
	
	return index;
}

uint32_t
writeObjFunction(ObjFunction* function)
{
	uint32_t index = objPool.count;
	uint32_t offset = index;

	objPool.count  += sizeof(ObjFunction)
					+ sizeof(ByteCode)  + function->bCode.count
					+ sizeof(ConstPool) + function->bCode.constants.count * sizeof(Value);

	// Corner case: the main function hasn't name field.
	if (function->name != NULL) {
		objPool.count  += sizeof(ObjString) + function->name->len + 1;
	}

	objPool.values = GROW_ARRAY(uint8_t, objPool.values, index, objPool.count);
	
	memcpy(objPool.values + offset, (uint8_t*)function, sizeof(ObjFunction));
	offset += sizeof(ObjFunction);

	memcpy(objPool.values + offset, (uint8_t*)&function->bCode, sizeof(ByteCode));
	offset += sizeof(ByteCode);

	memcpy(objPool.values + offset, function->bCode.code, function->bCode.count);
	offset += function->bCode.count;
	
	memcpy(objPool.values + offset, (uint8_t*)&function->bCode.constants, sizeof(ConstPool));
	offset += sizeof(ConstPool);

	memcpy(objPool.values + offset, (uint8_t*)function->bCode.constants.values, function->bCode.constants.count * sizeof(Value));
	offset += function->bCode.constants.count;
	
	// Corner case: the main function hasn't name field.
	if (function->name != NULL) {
		memcpy(objPool.values + offset, (uint8_t*)function->name, sizeof(ObjString));
		offset += sizeof(ObjString);
		memcpy(objPool.values + offset, function->name->chars, function->name->len - 1);
		offset += function->name->len;
		objPool.values[offset - 1] = '\0';
	}
	
	return index;
}
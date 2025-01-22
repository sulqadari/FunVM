#include "common.h"
#include "memory.h"
#include "object_pool.h"
#include "globals.h"


void
initObjPool(void)
{
	objPool = ALLOCATE(ObjectPool, 1);
	objPool->idxCount  = 0;
	objPool->valuesLen = 0;
	objPool->values    = NULL;
	objPool->objList   = NULL;
}

void
freeObjPool(void)
{
	FREE_ARRAY(uint8_t, objPool->values, objPool->valuesLen);
	FREE(ObjectPool, objPool);
}

static void
updateAddress(uint32_t offset, uint8_t dataType)
{
	switch (dataType) {
		case 0: /* Nothing to update with ObjFunction itself. */							break;
		case 1: ((ObjFunction*)objPool->values)->bCode.code = (uint8_t*)offset;				break;
		case 2: ((ObjFunction*)objPool->values)->bCode.constants.values = (Value*)(offset);	break;
		case 3: ((ObjFunction*)objPool->values)->name->chars = (char*)(offset);				break;
		case 4: (  (ObjString*)objPool->values)->chars = (const char*)(offset);				break;
		default:
	}
}

static uint32_t
saveDataInObjPool(uint32_t offset, uint8_t* data, uint32_t length, uint8_t dataType)
{
	memcpy(objPool->values + offset, data, length);
	updateAddress(offset, dataType);

	return length;
}

uint32_t
writeObjString(ObjString* string)
{
	uint32_t index = objPool->valuesLen;
	uint32_t offset = index;

	objPool->valuesLen += sizeof(ObjString) + string->len + 1;
	objPool->values     = GROW_ARRAY(uint8_t, objPool->values, index, objPool->valuesLen);

	offset += saveDataInObjPool(offset, (uint8_t*)string, sizeof(ObjString), 0);
	offset += saveDataInObjPool(offset, (uint8_t*)string->chars, string->len + 1, 4);
	
	objPool->indexes[objPool->idxCount++] = index;
	return index;
}

static uint32_t
writeConstants(uint32_t offset, Value* constPool, uint32_t count)
{
	ObjType type;

	for (uint32_t i = 0; i < count; ++i) {
		
		type = OBJ_TYPE(constPool[i]);
		switch (type) {
			case obj_string:
				offset += writeObjString(STRING_UNPACK(constPool[i]));
			break;
			
			default:
				break;
		}
	}

	return offset;
}

uint32_t
writeObjFunction(ObjFunction* function)
{
	uint32_t index  = objPool->valuesLen;
	uint32_t offset = index;

	objPool->valuesLen += sizeof(ObjFunction)
					+ function->bCode.capacity
					+ function->bCode.constants.capacity * sizeof(Value);

	// Corner case: the main function hasn't name field.
	// if (function->name != NULL) {
	// 	objPool->valuesLen  += function->name->len + 1;
	// }

	objPool->values = GROW_ARRAY(uint8_t, objPool->values, index, objPool->valuesLen);

	offset += saveDataInObjPool(offset, (uint8_t*)function, sizeof(ObjFunction), 0);
	offset += saveDataInObjPool(offset, function->bCode.code, function->bCode.capacity, 1);
	offset += saveDataInObjPool(offset, (uint8_t*)function->bCode.constants.values, function->bCode.constants.capacity * sizeof(Value), 2);
	
	offset += writeConstants(offset, function->bCode.constants.values, function->bCode.constants.count);

	// Corner case: the main function hasn't name field.
	// if (function->name != NULL) {
	// 	offset += saveDataInObjPool(offset, (uint8_t*)function->name->chars, function->name->len, 3);
	// 	objPool->values[offset] = '\0';
	// }

	objPool->indexes[objPool->idxCount++] = index;
	return index;
}
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
	FREE_ARRAY(uint8_t, objPool.values, objPool.count);
	initObjPool();
}

static void
writeChars(const char* data, uint32_t length)
{
	uint32_t oldCap = objPool.count;
	objPool.count += length;

	objPool.values = GROW_ARRAY(uint8_t, objPool.values, oldCap, objPool.count);
	memcpy(objPool.values + oldCap, data, length - 1);
	objPool.values[objPool.count - 1] = '\0';
}

uint32_t
writeObjString(ObjString* string)
{
	const char* temp = string->chars;	// store the reference to the char array.
	uint32_t offset = objPool.count;
	objPool.count  += sizeof(ObjString);

	
	string->chars = NULL;	// Zero out pointer to chars, because its value will be updated in the VM later.

	objPool.values = GROW_ARRAY(uint8_t, objPool.values, offset, objPool.count);
	memcpy(objPool.values + offset, (uint8_t*)string, sizeof(ObjString));
	
	writeChars(temp, string->len + 1);
	string->chars = temp;	// restore pointer to char array. This will be deallocated freeObject()
	return offset;
}
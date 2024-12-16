#include "bytecode.h"
#include "memory.h"

// uint32_t* lines;

void
initByteCode(ByteCode* bCode)
{
	bCode->count = 0;
	bCode->capacity = 0;
	bCode->code = NULL;
	// lines = NULL;
	initConstPool(&bCode->constants);
}

void
freeByteCode(ByteCode* bCode)
{
	FREE_ARRAY(uint8_t, bCode->code, bCode->capacity);
	// FREE_ARRAY(uint32_t, lines, bCode->capacity);
	freeConstPool(&bCode->constants);
	initByteCode(bCode);
}

void
writeByteCode(ByteCode* bCode, uint8_t byte, uint32_t line)
{
	do {
		if (bCode->capacity >= bCode->count + 1)
			break;
		
		uint32_t oldCap = bCode->capacity;
		bCode->capacity = GROW_CAPACITY(oldCap);
		bCode->code     = GROW_ARRAY(uint8_t, bCode->code, oldCap, bCode->capacity);
		// lines           = GROW_ARRAY(uint32_t, lines, oldCap, bCode->capacity);
	} while(0);

	// lines[bCode->count] = line;
	bCode->code[bCode->count++] = byte;
}

uint32_t
addConstant(ByteCode* bCode, Value value)
{
	writeConstPool(&bCode->constants, value);
	return bCode->constants.count - 1;
}

uint32_t
addObject(ByteCode* bCode, void* obj)
{
	return writeObjPool(&bCode->objects, obj);
}
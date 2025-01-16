#ifndef FUNVM_STRING_POOL_H
#define FUNVM_STRING_POOL_H

#include <stdint.h>
#include "object.h"

typedef struct {
	uint32_t idxCount;
	uint32_t indexes[256];
	uint32_t valuesLen;
	uint8_t* values;
	Obj*     objects;
} ObjectPool;

void initObjPool(void);
void freeObjPool(void);
uint32_t writeObjString(ObjString* string);
uint32_t writeObjFunction(ObjFunction* function);

#endif /* FUNVM_STRING_POOL_H */
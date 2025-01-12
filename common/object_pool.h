#ifndef FUNVM_STRING_POOL_H
#define FUNVM_STRING_POOL_H

#include <stdint.h>
#include "object.h"

typedef struct {
	uint32_t count;
	uint8_t* values;
} ObjectPool;

void initObjPool(void);
void freeObjPool(void);
uint32_t writeObjString(ObjString* string);
uint32_t writeObjFunction(ObjFunction* function);

#endif /* FUNVM_STRING_POOL_H */
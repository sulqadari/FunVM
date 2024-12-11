#ifndef FUNVM_OBJECT_POOL_H
#define FUNVM_OBJECT_POOL_H

#include "common.h"
#include "value.h"
#include "object.h"

typedef struct {
	uint32_t size;
	uint8_t* values;
} ObjPool;

#define OBJPOOL_AS_STRING(pool)		((char*)objPool->values)

void initObjPool(ObjPool* objPool);
void freeObjPool(ObjPool* objPool);
int32_t writeObjPool(ObjPool* objPool, void* obj);

#endif /* FUNVM_OBJECT_POOL_H */
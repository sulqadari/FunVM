#ifndef FUNVM_OBJECT_POOL_H
#define FUNVM_OBJECT_POOL_H

#include "common.h"
#include "values.h"
#include "object.h"

typedef struct {
	uint32_t count;
	uint32_t capacity;
	void* values;
} ObjPool;

void initObjPool(ObjPool* objPool);
void freeObjPool(ObjPool* objPool);
void writeObjPool(ObjPool* objPool, Object* value);

#endif /* FUNVM_OBJECT_POOL_H */
#ifndef FUNVM_STRING_POOL_H
#define FUNVM_STRING_POOL_H

#include <stdint.h>

typedef struct {
	uint32_t count;
	uint8_t* values;
} ObjectPool;

void initObjPool(void);
void freeObjPool(void);
uint32_t writeObjPool(uint8_t* data, uint32_t length);

#endif /* FUNVM_STRING_POOL_H */
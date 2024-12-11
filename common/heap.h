#ifndef FUNVM_HEAP_H
#define FUNVM_HEAP_H

#include <stdint.h>

void  heapInit(void);
void* heapAlloc(uint32_t size);
void  heapFree(void* ptr);
void* heapRealloc(void* ptr, uint32_t newSize);

#endif /* FUNVM_HEAP_H */
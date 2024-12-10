#ifndef FUNVM_HEAP_H
#define FUNVM_HEAP_H

#include <stdint.h>

void heapInit(void);
void* heapAlloc(uint32_t size);
void heapFree(void* ptr);

#endif /* FUNVM_HEAP_H */
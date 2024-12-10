#ifndef FUNVM_HEAP_H
#define FUNVM_HEAP_H

#include <stdint.h>
#define HEAP_STATIC_SIZE (1 * 64)

void heapInit(void);
void* heapAlloc(uint32_t size);
void heapFree(void* ptr);

#endif /* FUNVM_HEAP_H */
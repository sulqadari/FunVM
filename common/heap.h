#ifndef FUNVM_HEAP_H
#define FUNVM_HEAP_H

#include <stdint.h>

#define HEAP_STATIC_SIZE		(10 * 1024)
typedef struct block_t block_t;

/**
 * int32_t size  - negative value designates a free memory block. When the block is
 *                 allocated, the actual length of the payload will be assigned to
 *                 this field.
 * block_t* next - a next vacant block. */
struct block_t {
	int32_t  size;
	block_t* next;
};

void  heapInit(void);
void* heapAlloc(uint32_t size);
void  heapFree(void* ptr);
void* heapRealloc(void* ptr, uint32_t newSize);

#endif /* FUNVM_HEAP_H */
#include "heap.h"

#ifndef NULL
#	define NULL ((void*)0)
#endif

/**
 * int32_t size  - negative value designates a free memory block. When the block is
 *                 allocated, the actual length of the payload will be assigned to
 *                 this field.
 * block_t* next - a next vacant block. */
typedef struct {
	int32_t size;
} block_t;

static uint8_t heap[HEAP_STATIC_SIZE] __attribute__ ((aligned (4)));

#define HEAP_INIT_SIZE ((block_t*)heap)->size
#define HEAP_INIT_NEXT ((block_t*)heap)->next

#define ALLIGN4(value) (((value + 3) >> 2) << 2)

/**
 * Initializes heap storage.
 */
void
heapInit(void)
{
	uint32_t* pHeap = (uint32_t*)heap;
	
	/* initialize the heap. */
	for (uint32_t i = 0; i < (HEAP_STATIC_SIZE / 4); ++i) {
		pHeap[i] = 0;
	}

	/* Here, the whole heap is marked as free: the size field has value '-10240'.
	 * According to accepted rule, the negative value designates a free block.
	 * Thus, the whole heap is free. */
	HEAP_INIT_SIZE = (0 - (HEAP_STATIC_SIZE - sizeof(block_t)));
	// HEAP_INIT_NEXT = NULL;
}

/**
 * Allocate memory from the heap.
 */
void*
heapAlloc(uint32_t requested)
{
	void* result = NULL;
	block_t* block = (block_t*)heap;
	uint32_t blockSize = 0;

	requested = ALLIGN4(requested);
	// Starting from the first block, iterate through the entire heap
	for (; (uint32_t)block < (uint32_t)(heap + HEAP_STATIC_SIZE); block = (block_t*)((uint8_t*)block + sizeof(block_t) + blockSize)) {
		if (block->size > 0) {
			blockSize = block->size;
			continue;
		}
		
		blockSize = 0 - block->size; // Get the number of available space in this block

		if (requested == blockSize) {
			block->size = requested;
			result = (void*)((uint8_t*)block + sizeof(block_t));
			break;
		} else if (blockSize > requested + sizeof(block_t)) {
			block_t* nextFreeBlk;
			block->size = requested;
			nextFreeBlk = (block_t*)((uint8_t*)block + sizeof(block_t) + requested);
			blockSize  -= requested + sizeof(block_t);
			nextFreeBlk->size = 0 - blockSize;
			
			result = (void*)((uint8_t*)block + sizeof(block_t));
			break;
		}
	}

	return result;
}

void
heapFree(void* ptr)
{

}
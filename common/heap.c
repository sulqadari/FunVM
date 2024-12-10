#include "heap.h"

#ifndef NULL
#	define NULL ((void*)0)
#endif

#define HEAP_STATIC_SIZE       			(1 * 40)	/*<! The max value is 0x07FFFF (524,287 bytes). */
#define HEAP_INIT_SIZE         			((block_t*)heap)->size
#define HEAP_INIT_INDX         			((block_t*)heap)->index
#define GET_BLOCK_INDEX(blk)   			((block_t*)((uint8_t*)blk - sizeof(block_t)))->index
#define GET_NEXT_BLOCK(curr, offset)	(block_t*)((uint8_t*)curr + sizeof(block_t) + (offset))
#define ALLIGN4(value)         			(((value + 3) >> 2) << 2)

/**
 * int32_t size  - negative value designates a free memory block. When the block is
 *                 allocated, the actual length of the payload will be assigned to
 *                 this field.
 * block_t* next - a next vacant block. */
typedef struct {
	uint32_t index : 12;	/*<! up to 4096 addressable blocks. */
	int32_t  size  : 20;	/*<! up to 0x07FFFF (524,287) bytes of memory. */
} block_t;

static uint8_t heap[HEAP_STATIC_SIZE] __attribute__ ((aligned (4)));
static uint16_t blockIdx = 0;


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
	HEAP_INIT_SIZE = (int32_t)(0 - (HEAP_STATIC_SIZE - sizeof(block_t)));
	HEAP_INIT_INDX = blockIdx++;
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
	uint32_t heapBound = (uint32_t)(heap + HEAP_STATIC_SIZE);

	requested = ALLIGN4(requested);
	// Starting from the first block, iterate through the entire heap
	for (; (uint32_t)block < heapBound; block = GET_NEXT_BLOCK(block, blockSize)) {
		if (block->size > 0) {
			blockSize = block->size;
			continue;
		}
		
		blockSize = 0 - block->size; // Get the number of available space in this block

		if (requested == blockSize) {
			block->size = requested;
			block->index = (blockIdx++ & 0x0FFF);
			result = (void*)((uint8_t*)block + sizeof(block_t));
			break;
		} else if (blockSize > requested + sizeof(block_t)) {
			
			block->size = requested;
			block->index = (blockIdx++ & 0x0FFF);
			block_t* nextFreeBlk = GET_NEXT_BLOCK(block, requested);
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
	block_t* block = (block_t*)heap;
	uint32_t blockSize = 0;
	uint32_t heapBound = (uint32_t)(heap + HEAP_STATIC_SIZE);
	int16_t blkIdx = GET_BLOCK_INDEX(ptr);

	for (; (uint32_t)block < heapBound; block = GET_NEXT_BLOCK(block, blockSize)) {
		if (block->index == blkIdx)
			break;

		blockSize = (block->size < 0) ? (0 - block->size) : block->size;
	}

	// Bail out either we reached the upper bound or this block wasn't allocated.
	if ((uint32_t)block >= heapBound || block->size < 0)
		return;

	block->size  = 0 - block->size;	// mark as free.
	block->index = 0x00;
	blockIdx--;

	// Starting from the first block, perform defragmentation.
	block_t* curr = (block_t*)heap;
	block_t* prev;
	while ((uint32_t)curr < heapBound) {
		if (curr->size < 0) {
			prev = curr;									// store 'curr' block in 'prev' variable.
			curr = GET_NEXT_BLOCK(curr, 0 - curr->size);	// get the next one.
			if ((uint32_t)curr >= heapBound)				// We've reached the end. Bail out.
				break;
			
			if (curr->size >= 0)							// This just fetched block is in use and thus can't be merged.
				continue;									// Skip this iteration.

			// To this point, we have two subsequent unused blocks. Merge them.
			prev->size -= sizeof(block_t) - curr->size;		// Add to the 'prev' unused block the length of the  'curr' unused one.
			prev = curr;									// Store the reference to unused block and 
			curr = GET_NEXT_BLOCK(curr, 0 - curr->size);	// then advance to the subsequent one.
			prev->size = 0;									// now clear any evidence about once existed unused block.
			prev->index = 0x00;
		} else {
			curr = GET_NEXT_BLOCK(curr, curr->size); // get the next one.
		}
	}
}
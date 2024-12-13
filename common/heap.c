#include "heap.h"

#ifndef NULL
#	define NULL ((void*)0)
#endif

#define HEAP_STATIC_SIZE       			(1 * 64)	/*<! The max value is 0x07FFFF (524,287 bytes). */
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
	uint32_t index : 12;	/*<! up to 4096 addressable blocks (RFU). */
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
heapAlloc(uint32_t newSize)
{
	void* result = NULL;
	block_t* currBlock = (block_t*)heap;
	uint32_t blockSize = 0;
	uint32_t heapBound = (uint32_t)(heap + HEAP_STATIC_SIZE);

	newSize = ALLIGN4(newSize);
	// Starting from the first block, iterate through the entire heap
	for (; (uint32_t)currBlock < heapBound; currBlock = GET_NEXT_BLOCK(currBlock, blockSize)) {
		if (currBlock->size > 0) {
			blockSize = currBlock->size;
			continue;
		}
		
		blockSize = 0 - currBlock->size; // Get the number of available space in this block

		if (newSize == blockSize) {

			currBlock->size = newSize;
			currBlock->index = (blockIdx++ & 0x0FFF);
			result = (void*)((uint8_t*)currBlock + sizeof(block_t));
			break;
		} else if (blockSize > newSize + sizeof(block_t)) {
			
			currBlock->size = newSize;
			currBlock->index = (blockIdx++ & 0x0FFF);
			block_t* nextFreeBlk = GET_NEXT_BLOCK(currBlock, newSize);
			blockSize  -= newSize + sizeof(block_t);
			nextFreeBlk->size = 0 - blockSize;
			
			result = (void*)((uint8_t*)currBlock + sizeof(block_t));
			break;
		}
	}

	return result;
}

void*
heapRealloc(void* ptr, uint32_t newSize)
{
	block_t* currBlock = (block_t*)heap;
	uint32_t blockSize = 0;
	uint32_t heapBound = (uint32_t)(heap + HEAP_STATIC_SIZE);
	int16_t blkIdx = GET_BLOCK_INDEX(ptr);
	newSize = ALLIGN4(newSize);

	// Nothing to do.
	if (newSize == 0) {
		goto _done;
	}

	for (; (uint32_t)currBlock < heapBound; currBlock = GET_NEXT_BLOCK(currBlock, blockSize)) {
		if (currBlock->index == blkIdx)
			break;

		blockSize = (currBlock->size < 0) ? (0 - currBlock->size) : currBlock->size;
	}

	// A block to be resized not found. Allocate new one and return.
	// Making it more clear: during the search we've reached the end OR block is found,
	// but it wasn't allocated beforehand.
	if ((uint32_t)currBlock >= heapBound || currBlock->size < 0) {
		ptr = heapAlloc(newSize);
		goto _done;
	}

	// Requested length equals to actual one. Just return.
	if (currBlock->size == newSize) {
		goto _done;
	}

	// User wants to increase the size of memory block.
	if (newSize > currBlock->size) {

		block_t* donor = GET_NEXT_BLOCK(currBlock, currBlock->size);
		int32_t extentLen = newSize - currBlock->size;  // the number of bytes we want to borrow from adjacent block.

		// is donor block vacant AND has enough space?
		if ((donor->size < 0) && ((0 - donor->size) >= extentLen)) {

			// store the state of donor block
			int32_t donorSiz = donor->size;
			int32_t donorIdx = donor->index;

			// clean up donor's state.
			donor->index = 0;
			donor->size = 0;

			// shift donor's starting offset.
			donor = (block_t*)((uint8_t*)donor + extentLen);

			// FIXME: each block that ends up with 'size == 0' creates a hole in the heap.
			// Consider the picture below (each cell represents 4-byte of block_t:size field):
			//   ____________________________________block N-2 (before: 4; after: 8)
			//  |                           _________block N-1 (before: 4; after: 0)
			//  |                          |   ______block N
			//  |                          |  |
			// | 8|  |  |  |  |  |  |  |  | 0|12|...|  |
			// Here, the four bytes of 'N-1 block' - dedicated for 'block_t' structure itself - are left unreachable.
			// Moreover, both heapAlloc() and heapFree() eventually fall into infinite loop trying to step over to the next block
			// by means of zero-length offset. 
			// May be a good solution is wipe out such a block and assign its 4 bytes to the previous one, even if
			// the latter is already in use.
			donor->size = donorSiz - extentLen; // reduce its size value to borrowed length.
			donor->index = donorIdx;

			currBlock->size = newSize;
		}
		// Donor (adjacent) block is either occupied or hasn't enough space. TODO:
		// 1. create a new block; 2. copy the content from the previos block; 3. delete previous block.
		else {

		}
	}
	// User wants to reduce the size of memory block.
	else {

	}

_done:
	return ptr;
}

void
heapFree(void* ptr)
{
	block_t* currBlock = (block_t*)heap;
	uint32_t blockSize = 0;
	uint32_t heapBound = (uint32_t)(heap + HEAP_STATIC_SIZE);
	int16_t blkIdx = GET_BLOCK_INDEX(ptr);

	for (; (uint32_t)currBlock < heapBound; currBlock = GET_NEXT_BLOCK(currBlock, blockSize)) {
		if (currBlock->index == blkIdx)
			break;

		blockSize = (currBlock->size < 0) ? (0 - currBlock->size) : currBlock->size;
	}

	// Bail out either we reached the upper bound or this block wasn't allocated.
	if ((uint32_t)currBlock >= heapBound || currBlock->size < 0) {
		return;
	}

	currBlock->size  = 0 - currBlock->size;							// mark as free.
	currBlock->index = 0x00;
	blockIdx--;

	currBlock = (block_t*)heap;										// Starting from the first block, perform defragmentation.
	block_t* prev;
	while ((uint32_t)currBlock < heapBound) {
		if (currBlock->size > 0) {
			currBlock = GET_NEXT_BLOCK(currBlock, currBlock->size);
			continue;
		}

		prev = currBlock;											// store 'currBlock' block in 'prev' variable.
		currBlock = GET_NEXT_BLOCK(currBlock, 0 - currBlock->size);	// get a next one.
		if ((uint32_t)currBlock >= heapBound)						// We've reached the end. Bail out.
			break;
		
		if (currBlock->size >= 0)									// The next block is in use, thus can't be merged. Skip this iteration
			continue;												// 
																	// To this point, we have two subsequent unused blocks. Merge them.
		prev->size -= sizeof(block_t) - currBlock->size;			// Add to the 'prev' unused block the length of the 'currBlock' (which is vacant too).
		prev = currBlock;											// Store the reference to unused block and 
		currBlock = GET_NEXT_BLOCK(currBlock, 0 - currBlock->size);	// then advance to the subsequent one.
		prev->size = 0;												// now clear any evidence about once existed unused block.
		prev->index = 0x00;
	}
}
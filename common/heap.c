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

static void
copyRange(uint8_t* dest, uint8_t* source, uint32_t length)
{
	for (uint32_t i = 0; i < length; ++i) {
		dest[i] = source[i];
	}
}

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
	int16_t blkIdx;

	if (ptr == NULL) {
		goto _done;
	}

	// Attempt to access to address which is out of heap range.
	if ((uint32_t)ptr < (uint32_t)currBlock || (uint32_t)ptr > heapBound) {
		ptr = NULL;
	}

	// Nothing to do.
	if (newSize == 0) {
		goto _done;
	}

	blkIdx = GET_BLOCK_INDEX(ptr);
	newSize = ALLIGN4(newSize);

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

	// Requested length equals or less that actual one. Just return. The problem with reducing the size is as follows:
	// Consider we want to decrease the length from 8 to 4. Then attempt to calculate an offset to the next block in
	// chain using updated size field will end up with the pointer to an empty space in memory, having no chance to
	// reach a next block in the chain, which is 4 bytes ahead.
	if (currBlock->size == newSize || currBlock->size > newSize) {
		goto _done;
	}

	block_t* donor = GET_NEXT_BLOCK(currBlock, currBlock->size);
	uint32_t extentLen = newSize - currBlock->size;  // the number of bytes we want to borrow from adjacent block.

	// is donor block vacant AND has enough space?
	if ((donor->size < 0) && ((0 - donor->size) >= extentLen)) {

		// store the state of donor block
		int32_t donorSiz = donor->size;
		int32_t donorIdx = donor->index;

		// clean up donor's state. These four bytes go under 'currBlock'.
		donor->index = 0;
		donor->size  = 0;

		// advance donor's starting offset.
		donor           = (block_t*)((uint8_t*)donor + extentLen);
		donor->size     = donorSiz - extentLen; // reduce the value of 'size' field to the borrowed length.
		donor->index    = donorIdx;
		currBlock->size = newSize;

		// It may hapen that donor gave us all the space he had and now has no free space anymore.
		// Thus clear this region and pass these four bytes over to the currBlock too.
		if (donor->size == 0) {
			donor->size = 0;
			currBlock->size += sizeof(block_t);
		}
	}
	else { // Donor (adjacent) block is either occupied or hasn't enough space.
		ptr = heapAlloc(newSize);
		copyRange((uint8_t*)ptr, (uint8_t*)currBlock, currBlock->size);
		heapFree((void*)currBlock);
		goto _done;
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
	int16_t blkIdx;

	if (ptr == NULL) {
		return;
	}

	// Attempt to free an address which is out of heap range.
	if ((uint32_t)ptr < (uint32_t)currBlock || (uint32_t)ptr > heapBound) {
		return;
	}

	blkIdx = GET_BLOCK_INDEX(ptr);

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
																	// To this point, we have two adjacent unused blocks. Merge them.
		prev->size -= sizeof(block_t) - currBlock->size;			// Add to the 'prev' unused block the length of the 'currBlock' (which is vacant too).
		prev = currBlock;											// Store the reference to unused block and 
		currBlock = GET_NEXT_BLOCK(currBlock, 0 - currBlock->size);	// then advance to the subsequent one.
		prev->size = 0x00;											// now clear any evidence about once existed unused block.
		prev->index = 0x00;
	}
}
#include "heap.h"
#include <string.h>

#ifndef NULL
#	define NULL ((void*)0)
#endif

#define HEAP_GET_SIZE(arr)         		((block_t*)arr)->size
#define HEAP_GET_NEXT(arr)         		((block_t*)arr)->next
#define ALLIGN4(value)         			(((value + 3) >> 2) << 2)

uint8_t heap[HEAP_STATIC_SIZE] __attribute__ ((aligned (4)));
static uint32_t heapBound;

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

	/* The negative value designates a free block.
	 * Thus, at initial stage the heap array consists of one huge free block. */
	HEAP_GET_SIZE(pHeap) = (int32_t)(0 - (HEAP_STATIC_SIZE - sizeof(block_t)));
	HEAP_GET_NEXT(pHeap) = NULL;

	// The last consistent address shall encompass the block size too.
	heapBound = (uint32_t)((heap + HEAP_STATIC_SIZE) - sizeof(block_t));
}

/**
 * Returns a pointer to address which is 'offset' bytes ahead from 'currBlk'.
 * @param block_t* currBlk - an address to start from
 * @param uint32_t offset - number of bytes to step over.
 * @returns block_t* - the pointer to address or NULL.
 */
static block_t*
getNext(block_t* currBlk, uint32_t offset)
{
	if ((uint32_t)currBlk + offset + sizeof(block_t) >= heapBound)
		return NULL;
	else
		return (block_t*)((uint8_t*)currBlk + sizeof(block_t) + (offset));
}

static uint8_t
isOutOfRange(void* ptr)
{
	return ((uint32_t)ptr < (uint32_t)heap || (uint32_t)ptr >= heapBound) ? 1 : 0;
}

static block_t*
findBlock(block_t* ptr)
{
	block_t* currBlk = (block_t*)heap;
	ptr = (block_t*)((uint8_t*)ptr - sizeof(block_t));

	// Attempt to access an address which is out of heap range.
	if ((ptr == NULL) || isOutOfRange(ptr)) {
		return NULL;
	}

	do {
		if (currBlk == ptr)
			break;
		
		currBlk = currBlk->next;
	} while (currBlk != NULL);

	return currBlk;
}

static block_t*
findVacant(void)
{
	block_t* currBlk = (block_t*)heap;

	do {
		if (currBlk->size < 0)
			break;
		
		currBlk = currBlk->next;
	} while (currBlk != NULL);

	return currBlk;
}

static void*
takeSuitableBlock(block_t* currBlk, uint32_t actualSize)
{
	currBlk->size = actualSize;
	return (void*)((uint8_t*)currBlk + sizeof(block_t));
	
}

static void*
splitUpMemory(block_t* currBlk, uint32_t actualSize, uint32_t newSize, block_t* next)
{
	next = getNext(currBlk, newSize);
	if (next != NULL) {								// if 'next' isn't null, then update its size length.
		actualSize -= newSize + sizeof(block_t);	// subtract the block size and reserved space for the new block.
		next->size  = 0 - actualSize;				// encode new value.
		next->next  = currBlk->next;				// Now, the 'next' points to one, which was referenced by previous block
	}

	currBlk->next = next;
	currBlk->size = newSize;
	return (void*)((uint8_t*)currBlk + sizeof(block_t));
}

void*
heapAlloc(uint32_t newSize)
{
	uint32_t actualSize = 0;
	void* ptr        = NULL;
	block_t* next    = NULL;
	block_t* currBlk = findVacant();
	
	if (newSize == 0)	// Nothing to do.
		return ptr;
	
	newSize = ALLIGN4(newSize);

	for (; currBlk != NULL; currBlk = currBlk->next) {
		
		if (currBlk->size > 0)				// proceed to next block if this one is occupied.
			continue;			
		
		actualSize = (0 - currBlk->size);	// Decode the size of block.

		if (newSize == actualSize) {		// Block's size matches exactly.
			ptr = takeSuitableBlock(currBlk, actualSize);
			break;
		}									// Block's size is greater than required.
		else if (newSize + sizeof(block_t) < actualSize) {
			ptr = splitUpMemory(currBlk, actualSize, newSize, next);
			break;
		}
	}

	return ptr;
}

static void*
borrowMemory(block_t* currBlk, block_t* donor, uint32_t extentLen, uint32_t newSize)
{
	// store the state of donor block
	int32_t donorSize  = 0 - donor->size;
	block_t* donorNext = donor->next;

	// clean up donor's state. These four bytes go under 'currBlk'.
	donor->next = NULL;
	donor->size = 0;

	// advance donor's starting offset.
	donor       = (block_t*)((uint8_t*)donor + extentLen);
	donor->size = 0 - (donorSize - extentLen); // reduce the value of 'size' field for the borrowed length.
	
	// It may hapen that donor gave us all the space he had and now has no free space anymore.
	// Thus clear this region and pass these four bytes over to the currBlk too.
	if (donor->size == 0) {
		donor->next   = NULL;
		currBlk->size = newSize + sizeof(block_t); // add the bytes dedicated for 'block_t' struct to currBlock
		currBlk->next = donorNext; // make the currBlk points to one, being pointed by donor.
	} else {
		donor->next   = donorNext;
		currBlk->size = newSize;
		currBlk->next = donor;
	}

	return (void*)((uint8_t*)currBlk + sizeof(block_t));
}

void*
heapRealloc(void* ptr, uint32_t newSize)
{
	block_t* currBlk = NULL;
	
	do {
		if (newSize == 0)	// Nothing to do.
			break;

		newSize = ALLIGN4(newSize);
		currBlk = findBlock(ptr);

		// A block to be resized not found. Allocate new one and return.
		if (currBlk == NULL || currBlk->size < 0) {
			ptr = heapAlloc(newSize);
			break;
		}

		// Current inplementation doesn't handle requests to reduce the allocated size.
		if (currBlk->size >= newSize) {
			break;
		}

		block_t* donor     = currBlk->next;
		uint32_t extentLen = newSize - currBlk->size;  // the number of bytes we want to borrow from adjacent block.

		// The donor is NULL, occupied or hans't enougth space
		if ((donor == NULL) || (donor->size > 0) || ((0 - donor->size) < extentLen)) {
			ptr = heapAlloc(newSize);	// allocate new space
			if (ptr == NULL)			// Are we happy?
				break;					// Nope, there is no memory.
			
			memcpy((uint8_t*)ptr, (uint8_t*)currBlk + sizeof(block_t), currBlk->size);	// transfer data from previous memory region
			heapFree((void*)currBlk + sizeof(block_t));									// free previous block
			break;
		}
	
		ptr = borrowMemory(currBlk, donor, extentLen, newSize);
	} while (0);

	return ptr;
}

static void
defragging(void)
{
	block_t* currBlk = (block_t*)heap;	// Starting from the first block, perform defragmentation.
	block_t* nextBlk = NULL;
	
	for (; currBlk != NULL; currBlk = currBlk->next) {
		if (currBlk->size > 0)		// this block is in use. Proceed to next one.
			continue;

		nextBlk = currBlk->next;	// take the reference to the next one.
		if (nextBlk == NULL)		// There is no more blocks ahead. Bail out.
			break;
		
		if (nextBlk->size >= 0)		// if the next block is in use, then skip this iteration.
			continue;
		
		// add to current block the length of the next vacant block.
		currBlk->size -= sizeof(block_t) - nextBlk->size;
		currBlk->next  = nextBlk->next;	// store the reference to a block, which goes right after 
										// one we are about to merge.
		nextBlk->size = 0;				// clean up staled service data
		nextBlk->next = NULL;
	}
}

void
heapFree(void* ptr)
{
	block_t* currBlk = NULL;
	
	if (ptr == NULL) {
		return;
	}

	currBlk = findBlock(ptr);

	// This block wasn't allocated before. Bail out.
	if (currBlk == NULL || (currBlk->size < 0)) {
		return;
	}

	memset((uint8_t*)currBlk + sizeof(block_t), 0x00, currBlk->size);	// clean up and
	currBlk->size = 0 - currBlk->size;									// mark as vacant.

	defragging();
}
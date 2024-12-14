#include "heap.h"
#include <string.h>

#ifndef NULL
#	define NULL ((void*)0)
#endif

#define HEAP_STATIC_SIZE				(1 * 48)
#define HEAP_GET_SIZE(arr)         		((block_t*)arr)->size
#define HEAP_GET_NEXT(arr)         		((block_t*)arr)->next
#define ALLIGN4(value)         			(((value + 3) >> 2) << 2)

static uint8_t heap[HEAP_STATIC_SIZE] __attribute__ ((aligned (4)));
static uint32_t heapBound;

static block_t*
getNext(block_t* curr, uint32_t offset)
{
	if ((uint32_t)curr + offset + sizeof(block_t) >= heapBound)
		return NULL;
	else
		return (block_t*)((uint8_t*)curr + sizeof(block_t) + (offset));
}

static block_t*
findBlock(block_t* ptr)
{
	block_t* curr = (block_t*)heap;
	ptr = (block_t*)((uint8_t*)ptr - sizeof(block_t));

	// Attempt to free an address which is out of heap range.
	if ((uint32_t)ptr < (uint32_t)curr || (uint32_t)ptr >= heapBound) {
		return NULL;
	}

	do {
		if (curr == ptr)
			break;
		
		curr = curr->next;
	} while (curr != NULL);

	return curr;
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
	heapBound = (uint32_t)(heap + HEAP_STATIC_SIZE);
}

/**
 * Allocate memory from the heap.
 */
void*
heapAlloc(uint32_t newSize)
{
	block_t* currBlk   = NULL;
	uint32_t blockSize = 0;

	newSize = ALLIGN4(newSize);
	currBlk = findVacant();

_again:
	if (currBlk == NULL) {
		return NULL;
	}

	blockSize = (0 - currBlk->size); // Decode the size of block.

	if (newSize == blockSize) {
		currBlk->size = newSize;
		return (void*)((uint8_t*)currBlk + sizeof(block_t));
	}

	if (newSize + sizeof(block_t) < blockSize) {
		
		block_t* next = getNext(currBlk, newSize);
		currBlk->size = newSize;

		if (next == NULL) {
			currBlk->next = NULL;
		}
		else {
			currBlk->next = next;
			blockSize    -= newSize + sizeof(block_t);
			next->size    = 0 - blockSize;
		}
		
		return (void*)((uint8_t*)currBlk + sizeof(block_t));
	} else {
		currBlk = currBlk->next;
		goto _again;
	}
}

void*
heapRealloc(void* ptr, uint32_t newSize)
{
#if(0)
	block_t* currBlk = (block_t*)heap;
	uint32_t blockSize = 0;
	uint32_t heapBound = (uint32_t)(heap + HEAP_STATIC_SIZE);
	int16_t blkIdx;

	if (ptr == NULL) {
		goto _done;
	}

	// Attempt to access to address which is out of heap range.
	if ((uint32_t)ptr < (uint32_t)currBlk || (uint32_t)ptr > heapBound) {
		ptr = NULL;
	}

	// Nothing to do.
	if (newSize == 0) {
		goto _done;
	}

	blkIdx = GET_BLOCK_INDEX(ptr);
	newSize = ALLIGN4(newSize);

	for (; (uint32_t)currBlk < heapBound; currBlk = GET_NEXT_BLOCK(currBlk, blockSize)) {
		if (currBlk->index == blkIdx)
			break;

		blockSize = (currBlk->size < 0) ? (0 - currBlk->size) : currBlk->size;
	}

	// A block to be resized not found. Allocate new one and return.
	if ((uint32_t)currBlk >= heapBound || currBlk->size < 0) {
		ptr = heapAlloc(newSize);
		goto _done;
	}

	// Requested length equals or less that actual one. Just return. The problem with reducing the size is as follows:
	// Consider we want to decrease the length from 8 to 4. Then attempt to calculate an offset to the next block in
	// chain using updated size field will end up with the pointer to an empty space in memory, having no chance to
	// reach a next block in the chain, which is 4 bytes ahead.
	if (currBlk->size == newSize || currBlk->size > newSize) {
		goto _done;
	}

	block_t* donor = GET_NEXT_BLOCK(currBlk, currBlk->size);
	uint32_t extentLen = newSize - currBlk->size;  // the number of bytes we want to borrow from adjacent block.

	// is donor block vacant AND has enough space?
	if ((donor->size < 0) && ((0 - donor->size) >= extentLen)) {

		// store the state of donor block
		int32_t donorSiz = donor->size;
		int32_t donorIdx = donor->index;

		// clean up donor's state. These four bytes go under 'currBlk'.
		donor->index = 0;
		donor->size  = 0;

		// advance donor's starting offset.
		donor           = (block_t*)((uint8_t*)donor + extentLen);
		donor->size     = donorSiz - extentLen; // reduce the value of 'size' field to the borrowed length.
		donor->index    = donorIdx;
		currBlk->size = newSize;

		// It may hapen that donor gave us all the space he had and now has no free space anymore.
		// Thus clear this region and pass these four bytes over to the currBlk too.
		if (donor->size == 0) {
			donor->size = 0;
			currBlk->size += sizeof(block_t);
		}
	}
	else { // Donor (adjacent) block is either occupied or hasn't enough space.
		ptr = heapAlloc(newSize);
		copyRange((uint8_t*)ptr, (uint8_t*)currBlk, currBlk->size);
		heapFree((void*)currBlk);
		goto _done;
	}

_done:
#endif
	return ptr;
}

void
heapFree(void* ptr)
{
	block_t* prev;
	block_t* currBlk;
	
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

	currBlk = (block_t*)heap;			// Starting from the first block, perform defragmentation.
	while (currBlk != NULL) {
		if (currBlk->size > 0) {		// this block is in use.
			currBlk = currBlk->next;	// Proceed to next one.
			continue;
		}

		prev = currBlk;					// store 'currBlk' in 'prev';
		currBlk = currBlk->next;		// now 'currBlk' points to 'next' one.

		if (currBlk == NULL) {
			break;						// We've reached the end, i.e. there is no vacant blocks ahead to be merged with. Bail out.
		}

		if (currBlk->size >= 0)			// The 'next' block is in use, thus can't be merged. Skip this iteration
			continue;					//
		
		prev->size -= sizeof(block_t) - currBlk->size;	// Add to the 'prev' unused block the length of the 'currBlk' (which is vacant too).
		prev = currBlk;									// Store the reference to unused block.
		currBlk = currBlk->next;						// Advance to the subsequent one.
		prev->size = 0x00;								// now clear any evidence about once existed unused block.
		prev->next = 0x00;
	}
}
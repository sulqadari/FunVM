#if !defined(FUNVM_MEM_MANAGER)
#	define FUNVM_MEM_MANAGER
#endif
#include "common.h"
#include "memory.h"

#define ARRAY_SIZE 5

#define _REALLOCATE(ptr, fill, len)					\
do {												\
	ptr = fvm_realloc(ptr, len);					\
	if (ptr != NULL) {								\
		memset(ptr, fill, len);						\
		break;										\
	}												\
													\
	printf("Error: failed to reallocate memory.\n"	\
	"line: %d\n", __LINE__);						\
	exit(1);										\
} while (0)											

static void
releaseMem(uint8_t** ptrs)
{
	for (uint32_t i = 0; i < ARRAY_SIZE; ++i) {
		fvm_free(ptrs[i]);
	}
}

int
main(int argc, char* argv[])
{
	uint8_t* pointers[ARRAY_SIZE] = {0};
	
	heapInit();
 	_REALLOCATE(pointers[0], 1, 8);
	_REALLOCATE(pointers[1], 2, 8);
	_REALLOCATE(pointers[2], 3, 8);
	fvm_free(pointers[1]);
	
	_REALLOCATE(pointers[1], 4, 8);
	fvm_free(pointers[1]);
	
	_REALLOCATE(pointers[1], 5, 4);

	releaseMem(pointers);
	printf("test\n\t%s\nresult\n\tSUCCESS\n", __FILE__);
}
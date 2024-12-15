#if !defined(FUNVM_MEM_MANAGER)
#	define FUNVM_MEM_MANAGER
#endif
#include "common.h"
#include "memory.h"


#define ASSERT_NULL(expr)								\
	do {												\
		if (expr == NULL) {								\
			printf("ERROR: at line %d\n", __LINE__);	\
		}												\
	} while (0)

int
main(int argc, char* argv[])
{
	uint8_t* ptr[5];
	heapInit();

	ptr[0] = fvm_alloc(4);
	ASSERT_NULL(ptr[0]);
	memset(ptr[0], 1, 4);

	ptr[0] = fvm_realloc(ptr[0], 8);
	ASSERT_NULL(ptr[0]);
	memset(ptr[0], 1, 8);

	fvm_free(ptr[0]);
	// fvm_free(ptr[1]);
	// fvm_free(ptr[2]);
}
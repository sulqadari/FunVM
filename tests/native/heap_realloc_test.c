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

static void
allocateBunch(uint8_t* ptr[5])
{
	for (uint8_t i = 1; i < 4; ++i) {
		ptr[i - 1] = fvm_alloc(i * 4);
		ASSERT_NULL(ptr[i - 1]);
		memset(ptr[i - 1], i, i * 4);
	}
}

int
main(int argc, char* argv[])
{
	uint8_t* ptr[5];
	heapInit();

	allocateBunch(ptr);

	ptr[0] = fvm_realloc(ptr[0], 8);
	ASSERT_NULL(ptr[0]);
	memset(ptr[0], 1, 8);

	fvm_free(ptr[0]);
	fvm_free(ptr[1]);
	fvm_free(ptr[2]);

	printf("test\n\t%s\nresult\n\tSUCCESS\n", __FILE__);
}
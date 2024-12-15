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

static void
_01allocateBunch(uint8_t* ptr[5])
{
	for (int8_t i = 3; i >= 1; --i) {
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
	fvm_free(ptr[0]);
	fvm_free(ptr[1]);
	fvm_free(ptr[2]);
	
	allocateBunch(ptr);
	fvm_free(ptr[2]);
	fvm_free(ptr[1]);
	fvm_free(ptr[0]);

	allocateBunch(ptr);
	fvm_free(ptr[2]);
	fvm_free(ptr[0]);
	fvm_free(ptr[1]);

	allocateBunch(ptr);
	fvm_free(ptr[1]);
	fvm_free(ptr[0]);
	fvm_free(ptr[2]);

	allocateBunch(ptr);
	fvm_free(ptr[1]);
	fvm_free(ptr[2]);
	fvm_free(ptr[0]);

	allocateBunch(ptr);
	fvm_free(ptr[0]);
	fvm_free(ptr[2]);
	fvm_free(ptr[1]);

	_01allocateBunch(ptr);
	fvm_free(ptr[0]);
	fvm_free(ptr[1]);
	fvm_free(ptr[2]);
	
	_01allocateBunch(ptr);
	fvm_free(ptr[2]);
	fvm_free(ptr[1]);
	fvm_free(ptr[0]);

	_01allocateBunch(ptr);
	fvm_free(ptr[2]);
	fvm_free(ptr[0]);
	fvm_free(ptr[1]);

	_01allocateBunch(ptr);
	fvm_free(ptr[1]);
	fvm_free(ptr[0]);
	fvm_free(ptr[2]);

	_01allocateBunch(ptr);
	fvm_free(ptr[1]);
	fvm_free(ptr[2]);
	fvm_free(ptr[0]);

	_01allocateBunch(ptr);
	fvm_free(ptr[0]);
	fvm_free(ptr[2]);
	fvm_free(ptr[1]);
}
#if !defined(FUNVM_MEM_MANAGER)
#	define FUNVM_MEM_MANAGER
#endif
#include "common.h"
#include "memory.h"
#include "object.h"
#include "object_pool.h"
#include "globals.h"

static void
serialize(char* outputPath)
{
	FILE* file;
	char binFileName[256];

	if (outputPath == NULL) {
		fprintf(stderr, "TEST 'portable_create_test.c': Failed to serialize file because it's null.\n");
		exit(74);
	}
	
	sprintf(binFileName, "%s%s", outputPath, "portable_create_test.bin");

	file = fopen(binFileName, "wb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't create binary file '%s'.\n", binFileName);
		exit(74);
	}

	fwrite(objPool.values, sizeof(uint8_t), objPool.valuesSize, file);
	fclose(file);
}

int
main(int argc, char* argv[])
{
	heapInit();
	initObjPool();

	ObjString* string = copyString("123", 3);

	freeObjects();
	serialize(argv[1]);
	freeObjPool();
}
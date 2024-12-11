#include "common.h"
#include "compiler.h"
#include "source_handler.h"
#include "memory.h"

static void
usage(void)
{
	printf("Usage:\n\tfunvmc <source.fn>\n\tfunvm source.fnb\n");
	exit(1);
}

int
main(int argc, char* argv[])
{
	if (argc != 2)
		usage();
	
	heapInit();
	uint8_t* ptr[5];
	ptr[0] = fvm_alloc(4);
	ptr[1] = fvm_alloc(5);
	ptr[2] = fvm_alloc(9);
	ptr[3] = fvm_alloc(11);
	ptr[4] = fvm_alloc(13);
	fvm_free(ptr[4]);
	fvm_free(ptr[3]);
	fvm_free(ptr[2]);
	fvm_free(ptr[1]);
	fvm_free(ptr[0]);
	// char* source;
	// ByteCode bCode;
	
	// source = readSourceFile(argv[1]);
	// bool res = compile(source, &bCode);
	// if (!res) {
	// 	printf("Failed to compile...\n");
	// 	free(source);
	// 	exit(1);
	// }

	// serializeByteCode(argv[1], &bCode);
	// freeByteCode(&bCode);
	// free(source);
}
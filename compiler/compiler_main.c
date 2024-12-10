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
	uint8_t* ptr;
	ptr = fvm_alloc(4);
	ptr = fvm_alloc(8);
	ptr = fvm_alloc(12);
	ptr = fvm_alloc(16);
	ptr = fvm_alloc(4);
	ptr = fvm_alloc(20);
	ptr = fvm_alloc(24);
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
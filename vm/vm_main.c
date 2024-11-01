#include "common.h"
#include "vm.h"
#include "source_handler.h"

static void
usage(void)
{
	printf("Usage:\n\tfunvm binary.fnb\n");
	exit(1);
}

int
main(int argc, char* argv[])
{
	if (argc != 2)
		usage();
	
	ByteCode bCode;

	initByteCode(&bCode);
	deserializeByteCode(argv[1], &bCode);

	initVM();
	interpret(&bCode);
	
	freeByteCode(&bCode);
	freeVM();
	return (0);
}
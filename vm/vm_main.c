#include "common.h"
#include "vm.h"
#include "source_handler.h"

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
	
	ByteCode bCode;
	
	deserializeByteCode(argv[1], &bCode);
	initVM();
	interpret(&bCode);
	freeVM();
	return (0);
}
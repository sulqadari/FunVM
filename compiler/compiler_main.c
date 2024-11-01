#include "common.h"
#include "compiler.h"
#include "source_handler.h"

static void
usage(void)
{
	printf("Usage:\n\tfunvmc <source.fn>\n");
	exit(1);
}

int
main(int argc, char* argv[])
{
	if (argc != 2)
		usage();
	
	char* source;
	ByteCode bCode;
	
	source = readSourceFile(argv[1]);
	bool res = compile(source, &bCode);
	if (!res) {
		printf("Failed to compile...\n");
		free(source);
		exit(1);
	}

	serializeByteCode(argv[1], &bCode);
	freeByteCode(&bCode);
	free(source);
}
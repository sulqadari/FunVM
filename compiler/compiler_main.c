#include "common.h"
#include "compiler.h"
#include "source_handler.h"

static void
usage(void)
{
	printf("Usage:\n\tfunvmc <source.fn>\n\tfunvm source.fnb");
	exit(1);
}

int
main(int argc, char* argv[])
{
	if (argc != 2)
		usage();
	
	const char* source;
	ByteCode bCode;
	
	source = readSourceFile(argv[1]);
	compile(source, &bCode);
	saveByteCodeFile(argv[1], &bCode);
}
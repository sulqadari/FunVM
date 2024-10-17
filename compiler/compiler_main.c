#include "common.h"
#include "compiler.h"
#include "vm.h"

int
main(int argc, char* argv[])
{
	if (argc != 2)
		usage();
	
	const char* source;
	Bytecode bCode;
	
	source = readSourceFile(argv[1]);
	compile(source, &bCode);
	storeBytecodeFile(argv[1], &bCode);
}
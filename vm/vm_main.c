#include "common.h"
#include "vm.h"

int
main(int argc, char* argv[])
{
	ByteCode bCode;
	initVM();
	interpret(&bCode);
	freeVM();
	return (0);
}
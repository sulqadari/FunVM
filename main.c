#include "funvmConfig.h"

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "debug.h"
#include "vm.h"
#include "utils.h"

int
main(int argc, char *argv[])
{
	VM vm;
	printf("\n*** Interpreter ***\n");
	initVM(&vm);
	
	// if (1 == argc)
	// 	repl();
	// else if (2 == argc)
	// 	runFile(argv[1]);
	// else {
	// 	fprintf(stderr, "Usage: FunVM.exe [path/to/src]\n");
	// 	exit(64);
	// }
	// TODO: find workaround to launch FunVM with argument in debug mode
	// instead of using this kludge.
	runFile("./test_scripts/functional/06_and.fn");
	return (0);
}
#include "funvmConfig.h"

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "debug.h"
#include "vm.h"
#include "utils.h"

static int
case_1(int argc, char *argv[])
{
	VM vm;
	printf("*** Interpreter ***\n");
	initVM(&vm);
	
	if (1 == argc)
		repl(&vm);
	else if (2 == argc)
		runFile(&vm, argv[1]);
	else {
		fprintf(stderr, "Usage: test_scanner.c [path/to/src]\n");
		exit(64);
	}

	freeVM(&vm);
	return (0);
}

int
main(int argc, char *argv[])
{
	case_1(argc, argv);
	
	return (0);
}
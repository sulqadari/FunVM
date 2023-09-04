#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

void
repl(void)
{
	char line[1024];
	for (;;) {
		printf("> ");

		if (!fgets(line, sizeof(line), stdin)) {
			printf("\n");
			break;
		}

		interpret(line);
	}
}

static char*
readFile(const char* path)
{
	size_t fileSize;
	FILE* file;
	char* buffer;
	size_t bytesRead;

	file = fopen(path, "rb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't open source file '%s'.\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);	/* Move file prt to EOF. */
	fileSize = ftell(file);		/* How far we are from start of the file? */
	rewind(file);				/* Rewind file ptr back to the beginning. */

	buffer = malloc(fileSize + 1);
	if (NULL == buffer) {
		fprintf(stderr, "Failed to allocate memory for buffer "
		"for source file '%s'.\n", path);
		exit(74);
	}

	bytesRead = fread(buffer, sizeof(char), fileSize, file);
	if (bytesRead < fileSize) {
		fprintf(stderr, "Couldn't read source file '%s'.\n", path);
		exit(74);
	}

	buffer[fileSize] = '\0';
	fclose(file);
	
	return buffer;
}

void
runFile(const char* path)
{
	char* source = readFile(path);
	InterpretResult result = interpret(source);
	free(source);

	if (IR_COMPILE_ERROR == result)
		exit(65);
	
	if (IR_RUNTIME_ERROR == result)
		exit(70);
	
}
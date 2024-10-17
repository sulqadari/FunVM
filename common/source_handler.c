#include "common.h"
#include "vm.h"

char*
readSourceFile(const char* path)
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
saveBytecodeFile(const char* path, Bytecode* bCode)
{
	FILE* file;
	char file[256];
	sprintf(file, "%sbin", path);
	file = fopen(path, "wb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't create binary file '%s'.\n", file);
		exit(74);
	}

	fwrite(bCode, 1, sizeof(Bytecode), file);
	fwrite(bCode->code, 1, bCode->count, file);
	fwrite(&bCode->constants, 1, sizeof(ValueArray), file);
	fwrite(bCode->constants.values, 4, bCode->constants.count, file);
	fclose(file);
}
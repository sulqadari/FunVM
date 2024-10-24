#include "common.h"
#include "bytecode.h"

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
serializeByteCode(const char* path, ByteCode* bCode)
{
	FILE* file;
	char name[256];
	sprintf(name, "%sbin", path);
	file = fopen(name, "wb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't create binary file '%s'.\n", name);
		exit(74);
	}

	fwrite(&bCode->count, sizeof(uint32_t), 1, file);
	fwrite(&bCode->capacity, sizeof(uint32_t), 1, file);
	fwrite(bCode->code, sizeof(uint8_t), bCode->count, file);

	fwrite(&bCode->constants.count, sizeof(uint32_t), 1, file);
	fwrite(&bCode->constants.capacity, sizeof(uint32_t), 1, file);
	fwrite(bCode->constants.values, sizeof(int32_t), bCode->constants.count, file);
	fclose(file);
}

void
deserializeByteCode(const char* path, ByteCode* bCode)
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

	memcpy(&bCode->count, buffer, 4);
	bCode->capacity = bCode->count;
	memcpy(bCode->code, buffer + 8, bCode->count);

	memcpy(&bCode->constants.count, buffer + 12, 4);
	bCode->constants.capacity = bCode->constants.count;
	memcpy(bCode->constants.values, buffer+ 20, bCode->constants.count);
	fclose(file);
}
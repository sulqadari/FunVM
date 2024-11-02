#include "common.h"
#include "bytecode.h"

static void
allocateBuffer(uint8_t** buffer, uint32_t size, char* msg)
{
	*buffer = malloc(size + 1);
	if (NULL == buffer) {
		fprintf(stderr, "Failed to allocate memory for %s\n", msg);
		exit(74);
	}
}

uint8_t*
readSourceFile(const char* path)
{
	size_t fileSize;
	FILE* file;
	uint8_t* buffer;
	size_t bytesRead;

	file = fopen(path, "rb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't open source file '%s'.\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);	/* Move file prt to EOF. */
	fileSize = ftell(file);		/* How far we are from start of the file? */
	rewind(file);				/* Rewind file ptr back to the beginning. */

	allocateBuffer(&buffer, fileSize + 1, "source file");

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
	sprintf(name, "%sb", path);
	file = fopen(name, "wb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't create binary file '%s'.\n", name);
		exit(74);
	}

	fwrite(&bCode->count, sizeof(uint32_t), 1, file);
	fwrite(&bCode->capacity, sizeof(uint32_t), 1, file);

	fwrite(&bCode->constants.count, sizeof(uint32_t), 1, file);
	fwrite(&bCode->constants.capacity, sizeof(uint32_t), 1, file);
	fwrite(&bCode->objects.size, sizeof(uint32_t), 1, file);

	fwrite(bCode->code, sizeof(uint8_t), bCode->capacity, file);
	fwrite(bCode->constants.values, sizeof(int32_t), bCode->constants.capacity, file);
	fwrite(bCode->objects.values, sizeof(uint8_t), bCode->objects.size, file);
	fclose(file);
}

void
deserializeByteCode(const char* path, ByteCode* bCode)
{
	size_t fileSize;
	FILE* file;

	uint8_t* bufferPtr;
	uint8_t* buffer;
	size_t bytesRead;

	file = fopen(path, "rb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't open source file '%s'.\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);	/* Move file prt to EOF. */
	fileSize = ftell(file);		/* How far we are from the start of file? */
	rewind(file);				/* Rewind file ptr back to the beginning. */

	allocateBuffer(&buffer, fileSize, "serialized data");
	
	bufferPtr = buffer;

	bytesRead = fread(bufferPtr, sizeof(char), fileSize, file);
	if (bytesRead < fileSize) {
		fprintf(stderr, "Couldn't read source file '%s'.\n", path);
		exit(76);
	}

	memcpy(&bCode->count,    bufferPtr,      4);
	memcpy(&bCode->capacity, bufferPtr += 4, 4);

	memcpy(&bCode->constants.count,    bufferPtr += 4, 4);
	memcpy(&bCode->constants.capacity, bufferPtr += 4, 4);
	memcpy(&bCode->objects.size,       bufferPtr += 4, 4);

	allocateBuffer(&bCode->code, bCode->capacity, "bCode->code");
	memcpy(bCode->code, bufferPtr += 4, bCode->capacity);

	allocateBuffer((uint8_t**)&bCode->constants.values, bCode->constants.capacity * sizeof(int32_t), "bCode->constants.values");
	memcpy(bCode->constants.values, bufferPtr += bCode->capacity, bCode->constants.capacity * sizeof(int32_t));

	allocateBuffer(&bCode->objects.values, bCode->objects.size, "bCode->objects.values");
	memcpy(bCode->objects.values, bufferPtr += bCode->constants.capacity * sizeof(int32_t), bCode->objects.size);

	free(buffer);
	fclose(file);
}
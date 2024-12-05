#include "common.h"
#include "bytecode.h"

static void*
bufAlloc(uint32_t size, char* msg)
{
	void* buffer = malloc(size + 1);
	if (NULL == buffer) {
		fprintf(stderr, "Failed to allocate memory for %s\n", msg);
		exit(74);
	}

	return buffer;
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

	buffer = bufAlloc(fileSize + 1, "source file");

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
	ConstPool* cPool = &bCode->constants;
	ObjPool* objPool = &bCode->objects;
	char binFileName[256];

	sprintf(binFileName, "%sb", path);
	file = fopen(binFileName, "wb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't create binary file '%s'.\n", binFileName);
		exit(74);
	}

	fwrite(&bCode->count, sizeof(uint32_t), 1, file);
	fwrite(&bCode->capacity, sizeof(uint32_t), 1, file);

	fwrite(&cPool->count, sizeof(uint32_t), 1, file);
	fwrite(&cPool->capacity, sizeof(uint32_t), 1, file);
	fwrite(&objPool->size, sizeof(uint32_t), 1, file);

	fwrite(bCode->code, sizeof(uint8_t), bCode->capacity, file);
	fwrite(cPool->values, sizeof(int64_t), cPool->capacity, file);
	fwrite(objPool->values, sizeof(uint8_t), objPool->size, file);
	
	fclose(file);
}

void
deserializeByteCode(const char* path, ByteCode* bCode)
{
	size_t fileSize;
	FILE* file;

	uint8_t* buffer;
	uint8_t* pBuf; 
	size_t bytesRead;
	ConstPool* cPool = &bCode->constants;
	ObjPool* objPool = &bCode->objects;

	file = fopen(path, "rb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't open source file '%s'.\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);	/* Move file prt to EOF. */
	fileSize = ftell(file);		/* How far we are from the start of file? */
	rewind(file);				/* Rewind file ptr back to the beginning. */

	buffer = bufAlloc(fileSize, "serialized data");
	
	pBuf = buffer;

	bytesRead = fread(pBuf, sizeof(char), fileSize, file);
	if (bytesRead < fileSize) {
		fprintf(stderr, "Couldn't read source file '%s'.\n", path);
		exit(76);
	}

	memcpy(&bCode->count,    pBuf += 0, 4);
	memcpy(&bCode->capacity, pBuf += 4, 4);

	memcpy(&cPool->count,    pBuf += 4, 4);
	memcpy(&cPool->capacity, pBuf += 4, 4);
	memcpy(&objPool->size,   pBuf += 4, 4);

	bCode->code = bufAlloc(bCode->capacity, "bCode->code");
	memcpy(bCode->code, pBuf += 4, bCode->capacity);

	cPool->values = bufAlloc(cPool->capacity * sizeof(int64_t), "cPool->values");
	memcpy(cPool->values, pBuf += bCode->capacity, cPool->capacity * sizeof(int64_t));

	objPool->values = bufAlloc(objPool->size, "objPool->values");
	memcpy(objPool->values, (pBuf += (cPool->capacity * sizeof(int32_t))), objPool->size);

	free(buffer);
	fclose(file);
}
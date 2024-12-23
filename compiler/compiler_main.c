#include "common.h"
#include "compiler.h"
#include "memory.h"

static void
usage(void)
{
	printf("Usage:\n\tfunvmc <source.fn>\n\tfunvm source.fnb\n");
	exit(1);
}

static char*
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

	buffer = ALLOCATE(char, fileSize + 1);

	bytesRead = fread(buffer, sizeof(char), fileSize, file);
	if (bytesRead < fileSize) {
		fprintf(stderr, "Couldn't read source file '%s'.\n", path);
		exit(74);
	}

	buffer[fileSize] = '\0';
	fclose(file);

	return buffer;
}

static void
serialize(const char* path, ByteCode* bCode)
{
	FILE* file;
	ConstPool* cPool = &bCode->constants;
	char binFileName[256];

	sprintf(binFileName, "%sb", path);
	file = fopen(binFileName, "wb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't create binary file '%s'.\n", binFileName);
		exit(74);
	}

	fwrite(&bCode->count,        sizeof(uint32_t), 1, file);
	fwrite(&bCode->capacity,     sizeof(uint32_t), 1, file);
	fwrite(&cPool->count,        sizeof(uint32_t), 1, file);
	fwrite(&cPool->capacity,     sizeof(uint32_t), 1, file);

	fwrite(bCode->code,      sizeof(uint8_t),  bCode->capacity, file);
	fwrite(cPool->values,    sizeof(uint64_t), cPool->capacity, file);
	
	fclose(file);
}

int
main(int argc, char* argv[])
{
	if (argc != 2)
		usage();
#if defined(FUNVM_MEM_MANAGER)
	heapInit();
#endif
	char* source;
	ByteCode bCode;
	
	source = readSourceFile(argv[1]);
	bool res = compile(source, &bCode);
	if (!res) {
		printf("Failed to compile...\n");
		fvm_free(source);
		exit(1);
	}

	serialize(argv[1], &bCode);
	freeByteCode(&bCode);
	fvm_free(source);
}
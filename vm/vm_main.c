#include "common.h"
#include "vm.h"

static void
usage(void)
{
	printf("Usage:\n\tfunvmc <source.fn>\n\tfunvm source.fnb\n");
	exit(1);
}

static void
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

	buffer = ALLOCATE(uint8_t, fileSize);
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

	bCode->code     = ALLOCATE(uint8_t, bCode->capacity);
	cPool->values   = ALLOCATE(Value, cPool->capacity * sizeof(Value));
	objPool->values = ALLOCATE(uint8_t, objPool->size);
	
	memcpy(bCode->code,     pBuf += 4, bCode->capacity);
	memcpy(cPool->values,   pBuf += bCode->capacity, cPool->capacity * sizeof(Value));
	memcpy(objPool->values, pBuf += (cPool->capacity * sizeof(Value)), objPool->size);

	FREE(uint8_t, buffer);
	fclose(file);
}

int
main(int argc, char* argv[])
{
	if (argc != 2)
		usage();
	
	ByteCode bCode;
#if defined(FUNVM_MEM_MANAGER)
	heapInit();
#endif
	initByteCode(&bCode);
	deserializeByteCode(argv[1], &bCode);

	initVM();
	interpret(&bCode);
	
	freeByteCode(&bCode);
	freeVM();
	return (0);
}
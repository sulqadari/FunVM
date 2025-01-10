#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "globals.h"


static void
usage(void)
{
	printf("Usage:\n\tfunvmc <source.fn>\n\tfunvm source.fnb\n");
	exit(1);
}


static char*
concatenate(const char* path, const char* name)
{
	uint32_t pathLen = 0;
	uint32_t nameLen = 0;
	uint32_t len = 0;

	if (path != NULL)
		pathLen = strlen(path);
	else
		path = "";
	
	nameLen = strlen(name);

	len = pathLen + nameLen;
	char* absPath = ALLOCATE(char, len + 1);

	memcpy(absPath, path, pathLen);
	memcpy(absPath + pathLen, name, nameLen);
	absPath[len] = '\0';
	
	return absPath;
}

static char*
readSourceFile(const char* path, const char* name)
{
	size_t fileSize;
	FILE* file;
	char* buffer;
	size_t bytesRead;
	char* sourceFile = concatenate(path, name);

	file = fopen(sourceFile, "rb");
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
	FREE(char, sourceFile);

	return buffer;
}

static void
serialize(const char* path, const char* name, ByteCode* bCode)
{
	FILE* file;
	ConstPool* cPool = &bCode->constants;
	char binFileName[256];

	if (path == NULL)
		path = "";
	
	sprintf(binFileName, "%sbin/%sb", path, name);

	file = fopen(binFileName, "wb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't create binary file '%s'.\n", binFileName);
		exit(74);
	}

	fwrite(&bCode->count,    sizeof(uint32_t), 1, file);
	fwrite(&bCode->capacity, sizeof(uint32_t), 1, file);
	fwrite(&cPool->count,    sizeof(uint32_t), 1, file);
	fwrite(&cPool->capacity, sizeof(uint32_t), 1, file);
	
	fwrite(&objPool.count, sizeof(uint32_t), 1, file);

	fwrite(bCode->code,    sizeof(uint8_t), bCode->capacity, file);
	fwrite(cPool->values,  sizeof(Value),   cPool->capacity, file);
	fwrite(objPool.values, sizeof(char),    objPool.count,   file);

	fclose(file);
	
}

int
main(int argc, char* argv[])
{
	char* filePath = NULL;
	char* fileName = NULL;

	if (argc == 2) {
		fileName = argv[1];
	} else if (argc == 3) {
		filePath = argv[1];
		fileName = argv[2];
	} else {
		usage();
	}

#if defined(FUNVM_MEM_MANAGER)
	heapInit();
#endif

	char* source;
	ObjFunction* entryPoint;

	source = readSourceFile(filePath, fileName);
	initObjPool();
	entryPoint = compile(source);
	if (entryPoint == NULL) {
		printf("Failed to compile...\n");
		fvm_free(source);
		exit(1);
	}

	freeObjects();
	serialize(filePath, fileName, &entryPoint->bCode);
	freeObjPool();
	fvm_free(source);
}
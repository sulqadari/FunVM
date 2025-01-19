#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "globals.h"

static void
usage(void)
{
	printf("Usage: funvmc <source_file.fv> <path/to/source/dir> <path/to/output/dir>\n");
	exit(1);
}

static char*
readSourceFile(char* sourcePath, char* name)
{
	size_t fileSize;
	FILE* file;
	char* buffer;
	size_t bytesRead;
	char pathAndName[256];
	sprintf(pathAndName, "%s%s", sourcePath, name);

	file = fopen(pathAndName, "rb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't open source file '%s'.\n", pathAndName);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);	/* Move file prt to EOF. */
	fileSize = ftell(file);		/* How far we are from start of the file? */
	rewind(file);				/* Rewind file ptr back to the beginning. */

	buffer = ALLOCATE(char, fileSize + 1);

	bytesRead = fread(buffer, sizeof(char), fileSize, file);
	if (bytesRead < fileSize) {
		fprintf(stderr, "Error: the source file '%s' have been read partially.\n", pathAndName);
		FREE(char, buffer);
		exit(74);
	}

	buffer[fileSize] = '\0';
	fclose(file);

	return buffer;
}

static void
serialize(char* outputPath, char* outputName)
{
	FILE* file;
	char binFileName[256];

	if (outputPath == NULL || outputName == NULL) {
		fprintf(stderr, "Failed to serialize file because it's null.\n");
		exit(74);
	}
	
	sprintf(binFileName, "%s%sb", outputPath, outputName);

	file = fopen(binFileName, "wb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't create binary file '%s'.\n", binFileName);
		exit(74);
	}

	fwrite(objPool->values, sizeof(uint8_t), objPool->valuesLen, file);
	fclose(file);
}

int
main(int argc, char* argv[])
{
	char* name = NULL;
	char* sourcePath = NULL;
	char* outputPath = NULL;

	if (argc == 4) {
		name = argv[1];
		sourcePath = argv[2];
		outputPath = argv[3];
	} else {
		usage();
	}

#if defined(FUNVM_MEM_MANAGER)
	heapInit();
#endif

	char* source;
	ObjFunction* mainFunction;

	source = readSourceFile(sourcePath, name);
	initObjPool();
	mainFunction = compile(source);
	if (mainFunction == NULL) {
		printf("Failed to compile...\n");
		fvm_free(source);
		exit(1);
	}

	freeObjects();
	serialize(outputPath, name);
	freeObjPool();
	freeTable(&vm.strings);
	freeTable(&vm.globals);
	fvm_free(source);
}
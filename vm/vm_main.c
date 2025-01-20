#include "globals.h"

static void
usage(void)
{
	printf("Usage: \tfunvm source.fnb\n");
	exit(1);
}

static size_t
openBinary(FILE** file, const char* path)
{
	size_t fileSize;
	*file = fopen(path, "rb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't open source file '%s'.\n", path);
		exit(74);
	}

	fseek(*file, 0L, SEEK_END);	/* Move file prt to EOF. */
	fileSize = ftell(*file);	/* How far we are from the start of file? */
	rewind(*file);				/* Rewind file ptr back to the beginning. */
	return fileSize;
}

static void
resolveAddresses(ObjFunction* mainFunction, uint32_t cplrAddr)
{
	uint8_t* bytecode = mainFunction->bCode.code;
	uint8_t* values = (uint8_t*)mainFunction->bCode.constants.values;

	mainFunction->bCode.code             = (uint8_t*)((uint32_t)mainFunction + (uint32_t)bytecode);
	mainFunction->bCode.constants.values = (Value*)((uint32_t)mainFunction + (uint32_t)values);
}

static ObjFunction*
deserializeByteCode(const char* path)
{
	size_t fileSize;
	uint32_t cplrAddr = 0x5655efd0;		// must be variable.
	FILE*  file;
	uint8_t* mainFunction;

	fileSize     = openBinary(&file, path);
	mainFunction = ALLOCATE(uint8_t, fileSize);

	fread(mainFunction, sizeof(uint8_t), fileSize, file);
	fclose(file);

	memcpy(&cplrAddr, mainFunction + fileSize - 4, 4);
	resolveAddresses((ObjFunction*)mainFunction, cplrAddr);
	return (ObjFunction*)mainFunction;
}

int
main(int argc, char* argv[])
{
	if (argc != 2)
		usage();
	
	ObjFunction* mainFunction = NULL;

#if defined(FUNVM_MEM_MANAGER)
	heapInit();
#endif

	mainFunction = deserializeByteCode(argv[1]);
	initObjPool();
	initVM();
	interpret(mainFunction);
	
	freeVM();
	FREE(uint8_t, mainFunction);

	return (0);
}
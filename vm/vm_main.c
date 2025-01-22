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
updateConstants(Value* constPool, uint32_t count, uint32_t startAddr)
{
	ObjType type;

	for (uint32_t i = 0; i < count; ++i) {

		type = OBJ_TYPE(constPool[i]);
		switch (type) {
			case obj_string: {
				uint8_t* prevAddr = (uint8_t*)CSTRING_UNPACK(constPool[i]);
				CSTRING_UNPACK(constPool[i]) = (const char*)(startAddr + (uint32_t)prevAddr);
			} break;
			default: {

			} break;
		}
	}
}

static void
resolveAddresses(ObjFunction* mainFunction)
{
	uint8_t* bytecode = mainFunction->bCode.code;
	uint8_t* values = (uint8_t*)mainFunction->bCode.constants.values;

	mainFunction->bCode.code             = (uint8_t*)((uint32_t)mainFunction + (uint32_t)bytecode);
	mainFunction->bCode.constants.values = (Value*)((uint32_t)mainFunction + (uint32_t)values);
	updateConstants(mainFunction->bCode.constants.values, mainFunction->bCode.constants.count, (uint32_t)mainFunction);
}

static ObjFunction*
deserializeByteCode(const char* path)
{
	size_t fileSize;
	FILE*  file;
	uint8_t* mainFunction;

	fileSize     = openBinary(&file, path);
	mainFunction = ALLOCATE(uint8_t, fileSize);

	fread(mainFunction, sizeof(uint8_t), fileSize, file);
	fclose(file);

	resolveAddresses((ObjFunction*)mainFunction);
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
	freeObjPool();
	return (0);
}
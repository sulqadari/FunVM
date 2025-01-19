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

// static ObjString*
// parseObjString(ObjString* objString)
// {
// 	objString->chars        = (char*)((uint8_t*)objString + sizeof(ObjString));
// 	((Obj*)objString)->next = objPool->objList;
// 	objPool->objList         = (Obj*)objString;
// 	return objString;
// }

// static ObjFunction*
// parseObjFunction(ObjFunction* objFunction)
// {
// 	uint32_t offset = sizeof(ObjFunction) + sizeof(ByteCode);
// 	objFunction->bCode.code = (uint8_t*)objFunction + offset;

// 	offset += sizeof(ConstPool) + objFunction->bCode.count;
// 	objFunction->bCode.constants.values = (Value*)((uint8_t*)objFunction + offset);

// 	if (objFunction->name != NULL) {
// 		offset += objFunction->bCode.constants.count;
// 		objFunction->name->chars = (char*)objFunction + offset;
// 	}

// 	return objFunction;
// }

static uint8_t*
deserializeByteCode(const char* path)
{
	size_t fileSize;
	FILE*  file;
	uint8_t* buffer;

	fileSize = openBinary(&file, path);
	buffer   = ALLOCATE(uint8_t, fileSize);
	fread(buffer, sizeof(uint8_t), fileSize, file);
	fclose(file);

	objPool = (ObjectPool*)buffer;

	uint32_t cplrAddrSpace = (uint32_t)buffer;
	uint32_t vmAddrSpace   = heapStartAddress();
	uint32_t diff;

	return buffer;
}

int
main(int argc, char* argv[])
{
	if (argc != 2)
		usage();
	
	ObjFunction mainFunction;
	uint8_t* binary;
#if defined(FUNVM_MEM_MANAGER)
	heapInit();
#endif

	binary = deserializeByteCode(argv[1]);

	initVM();
	interpret(&mainFunction);
	
	freeVM();
	FREE(uint8_t, binary);

	return (0);
}
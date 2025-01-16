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

static ObjString*
parseObjString(ObjString* objString)
{
	objString->chars        = (char*)((uint8_t*)objString + sizeof(ObjString));
	((Obj*)objString)->next = objPool.objects;
	objPool.objects         = (Obj*)objString;
	return objString;
}

static ObjFunction*
parseObjFunction(ObjFunction* objFunction)
{
	return objFunction;
}

static void
deserializeByteCode(const char* path)
{
	size_t fileSize;
	FILE* file;

	fileSize = openBinary(&file, path);

	fread(&objPool, sizeof(ObjectPool), 1, file);
	objPool.values = ALLOCATE(uint8_t, (fileSize - sizeof(ObjectPool)));

	fread(objPool.values, sizeof(uint8_t), objPool.valuesLen, file);
	fclose(file);

	for (int32_t i = objPool.idxCount - 1; i >= 0; --i) {
		uint32_t idx = objPool.indexes[i];
		ObjType type = ((Obj*)&objPool.values[idx])->type;

		if (obj_string == type) {
			parseObjString((ObjString*)&objPool.values[idx]);
		} else if (obj_func == type) {
			parseObjFunction((ObjFunction*)&objPool.values[idx]);
		} else {
			fprintf(stderr, "Error: unknown object type encountered while parsing '%s' binary file.\n", path);
			FREE(uint8_t, objPool.values);
			exit(74);
		}
	}
}

int
main(int argc, char* argv[])
{
	if (argc != 2)
		usage();
	
	ObjFunction mainFunction;
#if defined(FUNVM_MEM_MANAGER)
	heapInit();
#endif
	initObjPool();
	initByteCode(&mainFunction.bCode);
	deserializeByteCode(argv[1]);

	initVM();
	interpret(&mainFunction);
	freeVM();
	freeObjPool();
	return (0);
}
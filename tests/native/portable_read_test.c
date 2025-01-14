#if !defined(FUNVM_MEM_MANAGER)
#	define FUNVM_MEM_MANAGER
#endif
#include "common.h"
#include "memory.h"
#include "object.h"
#include "object_pool.h"
#include "globals.h"

static ObjString*
deserialize(char* binary)
{
	size_t fileSize;
	FILE* file;
	size_t bytesRead;
	uint8_t* string = NULL;

	file = fopen(binary, "rb");
	if (NULL == file) {
		fprintf(stderr, "Couldn't open source file '%s'.\n", binary);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);	/* Move file prt to EOF. */
	fileSize = ftell(file);		/* How far we are from the start of file? */
	rewind(file);				/* Rewind file ptr back to the beginning. */

	string = (uint8_t*)reallocate(NULL, 0 , fileSize);
	bytesRead = fread(string, sizeof(uint8_t), fileSize, file);
	if (bytesRead < fileSize) {
		fprintf(stderr, "Couldn't read source file '%s'.\n", binary);
		fclose(file);
		exit(76);
	}
	uint32_t offset = sizeof(ObjString);
	((ObjString*)string)->chars = (char*)&string[offset];
	fclose(file);
	return (ObjString*)string;
}

int
main(int argc, char* argv[])
{
	heapInit();

	ObjString* string = deserialize(argv[1]);
	printf("%s\n", string->chars);
	FREE(ObjString, string);

	return 0;
}
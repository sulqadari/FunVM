#ifndef FUNVM_VM_H
#define FUNVM_VM_H

#include "common.h"
#include "bytecode.h"
#include "memory.h"
#include "value.h"
#include "hash_table.h"
#include "object.h"

#define FRAMES_MAX 4
#define STACK_SIZE (FRAMES_MAX * 96)

/**
 * Represents a single ongoing function call.
 */
typedef struct {
	ObjFunction* function;	/* Used for look up constants. */
	uint8_t* ip;			/* Point to proceed from when this function commits another function call. */
	Value* slots;			/* Points into the VM's value stack at the first slot that this function can use. */
} CallFrame;

typedef struct {
	CallFrame frames[FRAMES_MAX];
	uint32_t  frameCount;	/* The current height of the CallFrame stack - the number of ongoing function calls. */
	Value     stack[STACK_SIZE];
	Value*    stackTop;		/* Points to the element just past the last item on the stack. */
	Value*    stackStart;
	Value*    stackEnd;
	Table     globals;
	Table     strings;
	Obj*      objects;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;


void initVM(void);
void freeVM(void);
InterpretResult interpret(ObjFunction* topLevel);

void push(Value value);
Value pop(void);

#endif /* FUNVM_VM_H */
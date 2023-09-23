#ifndef FUNVM_VM_H
#define FUNVM_VM_H

#include "value.h"
#include "bytecode.h"
#include "object.h"

typedef struct Table Table;

/**
 * Represents a single ongoing function call.
 * For each call that hasn't returned yet we need to track where on the stack
 * that function's locals begin, and where the caller should resume.
 * 
 * ObjFunction* function:
 * 			current function.
 * FN_ubyte* ip;
 * 			current function's instruction set.
 * Value* slots:
 * 			points into the VM's values stack at the first slot that
 * 			this function can use.
*/
typedef struct {
	ObjFunction* function;
	FN_ubyte* ip;
	Value* slots;
} CallFrame;

#define STACK_MAX 256
#define FRAMES_MAX 64

/**
 * CallFrame frames[FRAMES_MAX]:
 * 			Each CallFrame has its own instruction pointer and its own pointer
 * 			to the ObjFunction that's executing.
 * FN_uint frameCount:
 * 			stores the current height of the CallFrame stack, i.e., the number of
 * 			ongoing function calls.
*/
typedef struct {
	CallFrame frames[FRAMES_MAX];
	FN_uint frameCount;
	Value stack[STACK_MAX];
	Value* stackTop;
	Table* globals;
	Table* interns;		/* String interning (see  section 20.5). */
	Object* objects;	/* Pointer to the head of the list of objects on the heap. */
} VM;

typedef enum {
	IR_OK,
	IR_COMPILE_ERROR,
	IR_RUNTIME_ERROR
} InterpretResult;

void initVM(VM* vm);
void freeVM(VM* vm);
void objSetVM(VM* vm);
Value pop(void);
InterpretResult interpret(const char* source);

#endif // !FUNVM_VM_H
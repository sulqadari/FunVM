#ifndef FUNVM_VM_H
#define FUNVM_VM_H

#include "common.h"
#include "value.h"
#include "bytecode.h"
#include "object.h"
#include "table.h"
#include "compiler.h"

typedef struct Table Table;

/**
 * Represents a single ongoing function call.
 * 
 * Each CallFrame has its own 'ip' and pointer to the ObjFunction that it's
 * executing, from which we can get to the function's bytecode.
 * 
 * The function call is straightforward - simply setting 'ip' to point to the
 * first instruction in that function's bytecode.
 * 
 * At the beginning of each function call, the VM records the location of the 
 * first slot where that function's own locals begin.
 * The instructions for working with local variables access them by a slot
 * index relative to that location, instead of relative to the bottom of the stack.
 * At compile time, those relative slots are calculated in advance.
 * At runtime, we convert that relative slot to an absolute stack index by adding
 * the function call's starting slot.
 * 
 * For each current call the VM need to track where on the stack the locals of the
 * given function begin, and where the caller should resume.
 * In case of recursion there may be multiple return addresses for
 * a single function.
 * Thus, 'return address' is the property of invocation and not the function itself.
 * 
 * ObjClosure* closure:
 * 			Contains a function being called.
 * 			Used to look up constants and for other things.
 * 
 * FN_UBYTE* ip;
 * 			Caller's 'ip',
 * 			Used as return address to resume from.
 * 
 * Value* slots:
 * 			The "Frame Pointer".
 * 			Points into the VM's value stack at the first slot that
 * 			current function can use.
*/
typedef struct {
	ObjClosure* closure;
	FN_UBYTE* ip;
	Value* slots;
} CallFrame;

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

/**
 * The main structure of Virtual Machine.
 * 
 * Every time we call a function, the VM determines the first stack slot where
 * a given function's variables begin.
 * 
 * CallFrame frames[FRAMES_MAX]:
 * 			Each CallFrame has its own instruction pointer and its own pointer
 * 			to the ObjFunction that's executing.
 * 
 * FN_UWORD frameCount:
 * 			stores the current height of the CallFrame stack, i.e., the number of
 * 			ongoing function calls.
 * 
 * Value stack[STACK_MAX]:
 * 			The stack to keep track of local variables and temporary values.
 * 
 * Value* stackTop:
 * 			The most-right slot of the stack being used.
 * 
 * Table* globals:
 * 			Hash Table of global variables.
 * 
 * Table* interns:
 * 			Hash Table of interns. see "String interning" section 20.5.
 * 
 * Object* objects:
 * 			Pointer to the head of the list of objects on the heap.
*/
typedef struct {
	CallFrame frames[FRAMES_MAX];
	FN_UWORD frameCount;
	Value stack[STACK_MAX];
	Value* stackTop;
	Table* globals;
	Table* interns;
	ObjUpvalue* openUpvalues;	/* Head of linked list of upvalues. */

	size_t bytesAllocated;		/* allocated memory. */
	size_t nextGC;				/* GC triggering threshold. */

	Object* objects;
	FN_WORD grayCount;
	FN_WORD grayCapacity;
	Object** grayStack;
} VM;

typedef enum {
	IR_OK,
	IR_COMPILE_ERROR,
	IR_RUNTIME_ERROR
} InterpretResult;

void initVM(VM* vm);
void freeVM(VM* vm);
void objectSetVM(VM* vm);
void memorySetVM(VM* _vm);
void push(Value value);
Value pop(void);
InterpretResult interpret(const char* source);

#endif // !FUNVM_VM_H
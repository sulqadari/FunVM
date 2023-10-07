#ifndef FUNVM_VM_H
#define FUNVM_VM_H

#include "value.h"
#include "bytecode.h"
#include "object.h"
// #include "table.h"
#include "compiler.h"

typedef struct Table Table;

/**
 * Represents a single ongoing function call.
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
 * ObjFunction* function:
 * 			current function.
 * FN_UBYTE* ip;
 * 			The caller's instruction pointer used sd 'return address'.
 * 			The VM will jump to the 'ip' of the caller's CallFrame and
 * 			resume from there. 
 * Value* slots:
 * 			points into the VM's value stack at the first slot that
 * 			current function can use.
*/
typedef struct {
	ObjFunction* function;
	FN_UBYTE* ip;
	Value* slots;
} CallFrame;

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

/**
 * Every time we call a function, the VM determines the first stack slot where
 * a given function's variables begin.
 * 
 * CallFrame frames[FRAMES_MAX]:
 * 			Each CallFrame has its own instruction pointer and its own pointer
 * 			to the ObjFunction that's executing.
 * FN_UWORD frameCount:
 * 			stores the current height of the CallFrame stack, i.e., the number of
 * 			ongoing function calls.
*/
typedef struct {
	CallFrame frames[FRAMES_MAX];
	FN_UWORD frameCount;
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
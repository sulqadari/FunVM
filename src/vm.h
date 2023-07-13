#ifndef FUNVM_VM_H
#define FUNVM_VM_H

#include "bytecode.h"
#include "constant_pool.h"
#include <stddef.h>

typedef struct {
	Bytecode *bytecode;	/* A chunk of bytecode to execute. */
	uint8_t *ip;		/* InsPtr, points to the ins about to be executed. */
	Value *stack;
	Value *stackTop;
	uint32_t stackSize;
} VM;

typedef enum {
	IR_OK,
	IR_COMPILE_ERROR,
	IR_RUNTIME_ERROR
} InterpretResult;

void initVM(VM *vm);
void freeVM(VM *vm);
InterpretResult interpret(VM *vm, Bytecode *bytecode);
//void resizeStack(void);
void push(Value value, VM *vm);
Value pop(VM *vm);

#endif // !FUNVM_VM_H
#ifndef FUNVM_VM_H
#define FUNVM_VM_H

#include "funvmConfig.h"
#include "bytecode.h"
#include "constant_pool.h"
#include <stddef.h>

typedef struct {
	Bytecode *bytecode;	/* A chunk of bytecode to execute. */
	uint8_t *ip;		/* InsPtr, points to the ins about to be executed. */
	Value *stack;
	uint32_t stackSize;
	Value *stackTop;
} VM;

typedef enum {
	IR_OK,
	IR_COMPILE_ERROR,
	IR_RUNTIME_ERROR
} InterpretResult;

void initVM(void);
void freeVM(void);
InterpretResult interpret(Bytecode *bytecode);
void resizeStack(void);
void push(Value value);
Value pop(void);

#endif // !FUNVM_VM_H
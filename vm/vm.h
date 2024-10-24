#ifndef FUNVM_VM_H
#define FUNVM_VM_H

#include "common.h"
#include "bytecode.h"
#include "memory.h"
#include "values.h"

#define STACK_SIZE (8)

typedef struct {
	ByteCode* bCode;
	uint8_t*  ip;	/* <! Instruction pointer. Points to the next bytecode to be used. */
	Value stack[STACK_SIZE];
	Value* stackTop;/* <! Points to the element just past the last item on the stack. */
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM(void);
void freeVM(void);
InterpretResult interpret(ByteCode* bCode);

void push(Value value);
Value pop(void);

#endif /* FUNVM_VM_H */
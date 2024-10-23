#ifndef FUNVM_VM_H
#define FUNVM_VM_H

#include "bytecode.h"
#include "memory.h"

typedef struct {
	ByteCode* bCode;
	uint8_t* ip;		/* <! Instruction pointer. */
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM(void);
void freeVM(void);
InterpretResult interpret(ByteCode* bCode);

#endif /* FUNVM_VM_H */
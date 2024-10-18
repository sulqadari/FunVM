#ifndef FUNVM_VM_H
#define FUNVM_VM_H

#include "common.h"
#include "value.h"

typedef enum {
	op_iconst,
	op_add,
	op_sub,
	op_mul,
	op_div,
	op_negate,
	op_ret,
} OpCode;

typedef struct {
	uint32_t count;
	uint32_t capacity;
	uint8_t* code;
	ValueArray constants;
} Bytecode;

extern uint32_t* lines;

void initBytecode(Bytecode* bCode);
void freeBytecode(Bytecode* bCode);
void writeBytecode(Bytecode* bCode, uint8_t byte, uint32_t line);
int addConstant(Bytecode* chunk, i32 value);

#endif /* FUNVM_VM_H */
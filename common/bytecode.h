#ifndef FUNVM_BYTECODE_H
#define FUNVM_BYTECODE_H

#include "common.h"
#include "const_pool.h"

typedef enum {
	op_iconst,
	op_iconst_long,
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
	ConstPool constants;
} ByteCode;

extern uint32_t* lines;

void initByteCode(ByteCode* bCode);
void freeByteCode(ByteCode* bCode);
void writeByteCode(ByteCode* bCode, uint8_t byte, uint32_t line);
uint32_t addConstant(ByteCode* bCode, Value value);

#endif /* FUNVM_BYTECODE_H */
#ifndef FUNVM_BYTECODE_H
#define FUNVM_BYTECODE_H

#include "common.h"
#include "const_pool.h"
#include "object_pool.h"

typedef enum {
	op_iconst,
	op_sconst,
	op_bconst,
	op_iconst_long,
	op_sconst_long,
	op_bconst_long,
	op_null,
	op_true,
	op_false,
	op_eq,
	op_neq,
	op_gt,
	op_gteq,
	op_lt,
	op_lteq,
	op_add,
	op_sub,
	op_mul,
	op_div,
	op_not,
	op_negate,
	op_obj,
	op_obj_long,
	op_ret,
} OpCode;

typedef struct {
	uint32_t count;
	uint32_t capacity;
	uint8_t* code;
	ConstPool constants;
	ObjPool objects;
} ByteCode;

extern uint32_t* lines;

void initByteCode(ByteCode* bCode);
void freeByteCode(ByteCode* bCode);
void writeByteCode(ByteCode* bCode, uint8_t byte, uint32_t line);
uint32_t addConstant(ByteCode* bCode, i32 value);
uint32_t addObject(ByteCode* bCode, void* obj);

#endif /* FUNVM_BYTECODE_H */
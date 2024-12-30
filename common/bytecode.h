#ifndef FUNVM_BYTECODE_H
#define FUNVM_BYTECODE_H

#include "common.h"
#include "const_pool.h"

typedef enum {
	op_iconst,
	op_iconstw,
	op_null,
	op_true,
	op_false,
	op_eq,
	op_gt,
	op_lt,
	op_add,
	op_sub,
	op_mul,
	op_div,
	op_not,
	op_negate,
	op_print,
	op_pop,
	op_popn,
	op_def_gvar,
	op_def_gvarw,
	op_get_gvar,
	op_get_gvarw,
	op_set_gvar,
	op_set_gvarw,
	op_get_locvar,
	op_get_locvarw,
	op_set_locvar,
	op_set_locvarw,
	op_jmp_false,	/*<! Jump if false. */
	op_jmp,
	op_loop,
	op_call,
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
#include <stdio.h>
#include <stdint.h>

#include "debug.h"
#include "object.h"
#include "value.h"
#include "common.h"

static FN_UWORD
simpleInstruction(const char* name, FN_WORD offset)
{
	printf("	%-16s\n", name);
	return offset + 1;
}

static FN_UWORD
byteInstruction(const char* name, Bytecode* bytecode, FN_WORD offset)
{
	FN_UBYTE slot = bytecode->code[offset + 1];
	printf("	%-16s %4d\n", name, slot);
	return offset + 2;
}

static FN_UWORD
jumpInstruction(const char* name, FN_WORD sign, Bytecode* bytecode, FN_WORD offset)
{
	FN_UWORD jump = 0;
	jump |= (FN_UWORD)((bytecode->code[offset + 1] << 8) & 0xFF);
	jump |= (FN_UWORD)( bytecode->code[offset + 2] & 0xFF);

	printf("	%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
	printf("%04d	   | 	%-16s   "
	"					`%d`\n", offset + 1, "op1", bytecode->code[offset + 1]);
	printf("%04d	   | 	%-16s   "
	"					`%d`\n", offset + 2, "op2", bytecode->code[offset + 2]);
	return offset + 3;
}

static FN_UWORD
constantInstruction(const char* name, Bytecode* bytecode, FN_WORD offset)
{
	FN_UBYTE constant = bytecode->code[offset + 1];

	printf("	%-16s %4d\n", name, constant);
	printf("%04d	   | 	%-16s   					`", offset + 1, "op1");
	printValue(bytecode->constPool.pool[constant]);
	printf("`\n");
	
	return offset + 2;
}

static FN_WORD
invokeInstruction(const char* name, Bytecode* bytecode, FN_WORD offset)
{
	FN_UBYTE constant = bytecode->code[offset + 1];
	FN_UBYTE argCount = bytecode->code[offset + 2];
	printf("%-16s (%d args) %4d '", name, argCount, constant);
	printValue(bytecode->constPool.pool[constant]);
	printf("'\n");
	return offset + 3;
}

FN_UWORD
disassembleInstruction(Bytecode* bytecode, FN_WORD offset)
{
	printf("%04d	", offset);

	/* If we aren't at the very beginning and the line of the current
	 * bytecode is the same as of the previous one, then just print '|'
	 * which means we are still on the same line of the source code. */
	if (offset > 0 &&
		bytecode->lines[offset] == bytecode->lines[offset - 1]) {
		printf("   | ");
	} else {
		printf("%4d ", bytecode->lines[offset]);
	}

	FN_UBYTE opcode = bytecode->code[offset];
	switch (opcode) {
		case OP_CONSTANT:
			return constantInstruction("OP_CONSTANT", bytecode, offset);
		case OP_NIL:
			return simpleInstruction("OP_NIL", offset);
		case OP_TRUE:
			return simpleInstruction("OP_TRUE", offset);
		case OP_FALSE:
			return simpleInstruction("OP_FALSE", offset);
		case OP_POP:
			return simpleInstruction("OP_POP", offset);

		case OP_GET_LOCAL:
			return byteInstruction("OP_GET_LOCAL", bytecode, offset);
		case OP_SET_LOCAL:
			return byteInstruction("OP_SET_LOCAL", bytecode, offset);

		case OP_DEFINE_GLOBAL:
			return constantInstruction("OP_DEFINE_GLOBAL", bytecode, offset);
		case OP_SET_GLOBAL:
			return constantInstruction("OP_SET_GLOBAL", bytecode, offset);
		case OP_GET_GLOBAL:
			return constantInstruction("OP_GET_GLOBAL", bytecode, offset);
		
		case OP_SET_UPVALUE:
			return byteInstruction("OP_SET_UPVALUE", bytecode, offset);
		case OP_GET_UPVALUE:
			return byteInstruction("OP_GET_UPVALUE", bytecode, offset);

		case OP_SET_PROPERTY:
			return constantInstruction("OP_SET_PROPERTY", bytecode, offset);
		case OP_GET_PROPERTY:
			return constantInstruction("OP_GET_PROPERTY", bytecode, offset);
		case OP_GET_SUPER:
			return constantInstruction("OP_GET_SUPER", bytecode, offset);
		case OP_EQUAL:
			return simpleInstruction("OP_EQUAL", offset);
		case OP_GREATER:
			return simpleInstruction("OP_GREATER", offset);
		case OP_LESS:
			return simpleInstruction("OP_LESS", offset);

		case OP_ADD:
			return simpleInstruction("OP_ADD", offset);
		case OP_SUBTRACT:
			return simpleInstruction("OP_SUBTRACT", offset);
		case OP_MULTIPLY:
			return simpleInstruction("OP_MULTIPLY", offset);
		case OP_DIVIDE:
			return simpleInstruction("OP_DIVIDE", offset);

		case OP_NOT:
			return simpleInstruction("OP_NOT", offset);
		case OP_NEGATE:
			return simpleInstruction("OP_NEGATE", offset);

		case OP_PRINT:
			return simpleInstruction("OP_PRINT", offset);
		case OP_PRINTLN:
			return simpleInstruction("OP_PRINTLN", offset);

		case OP_JUMP:
			return jumpInstruction("OP_JUMP", 1, bytecode, offset);
		case OP_JUMP_IF_FALSE:
			return jumpInstruction("OP_JUMP_IF_FALSE", 1, bytecode, offset);

		case OP_LOOP:
			return jumpInstruction("OP_LOOP", -1, bytecode, offset);

		case OP_CALL:
			return byteInstruction("OP_CALL", bytecode, offset);
		case OP_INVOKE:
			return invokeInstruction("OP_INVOKE", bytecode, offset);
		case OP_SUPER_INVOKE:
			return invokeInstruction("OP_SUPER_INVOKE", bytecode, offset);
		case OP_CLOSURE: {
			offset++;
			FN_UBYTE constant = bytecode->code[offset++];
			printf("%-16s %4d ", "OP_CLOSURE", constant);
			printValue(bytecode->constPool.pool[constant]);
			printf("\n");

			ObjFunction* function = FUNCTION_UNPACK(
									bytecode->constPool.pool[constant]);
			
			for (FN_WORD j = 0; j < function->upvalueCount; ++j) {
				FN_WORD isLocal = bytecode->code[offset++];
				FN_WORD index = bytecode->code[offset++];
				printf("%04d	|			%s %d\n",
						offset - 2, isLocal ? "local" : "upvalue", index);
			}

			return offset;
		} case OP_CLOSE_UPVALUE:
			return simpleInstruction("OP_CLOSE_UPVALUE", offset);

		case OP_RETURN:
			return simpleInstruction("OP_RETURN", offset);
		case OP_CLASS:
			return constantInstruction("OP_CLASS", bytecode, offset);
		case OP_INHERIT:
			return simpleInstruction("OP_INHERIT", offset);
		case OP_METHOD:
			return constantInstruction("OP_METHOD", bytecode, offset);
		default:
			printf("Unknown opcode %d\n", opcode);
			return offset;
	}
}

void
disassembleBytecode(Bytecode* bytecode, const char* name)
{
	printf("\n=== %s ===\n"
		"offset | line |    opcode    	   | Pool offset | Operand Value\n", name);
	for (FN_WORD offset = 0; offset < (FN_WORD)bytecode->count; ) {
		offset = disassembleInstruction(bytecode, offset);
	}
}
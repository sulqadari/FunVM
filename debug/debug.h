#ifndef FUNVM_DEBUG_H
#define FUNVM_DEBUG_H

#include "bytecode.h"

void disassembleBytecode(Bytecode* bytecode, const char* name);
int32_t disassembleInstruction(Bytecode* bytecode, uint16_t offset);

#endif // !FUNVM_DEBUG_H
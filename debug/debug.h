#ifndef FUNVM_DEBUG_H
#define FUNVM_DEBUG_H

#include "bytecode.h"

void disassembleBytecode(Bytecode* bytecode, const char* name);
FN_WORD disassembleInstruction(Bytecode* bytecode, FN_UWORD offset);

#endif // !FUNVM_DEBUG_H
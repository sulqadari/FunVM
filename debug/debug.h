#ifndef FUNVM_DEBUG_H
#define FUNVM_DEBUG_H

#include "../src/bytecode.h"

void disassembleBytecode(Bytecode* bytecode, const char* name);
FN_UWORD disassembleInstruction(Bytecode* bytecode, FN_WORD offset);

#endif // !FUNVM_DEBUG_H
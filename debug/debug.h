#ifndef FUNVM_DEBUG_H
#define FUNVM_DEBUG_H

#include <stdio.h>
#include "bytecode.h"

void disassembleBytecode(Bytecode *bytecode, const char *name);
int disassembleInstruction(Bytecode *bytecode, int offset);

#endif // !FUNVM_DEBUG_H
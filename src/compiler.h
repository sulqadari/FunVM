#ifndef FUNVM_COMPILER_H
#define FUNVM_COMPILER_H

#include "bytecode.h"
#include "vm.h"

bool compile(const char *source, Bytecode *bytecode);

#endif // !FUNVM_COMPILER_H
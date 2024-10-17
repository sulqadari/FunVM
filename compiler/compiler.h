#ifndef FUNVM_COMPILER_H
#define FUNVM_COMPILER_H

#include "vm.h"
#include "common.h"

bool compile(const char* source, Bytecode* bCode);

#endif /* FUNVM_COMPILER_H */
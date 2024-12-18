#ifndef FUNVM_COMPILER_H
#define FUNVM_COMPILER_H

#include "common.h"
#include "bytecode.h"

bool compile(const char* source, ByteCode* bCode);

#endif /* FUNVM_COMPILER_H */
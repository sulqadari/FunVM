#ifndef FUNVM_COMPILER_H
#define FUNVM_COMPILER_H

#include "bytecode.h"
#include "object.h"

#define UIN8_COUNT (UINT8_MAX + 1)
#define MAX_ARITY 127
ObjFunction* compile(const char* source);

#endif // !FUNVM_COMPILER_H
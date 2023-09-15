#ifndef FUNVM_COMPILER_H
#define FUNVM_COMPILER_H

#include "bytecode.h"
#include "object.h"

ObjFunction* compile(const char* source);

#endif // !FUNVM_COMPILER_H
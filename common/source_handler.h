#ifndef FUNVM_SOURCE_HANDLER_H
#define FUNVM_SOURCE_HANDLER_H

#include "vm.h"

char* readSourceFile(const char* path);
void saveBytecodeFile(const char* path, Bytecode* bCode);

#endif /* FUNVM_SOURCE_HANDLER_H */
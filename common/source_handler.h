#ifndef FUNVM_SOURCE_HANDLER_H
#define FUNVM_SOURCE_HANDLER_H

#include "common.h"
#include "bytecode.h"

char* readSourceFile(const char* path);
void saveByteCodeFile(const char* path, ByteCode* bCode);

#endif /* FUNVM_SOURCE_HANDLER_H */
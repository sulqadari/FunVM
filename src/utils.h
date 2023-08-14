#ifndef FUNVM_REPL_H
#define FUNVM_REPL_H

#include "vm.h"

void repl(VM *vm);
void runFile(VM *vm, const char *path);

#endif // !FUNVM_REPL_H
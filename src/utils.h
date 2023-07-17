#ifndef FUNVM_REPL_H
#define FUNVM_REPL_H

void repl(VM *vm);
void runFile(VM *vm, const char *path);

#endif // !FUNVM_REPL_H
#ifndef FUNVM_REPL_H
#define FUNVM_REPL_H

void repl(VM *vm);
static char* runFile(const char *path);

#endif // !FUNVM_REPL_H
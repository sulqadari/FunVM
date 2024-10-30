#include "values.h"

const boolean True = 0xa5;
const boolean False = 0x5a;
const Null null = 0;

bool
valuesEqual(i32 a, i32 b)
{
	return a == b;
}

#if defined(FUNVM_ARCH_x64)
void
printValue(i32 value)
{
	printf("%d", value);
}
#endif /* FUNVM_DEBUG */
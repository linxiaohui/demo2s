// hello, world

#include "lib.h"

void
umain(void)
{
	sys_cputs("hello, world\n");
	printf("i am environment %08x\n", env->env_id);
}

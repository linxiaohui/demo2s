// test bad pointer for user-level fault handler
// this is going to fault in the fault handler accessing eip (always!)
// so eventually the kernel kills it (PFM_KILL) because
// we outrun the stack with invocations of the user-level handler


#include "lib.h"

void
umain(void)
{
	sys_set_pgfault_handler(0, 0xDeadBeef, USTACKTOP);
	*(int*)0 = 0;
}

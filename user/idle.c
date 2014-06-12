// idle loop

#include "lib.h"
#include <inc/x86.h>

void
umain(void)
{
	// Since we're idling, there's no point in continuing on.
	outw(0x8A00, 0x8A00);
	sys_panic("idle loop can do I/O");
}


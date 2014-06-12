// test user-level fault handler -- alloc pages to fix faults

#include "lib.h"

void
handler(u_int va, u_int err)
{
	int r;

	printf("fault %x\n", va);
	if ((r=sys_mem_alloc(0, va, PTE_P|PTE_U|PTE_W)) < 0)
		panic("allocating at %x in page fault handler: %e", va, r);
	snprintf((char*)va, 100, "this string was faulted in at %x", va);
}

void
umain(void)
{
	set_pgfault_handler(handler);
	printf("%s\n", (char*)0xDeadBeef);
	printf("%s\n", (char*)0xCafeBffe);
}

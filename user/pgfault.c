// User-level page fault handler.  We use an assembly wrapper around a C function.
// The assembly wrapper is in entry.S.

#include "lib.h"

extern void (*__pgfault_handler)(u_int, u_int);
extern void __asm_pgfault_handler(void);

//
// Set the page fault handler function.
// If there isn't one yet, _pgfault_handler will be 0.
// The first time we register a handler, we need to 
// allocate an exception stack and tell the kernel to
// call _asm_pgfault_handler on it.
//
void
set_pgfault_handler(void (*fn)(u_int va, u_int err))
{
	int r;
		// demo2s_code_start;
	if (__pgfault_handler == 0) {
		// Your code here:
		// map one page of exception stack with top at UXSTACKTOP
		// register assembly handler and stack with operating system
	  r = sys_mem_alloc(0,UXSTACKTOP-BY2PG,PTE_W|PTE_U);
	  if(r<0) {
	    panic("!!@@##$$%%^^&&** %e\n",r);
	    return;
	  }
	  r = sys_set_pgfault_handler(0,__asm_pgfault_handler,UXSTACKTOP);
	  if(r<0)
	    panic("!@#$%^&* %e\n",r);
	}
		// demo2s_code_end;
	// Save handler pointer for assembly to call.
	__pgfault_handler = fn;
}


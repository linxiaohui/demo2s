// User-level IPC library routines

#include "lib.h"

// Send val to whom.  This function keeps trying until
// it succeeds.  It should panic() on any error other than
// -E_IPC_NOT_RECV.  
//
// Hint: use sys_yield() to be CPU-friendly.
void
ipc_send(u_int whom, u_int val)
{
	// Your code here.
	panic("ipc_send not implemented");
}

// Receive a value.  Return the value and store the caller's envid
// in *whom.  
//
// Hint: use env to discover the value and who sent it.
u_int
ipc_recv(u_int *whom)
{
	// Your code here
	panic("ipc_recv not implemented");
	return 0;
}


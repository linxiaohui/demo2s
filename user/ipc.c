// User-level IPC library routines

#include "lib.h"

// Send val to whom.  This function keeps trying until
// it succeeds.  It should panic() on any error other than
// -E_IPC_NOT_RECV.  
//
// Hint: use sys_yield() to be CPU-friendly.
void
ipc_send(u_int whom, u_int val, u_int srcva, u_int perm)
{
	//demo2s_code_start;
	int r;

	while ((r=sys_ipc_can_send(whom, val, srcva, perm)) == -E_IPC_NOT_RECV)
    sys_yield();
	if(r == 0)
		return;
	panic("error in ipc_send: %e", r);
	//demo2s_code_end;
}

// Receive a value.  Return the value and store the caller's envid
// in *whom.  
//
// Hint: use env to discover the value and who sent it.
u_int
ipc_recv(u_int *whom, u_int dstva, u_int *perm)
{
	//demo2s_code_start;
	sys_ipc_recv(dstva);
	if (whom)
  *whom=env->env_ipc_from;
	if (perm)
		*perm = env->env_ipc_perm;
  return env->env_ipc_value;
	//demo2s_code_end;
}


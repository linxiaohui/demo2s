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
	//demo2s_code_start;
  int s;
  s=sys_ipc_can_send(whom,val);
  while(s==-E_IPC_NOT_RECV){
    sys_yield();
    s =	sys_ipc_can_send(whom,val);
  }
  if(s<0)
    panic("error happened!\t%d\n",s);
	//demo2s_code_end;
}

// Receive a value.  Return the value and store the caller's envid
// in *whom.  
//
// Hint: use env to discover the value and who sent it.
u_int
ipc_recv(u_int *whom)
{
	//demo2s_code_start;
  sys_ipc_recv();
  *whom=env->env_ipc_from;
  return env->env_ipc_value;
	//demo2s_code_end;
	return 0;
}


/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/printf.h>
#include <kern/sched.h>

// return the current environment id
static u_int
sys_getenvid(void)
{
	return curenv->env_id;
}

// print a string to the screen.
static void
sys_cputs(char *s)
{
	printf("%s", s);
}

// deschedule current environment
static void
sys_yield(void)
{
	sched_yield();
}

// destroy the current environment
static void
sys_env_destroy(void)
{
	printf("[%08x] exiting gracefully\n", curenv->env_id);
	env_destroy(curenv);
}

// Block until a value is ready.  Record that you want to receive,
// mark yourself not runnable, and then give up the CPU.
static void
sys_ipc_recv(void)
{
	// Your code here
	panic("sys_ipc_recv not implemented");
}

// Try to send 'value' to the target env 'envid'.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target has not requested IPC with sys_ipc_recv.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends
//    env_ipc_from is set to the sending envid
//    env_ipc_value is set to the 'value' parameter
// The target environment is marked runnable again.
//
// Return 0 on success, < 0 on error.
//
// Hint: the only function you need to call is envid2env.
static int
sys_ipc_can_send(u_int envid, u_int value)
{
	// Your code here
	panic("sys_ipc_can_send not implemented");
}

// Set envid's pagefault handler entry point and exception stack.
// (xstacktop points one byte past exception stack).
//
// Returns 0 on success, < 0 on error.
static int
sys_set_pgfault_handler(u_int envid, u_int func, u_int xstacktop)
{
	// Your code here.
	panic("sys_set_pgfault_handler not implemented");
}

//
// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
//
// If a page is already mapped at 'va', that page is unmapped as a
// side-effect.
//
// perm -- PTE_U|PTE_P are required, 
//         PTE_AVAIL|PTE_W are optional,
//         but no other bits are allowed (return -E_INVAL)
//
// Return 0 on success, < 0 on error
//	- va must be < UTOP
//	- env may modify its own address space or the address space of its children
// 
static int
sys_mem_alloc(u_int envid, u_int va, u_int perm)
{
	// Your code here.
	panic("sys_mem_alloc not implemented");
}

// Map the page of memory at 'srcva' in srcid's address space
// at 'dstva' in dstid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_mem_alloc.
// (Probably we should add a restriction that you can't go from
// non-writable to writable?)
//
// Return 0 on success, < 0 on error.
//
// Cannot access pages above UTOP.
static int
sys_mem_map(u_int srcid, u_int srcva, u_int dstid, u_int dstva, u_int perm)
{
	// Your code here.
	panic("sys_mem_map not implemented");
}

// Unmap the page of memory at 'va' in the address space of 'envid'
// (if no page is mapped, the function silently succeeds)
//
// Return 0 on success, < 0 on error.
//
// Cannot unmap pages above UTOP.
static int
sys_mem_unmap(u_int envid, u_int va)
{
	// Your code here.
	panic("sys_mem_unmap not implemented");
}

// Allocate a new environment.
//
// The new child is left as env_alloc created it, except that
// status is set to ENV_NOT_RUNNABLE and the register set is copied
// from the current environment.  In the child, the register set is
// tweaked so sys_env_alloc returns 0.
//
// Returns envid of new environment, or < 0 on error.
static int
sys_env_alloc(void)
{
	// Your code here.
	panic("sys_env_alloc not implemented");
}

// Set envid's env_status to status. 
//
// Returns 0 on success, < 0 on error.
// 
// Return -E_INVAL if status is not a valid status for an environment.
static int
sys_set_env_status(u_int envid, u_int status)
{
	// Your code here.
	panic("sys_env_set_status not implemented");
}


// Dispatches to the correct kernel function, passing the arguments.
int
syscall(u_int sn, u_int a1, u_int a2, u_int a3, u_int a4, u_int a5)
{
	// printf("syscall %d %d %d %d from env %d\n", sn, a1, a2, a3, curenv->env_id);

	// Your code here
	panic("syscall not implemented");
}


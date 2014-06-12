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

extern int color;
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
  printf("%s",TRUP(s));
}

static void
sys_set_color(int c)
{

	color=c;
	//printf("%x\t%x\n",c,color);
	
}
// deschedule current environment
static void
sys_yield(void)
{
	sched_yield();
}

// destroy the current environment
static int
sys_env_destroy(u_int envid)
{
	int r;
	struct Env *e;
	if ((r=envid2env(envid, &e, 1)) < 0)
		return r;
#ifdef DEBUG	
	printf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
#endif
	env_destroy(e);
	return 0;
}

// Block until a value is ready.  Record that you want to receive,
// mark yourself not runnable, and then give up the CPU.
static void
sys_ipc_recv(u_int dstva)
{
	//demo2s_code_start;
	if(curenv->env_ipc_recving)
		panic("already recving!");
	if (dstva >= UTOP)
		panic("invalid dstva");
	curenv->env_ipc_recving = 1;
	curenv->env_ipc_dstva = dstva;
	curenv->env_status = ENV_NOT_RUNNABLE;
	sched_yield();
	//demo2s_code_end;
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
sys_ipc_can_send(u_int envid, u_int value, u_int srcva, u_int perm)
{
	int r;
	struct Env *e;
	struct Page *p;
	if ((r=envid2env(envid, &e, 0)) < 0)
		return r;
	if (!e->env_ipc_recving)
	  return -E_IPC_NOT_RECV;
	if (srcva != 0 && e->env_ipc_dstva != 0) {
		if (srcva >= UTOP)
			return -E_INVAL;
		if (((~perm)&(PTE_U|PTE_P)) ||
		    (perm&~(PTE_U|PTE_P|PTE_AVAIL|PTE_W)))
			return -E_INVAL;
		p = page_lookup(curenv->env_pgdir, srcva, 0);
		if (p == 0) {
			printf("[%08x] page_lookup %08x failed in sys_ipc_can_send\n",
				curenv->env_id, srcva);
			return -E_INVAL;
		}
		r = page_insert(e->env_pgdir, p, e->env_ipc_dstva, perm);
		if (r < 0)
			return r;
		e->env_ipc_perm = perm;
	} else {
		e->env_ipc_perm = 0;
	}
	e->env_ipc_recving=0;
	e->env_ipc_from=curenv->env_id;
	e->env_ipc_value=value;
	e->env_status=ENV_RUNNABLE;
	return 0;
//demo2s_code_end;
}

// Set envid's pagefault handler entry point and exception stack.
// (xstacktop points one byte past exception stack).
//
// Returns 0 on success, < 0 on error.
static int
sys_set_pgfault_handler(u_int envid, u_int func, u_int xstacktop)
{
//demo2s_code_start;
  struct Env * 	e;
  int 		i;
  i = envid2env(envid,&e,1);
  if(i<0)
    return i;
  e->env_pgfault_handler=func;
  e->env_xstacktop=xstacktop;
  return 0;
//demo2s_code_end;
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
//demo2s_code_start;
	// Your code here.
	struct Page * 	p;
	struct Env * 	e;
	int 		i;
	if(va>=UTOP) {
	  printf("!!!!%x\n",va);
	  return -E_INVAL;
	}
	i = page_alloc(&p);
	if(i<0) {
	   printf("@@@@%x\n",i);
	  return i;
	}
	i = envid2env(envid,&e,0);
	if(i<0) {
	   printf("####%x\n",va);
	  return i;
	}
	bzero((void*)page2kva(p),BY2PG);
	i = page_insert(e->env_pgdir,p,va,perm);
	if(i<0) {
	  printf("$$$$%x\n",va);
	  return i;
	}
	return 0;
//demo2s_code_end;
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
//demo2s_code_start;
	int 		 s;
	u_long 		 pa;
	struct Env 	*src;
	struct Env 	*dst;
	struct Page 	*p;
	if(srcva>=UTOP||dstva>=UTOP)
		return -E_INVAL;
	s = envid2env(srcid,&src,0);
	if(s<0)
		return s;
	s = envid2env(dstid,&dst,0);
	if(s<0)
		return s;
	p = page_lookup(src->env_pgdir,srcva,0);
	if(p==0)
		return -E_INVAL;
	s = page_insert(dst->env_pgdir,p,dstva,perm);
	return s;
//demo2s_code_end;
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
//demo2s_code_start;
	struct Env * 	e;
	int 		s;
	if(va>=UTOP)
		return -E_INVAL;
	s = envid2env(envid,&e,0);
	if(s<0)
		return s;
	page_remove(e->env_pgdir,va);
	return 0;
//demo2s_code_end;
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
//demo2s_code_start;
	struct Env * 	e;
	int 		s;
	s = env_alloc(&e,curenv->env_id);
	if(s<0)
		return s;
	e->env_status 	 = ENV_NOT_RUNNABLE;
	e->env_tf.tf_eax = 0;
	e->env_tf.tf_ebx = curenv->env_tf.tf_ebx;
	e->env_tf.tf_ecx = curenv->env_tf.tf_ecx;
	e->env_tf.tf_edx = curenv->env_tf.tf_edx;
	e->env_tf.tf_edi = curenv->env_tf.tf_edi;
	e->env_tf.tf_esi = curenv->env_tf.tf_esi;
	e->env_tf.tf_ebp = curenv->env_tf.tf_ebp;
	e->env_tf.tf_es = curenv->env_tf.tf_es;
	e->env_tf.tf_ds = curenv->env_tf.tf_ds;
	e->env_tf.tf_eip = curenv->env_tf.tf_eip;
	e->env_tf.tf_cs  = curenv->env_tf.tf_cs;
	e->env_tf.tf_eflags = curenv->env_tf.tf_eflags;
	e->env_tf.tf_esp = curenv->env_tf.tf_esp;
	e->env_tf.tf_cs  = curenv->env_tf.tf_cs;
	return e->env_id;
//demo2s_code_end;
}

// Set envid's env_status to status. 
//
// Returns 0 on success, < 0 on error.
// 
// Return -E_INVAL if status is not a valid status for an environment.
static int
sys_set_env_status(u_int envid, u_int status)
{
//demo2s_code_start;
	struct Env * 	e;
	int 		s;
	s = envid2env(envid,&e,0);
	if(s<0)
		return s;
	if((status!=ENV_FREE)&&(status!=ENV_RUNNABLE)&&(status!=ENV_NOT_RUNNABLE))
		return -E_INVAL;
	e->env_status = status;
	return 0;
//demo2s_code_end;
}

// Set envid's trap frame to tf.
//
// Returns 0 on success, < 0 on error.
//
// Return -E_INVAL if the environment cannot be manipulated.
static int
sys_set_trapframe(u_int envid, struct Trapframe *tf)
{
	int r;
	struct Env *e;
	struct Trapframe ltf;

	page_fault_mode = PFM_KILL;
	bcopy(TRUP(tf),&ltf,sizeof(struct Trapframe));
	page_fault_mode = PFM_NONE;

	ltf.tf_eflags |= FL_IF;
	ltf.tf_cs |= 3;

	if ((r=envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
	       	bcopy(&ltf,UTF,sizeof(struct Trapframe));
	else
	       	bcopy(&ltf,&e->env_tf,sizeof(struct Trapframe));
	return 0;
}

static int
sys_cgetc()
{
	return cons_getc();
}
static void
sys_panic(char *msg)
{
	// no page_fault_mode -- we are trying to panic!
	panic("%s", TRUP(msg));
}

// Dispatches to the correct kernel function, passing the arguments.
int
syscall(u_int sn, u_int a1, u_int a2, u_int a3, u_int a4, u_int a5)
{
//demo2s_code_start;
	// printf("syscall %d %d %d %d from env %d\n", sn, a1, a2, a3, curenv->env_id);
	page_fault_mode = PFM_KILL;
	
	switch (sn) {
	case SYS_getenvid:
		return sys_getenvid();
	case SYS_cputs:
		sys_cputs(a1);
		return 0;
	case SYS_yield:
		sys_yield();
		return 0;
	case SYS_env_destroy:
		sys_env_destroy(a1);
		return 0;
	case SYS_env_alloc:
		return sys_env_alloc();
	case SYS_ipc_can_send:
		return sys_ipc_can_send(a1,a2,a3,a4);
	case SYS_ipc_recv:
		sys_ipc_recv(a1);
		return 0;
	case SYS_set_pgfault_handler:
		return sys_set_pgfault_handler(a1,a2,a3);
	case SYS_set_env_status:
		return sys_set_env_status(a1,a2);
	case SYS_mem_alloc:
		return sys_mem_alloc(a1,a2,a3);
	case SYS_mem_map:
		return sys_mem_map(a1,a2,a3,a4,a5);
	case SYS_mem_unmap:
		return sys_mem_unmap(a1,a2);
	case SYS_set_trapframe:
		return sys_set_trapframe(a1,a2);
	case SYS_cgetc:
		return sys_cgetc();
	case SYS_set_color:
		sys_set_color(a1);
		return 0;
	case SYS_panic:
		sys_panic(a1);
	default:
		return -E_INVAL;
	}
//demo2s_code_end;
}


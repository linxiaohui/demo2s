/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/sched.h>
#include <kern/printf.h>

struct Env *envs = NULL;		// All environments
struct Env *curenv = NULL;	        // the current env

static struct Env_list env_free_list;	// Free list

//
// Calculates the envid for env e.  
//
static u_int
mkenvid(struct Env *e)
{
	static u_long next_env_id = 0;
	// lower bits of envid hold e's position in the envs array
	u_int idx = e - envs;
	// high bits of envid hold an increasing number
	return(++next_env_id << (1 + LOG2NENV)) | idx;
}

//
// Converts an envid to an env pointer.
//
// RETURNS
//   env pointer -- on success and sets *error = 0
//   NULL -- on failure, and sets *error = the error number
//
struct Env *
envid2env(u_int envid, int *error)
{
	struct Env *e = &envs[ENVX(envid)];
	if (e->env_status == ENV_FREE || e->env_id != envid) {
		*error = -E_BAD_ENV;
		return NULL;
	} else {
		*error = 0;
		return e;
	}
}

//
// Marks all environments in 'envs' as free and inserts them into 
// the env_free_list.  Insert in reverse order, so that
// the first call to env_alloc() returns envs[0].
//
void
env_init(void)
{
    //demo2s_code_start;
  int i;
  LIST_INIT(&env_free_list);
  i=NENV-1;
  while(i>=0) {
    envs[i].env_status=ENV_FREE;//Is this necessary?
    LIST_INSERT_HEAD(&env_free_list,&envs[i],env_link);
    i--;
  }
    //demo2s_code_end;  
}

//
// Initializes the kernel virtual memory layout for environment e.
//
// Allocates a page directory and initializes it.  Sets
// e->env_cr3 and e->env_pgdir accordingly.
//
// RETURNS
//   0 -- on sucess
//   <0 -- otherwise 
//
static int
env_setup_vm(struct Env *e)
{
	// Hint:

	int i, r;
	struct Page *p = NULL;

	// Allocate a page for the page directory
	if ((r = page_alloc(&p)) < 0)
		return r;
    //demo2s_code_start;
    e->env_cr3=page2pa(p);
    e->env_pgdir=page2kva(p);
	// Hint:
	//    - The VA space of all envs is identical above UTOP
	//      (except at VPT and UVPT) 
	//    - Use boot_pgdir
	//    - Do not make any calls to page_alloc 
	//    - Note: pp_refcnt is not maintained for physical pages mapped above UTOP.
    for(i=0;i<PDE2PD;i++)
      *(e->env_pgdir+i)=*(boot_pgdir+i);


    //demo2s_code_end;
	
	// ...except at VPT and UVPT.  These map the env's own page table
	e->env_pgdir[PDX(VPT)]   = e->env_cr3 | PTE_P | PTE_W;
	e->env_pgdir[PDX(UVPT)]  = e->env_cr3 | PTE_P | PTE_U;

	return 0;
}

//
// Allocates and initializes a new env.
//
// RETURNS
//   0 -- on success, sets *new to point at the new env 
//   <0 -- on failure
//
int
env_alloc(struct Env **new, u_int parent_id)
{
	int r;
	struct Env *e;

	if (!(e = LIST_FIRST(&env_free_list)))
		return -E_NO_FREE_ENV;

	if ((r = env_setup_vm(e)) < 0)
		return r;

	e->env_id = mkenvid(e);
	e->env_parent_id = parent_id;
	e->env_status = ENV_RUNNABLE;

	// Set initial values of registers
	//  (lower 2 bits of the seg regs is the RPL -- 3 means user process)
	e->env_tf.tf_ds = GD_UD | 3;
	e->env_tf.tf_es = GD_UD | 3;
	e->env_tf.tf_ss = GD_UD | 3;
	e->env_tf.tf_esp = USTACKTOP;
	e->env_tf.tf_cs = GD_UT | 3;
	// You also need to set tf_eip to the correct value.
	// Hint: see load_icode
	//demo2s_code_start;
	e->env_tf.tf_eip=UTEXT+0x20;
	
	e->env_tf.tf_eflags |= FL_IF;//interrupt 
	//e->env_tf.tf_eflags = 0;
	//demo2s_code_end;
	
	e->env_ipc_blocked = 0;
	e->env_ipc_value = 0;
	e->env_ipc_from = 0;

	e->env_pgfault_handler = 0;
	e->env_xstacktop = 0;

	// commit the allocation
	LIST_REMOVE(e, env_link);
	*new = e;

	return 0;
}

//
// Sets up the the initial stack and program binary for a user process.
//
// This function loads the complete binary image, including a.out header,
// into the environment's user memory starting at virtual address UTEXT,
// and maps one page for the program's initial stack
// at virtual address USTACKTOP - BY2PG.
// Since the a.out header from the binary is mapped at virtual address UTEXT,
// the actual program text starts at virtual address UTEXT+0x20.
//
// This function does not allocate or clear the bss of the loaded program,
// and all mappings are read/write including those of the text segment.
//
static void
load_icode(struct Env *e, u_char *binary, u_int size)
{
	// Hint: 
	//  Use page_alloc, page_insert, page2kva and e->env_pgdir
	//  You must figure out which permissions you'll need
	//  for the different mappings you create.
	//  Remember that the binary image is an a.out format image,
	//  which contains both text and data.
	//demo2s_code_start;
  struct Page * p;
  int i,n;
  n=(size+BY2PG-1)/BY2PG;
  
  if(page_alloc(&p)!=0)
    return;
  page_insert(e->env_pgdir,p,USTACKTOP-BY2PG,PTE_W|PTE_U);//?? permission

  for(i=0;i<n;i++) {
    if(page_alloc(&p)!=0)
      return;
    page_insert(e->env_pgdir,p,UTEXT+i*BY2PG,PTE_W|PTE_U); // ?? permission?
    bcopy(binary+i*BY2PG,page2kva(p),BY2PG);
  }
	//demo2s_code_end;
}

//
// Allocates a new env and loads the a.out binary into it.
//  - new env's parent env id is 0
void
env_create(u_char *binary, int size)
{
    //demo2s_code_start;
  struct Env * e;
  int status;
  if((status=env_alloc(&e,0))<0) {
    panic("error %e\n",status);
    return;
  }

  printf("alloc env success\t%d\n",e-envs);

  load_icode(e,binary,size);
	//demo2s_code_end;
}

//
// Frees env e and all memory it uses.
// 
void
env_free(struct Env *e)
{
	// For lab 3, env_free() doesn't really do
	// anything (except leak memory).  We'll fix
	// this in later labs.

	e->env_status = ENV_FREE;
	LIST_INSERT_HEAD(&env_free_list, e, env_link);
}

//
// Frees env e.  And schedules a new env
// if e is the current env.
//
void
env_destroy(struct Env *e) 
{
	env_free(e);
	if (curenv == e) {
		curenv = NULL;
		sched_yield();
	}
}


//
// Restores the register values in the Trapframe
//  (does not return)
//
void
env_pop_tf(struct Trapframe *tf)
{
#if 1
	printf(" --> %d 0x%x\n", ENVX(curenv->env_id), tf->tf_eip);
#endif

	asm volatile("movl %0,%%esp\n"
		"\tpopal\n"
		"\tpopl %%es\n"
		"\tpopl %%ds\n"
		"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
		"\tiret"
		:: "g" (tf) : "memory");
	panic("iret failed");  /* mostly to placate the compiler */
}


//
// Context switch from curenv to env e.
// Note: if this is the first call to env_run, curenv is NULL.
//  (This function does not return.)
//
void
env_run(struct Env *e)
{
	// step 1: save register state of curenv
	// step 2: set curenv
	// step 3: use lcr3
	// step 4: use env_pop_tf()
	//demo2s_code_start;
  if(curenv)
   bcopy(UTF,&curenv->env_tf,sizeof(struct Trapframe));

  curenv=e;
  lcr3(e->env_cr3);

  env_pop_tf(&e->env_tf);
	//demo2s_code_end;
// Hint: Skip step 1 until exercise 4.  You don't
// need it for exercise 1, and in exercise 4 you'll better
// understand what you need to do.
  
}



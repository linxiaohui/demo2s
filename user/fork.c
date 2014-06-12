// implement fork from user space

#include "lib.h"

#define PTE_COW		0x800

extern void __asm_pgfault_handler(void);

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//

static void
pgfault(u_int va, u_int err)
{
	int r;
	u_char *tmp;

	u_int 	addr;	
	Pte 	pte;

	pte = vpt[va/BY2PG];
	
	if((err&FEC_WR)&&(pte&PTE_COW)) {
	  
	  tmp =	(u_char*)(UTEXT-BY2PG);

	  if((r=sys_mem_alloc(0,tmp,PTE_P|PTE_U|PTE_W))<0) {
	    panic("fork.c pgfault: %e",r);
	  }
	  
	  
	  bcopy((u_char*)ROUNDDOWN(va,BY2PG),tmp,BY2PG);//
	  
	  if((r=sys_mem_map(0,tmp,0,ROUNDDOWN(va,BY2PG),PTE_P|PTE_U|PTE_W))<0)
	    panic("sys_mem_map: %e",r);
	  
	  if((r=sys_mem_unmap(0,tmp))<0)
	    panic("sys_mem_unmap: %e",r);  
	  
	}
	else
	  panic("???");

//demo2s_code_end;
}

//
// Map our virtual page pn (address pn*BY2PG) into the target envid
// at the same virtual address.  if the page is writable or copy-on-write,
// the new mapping must be created copy on write and then our mapping must be
// marked copy on write as well.  (Exercise: why mark ours copy-on-write again if
// it was already copy-on-write?)
// 
static void
duppage(u_int envid, u_int pn)
{
	int r;
	u_int addr;
	Pte pte;
//demo2s_code_start;
	// Your code here.
	addr = pn*BY2PG;
	pte=vpt[pn];

	if((pte&PTE_P)&&(pte&PTE_U)){
	  
	  if(pte&PTE_W||pte&PTE_COW) {
	    
	    r =	sys_mem_map(0,addr,envid,addr,PTE_COW|PTE_U|PTE_P);
	    if(r<0)
	      panic("duppage mem_map");
	    // sys_mem_unmap(0,addr);
	    r = sys_mem_map(envid,addr,0,addr,PTE_COW|PTE_U|PTE_P);
	    if(r<0)
	     panic("duppage mem_map");
	    
	  }
	  else
	    sys_mem_map(0,addr,envid,addr,PTE_U|PTE_P);
	  
	}
//demo2s_code_end;	
}

//
// User-level fork.  Create a child and then copy our address space
// and page fault handler setup to the child.
//
// Hint: use vpd, vpt, and duppage.
// Hint: remember to fix "env" in the child process!
// 
int
fork(void)
{
	int 	addr, envid, r,i;
//demo2s_code_start;
	set_pgfault_handler(pgfault);

	envid = sys_env_alloc();
	
	if (envid < 0)
		return envid;

	if (envid == 0) {
		env = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	/**/
	for(i=0;i<USTACKTOP/BY2PG;) {	/**/
		if(vpd[i>>10]&PTE_P){
			if(vpt[i]&PTE_P)
				duppage(envid,i);
			i++;
		}
		else {
			i += PDE2PD;
		}
	}

	r = sys_mem_alloc(envid,UXSTACKTOP-BY2PG,PTE_P|PTE_U|PTE_W);
	if(r<0)
		return r;

	r = sys_set_pgfault_handler(envid,__asm_pgfault_handler,UXSTACKTOP);
	if(r<0)
		return r;

	r = sys_set_env_status(envid,ENV_RUNNABLE);
	if(r<0)
		return r;

	return envid;
//demo2s_code_end;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}

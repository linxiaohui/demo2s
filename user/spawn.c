#include "lib.h"
#include <inc/aout.h>

#define TMPPAGE		(BY2PG)
#define TMPPAGETOP	(TMPPAGE+BY2PG)

// Set up the initial stack page for the new child process with envid 'child',
// using the arguments array pointed to by 'argv',
// which is a null-terminated array of pointers to null-terminated strings.
//
// On success, returns 0 and sets *init_esp
// to the initial stack pointer with which the child should start.
// Returns < 0 on failure.
//
static int
init_stack(u_int child, char **argv, u_int *init_esp)
{
	int argc, i, r, tot;
	char *strings;
	u_int *args;

	// Count the number of arguments (argc)
	// and the total amount of space needed for strings (tot)
	tot = 0;
	for (argc=0; argv[argc]; argc++)
		tot += strlen(argv[argc])+1;

	// Make sure everything will fit in the initial stack page
	if (ROUND(tot, 4)+4*(argc+3) > BY2PG)
		return -E_NO_MEM;

	// Determine where to place the strings and the args array
	strings = (char*)TMPPAGETOP - tot;
	args = (u_int*)(TMPPAGETOP - ROUND(tot, 4) - 4*(argc+1));

	if ((r = sys_mem_alloc(0, TMPPAGE, PTE_P|PTE_U|PTE_W)) < 0)
		return r;

	// Replace this with your code to:
	//
	//	- copy the argument strings into the stack page at 'strings'
	//
	//	- initialize args[0..argc-1] to be pointers to these strings
	//	  that will be valid addresses for the child environment
	//	  (for whom this page will be at USTACKTOP-BY2PG!).
	//
	//	- set args[argc] to 0 to null-terminate the args array.
	//
	//	- push two more words onto the child's stack below 'args',
	//	  containing the argc and argv parameters to be passed
	//	  to the child's umain() function.
	//
	//	- set *init_esp to the initial stack pointer for the child
	//
	*init_esp = USTACKTOP;	// Change this!

	if ((r = sys_mem_map(0, TMPPAGE, child, USTACKTOP-BY2PG, PTE_P|PTE_U|PTE_W)) < 0)
		goto error;
	if ((r = sys_mem_unmap(0, TMPPAGE)) < 0)
		goto error;

	return 0;

error:
	sys_mem_unmap(0, TMPPAGE);
	return r;
}

// Spawn a child process from a program image loaded from the file system.
// prog: the pathname of the program to run.
// argv: pointer to null-terminated array of pointers to strings,
// 	 which will be passed to the child as its command-line arguments.
// Returns child envid on success, < 0 on failure.
int
spawn(char *prog, char **argv)
{
	// Insert your code, following approximately this procedure:
	//
	//	- Open the program file and read the a.out header.
	//
	//	- Use sys_env_alloc() to create a new environment.
	//
	//	- Call the init_stack() function above to set up
	//	  the initial stack page for the child environment.
	//
	//	- Map the program's text segment, from file offset 0
	//	  through file offset aout.a_text-1, starting at
	//	  virtual address UTEXT in the child.
	//	  Use read_map() and map the pages it returns directly
	//	  into the child so that multiple instances of the
	//	  same program will share the same copy of the program text.
	//	  Be sure to map the program text read-only in the child.
	//
	//	- Set up the child process's data segment.  For each page,
	//	  allocate a page in the parent temporarily at TMPPAGE,
	//	  read() the appropriate block of the file into that page,
	//	  and then insert that page mapping into the child.
	//	  Look at init_stack() for inspiration.
	//	  Be sure you understand why you can't use read_map() here.
	//
	//	- Set up the child process's bss segment.
	//	  All you need to do here is sys_mem_alloc() the pages
	//	  directly into the child's address space, because
	//	  sys_mem_alloc() automatically zeroes the pages it allocates.
	//
	//	- Use the new sys_set_trapframe() call to set up the
	//	  correct initial eip and esp register values in the child.
	//	  You can use envs[ENVX(child)].env_tf as a template trapframe
	//	  in order to get the initial segment registers and such.
	//
	//	- Start the child process running with sys_set_env_status().
	//
	// Set up program text, data, bss
	panic("spawn unimplemented!");
}

// Spawn, taking command-line arguments array directly on the stack.
int
spawnl(char *prog, char *args, ...)
{
	return spawn(prog, &args);
}


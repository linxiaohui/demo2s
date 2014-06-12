#include "lib.h"
#include <inc/aout.h>

#define TMPPAGE		(BY2PG)
#define TMPPAGETOP	(TMPPAGE+BY2PG)

#define debug 0
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

	//demo2s_code_start;
	//
	//	- copy the argument strings into the stack page at 'strings'
	//
	//	- initialize args[0..argc-1] to be pointers to these strings
	//	  that will be valid addresses for the child environment
	//	  (for whom this page will be at USTACKTOP-BY2PG!).
	for(i=0;i<argc;i++) {
		strcpy(strings,argv[i]);
		args[i]	 = strings+USTACKTOP-TMPPAGETOP;
		strings	+= strlen(argv[i])+1;
	}
	/*
	 printf("*****************\n");
	printf("strings is %08x\n",strings);
	printf("args:\n");
	for(i=0;i<argc;i++) {
		printf("addr %08x: ",args[i]);
		printf("argvs %s\n",argv[i]);
	}
	*/
	//	- set args[argc] to 0 to null-terminate the args array.
	args[argc] = 0;
	//
	//	- push two more words onto the child's stack below 'args',
	//	  containing the argc and argv parameters to be passed
	//	  to the child's umain() function.
	args[-1]   = (USTACKTOP-ROUND(tot,4)-4*(argc+1));
	args[-2]   = argc;
	//
	//	- set *init_esp to the initial stack pointer for the child
	//
	*init_esp = args[-1]-8;
	//demo2s_code_end;
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
	//demo2s_code_start;
	int 	fd; //file descriptor
	int 	r;  //result
	int 	cid; //child id
	u_int 	end_addr;// end address of a segment
	u_int 	beg_addr;//begin address of a segment
	u_int 	init_esp;
	u_int*  addr;
	//
	struct Aout 		haout;
	struct Trapframe 	tf;
	//	- Open the program file and read the a.out header.
	if((fd=open(prog,O_RDONLY))<0) {
		if(debug) printf("open error\n");
		return fd;
	}
	if((r=read(fd,&haout,sizeof(struct Aout)))<0) {
		if(debug) printf("read head error!\n");
		return r;
	}
	// check it whether is an executable file?
	//printf("%x\n",haout.a_entry);
	//printf("%x\n",haout.a_magic);
	if(haout.a_entry!=0x20+UTEXT) {
		printf("%C",F_RED);
		printf("%s is not an executable file\n",prog);
		printf("%C",F_DEFAULT);
		return -E_INVAL;
	}
	
	//	- Use sys_env_alloc() to create a new environment.
	if((cid=sys_env_alloc())<0) {
		printf("env_alloc error\n");
		return cid;
	}
	//
	//	- Call the init_stack() function above to set up
	//	  the initial stack page for the child environment.
	if((r=init_stack(cid,argv,&init_esp))<0) {
		if(debug) printf("init_stack error\n");
		return r;
	}
	//
	//	- Map the program's text segment, from file offset 0
	//	  through file offset aout.a_text-1, starting at
	//	  virtual address UTEXT in the child.
	//	  Use read_map() and map the pages it returns directly
	//	  into the child so that multiple instances of the
	//	  same program will share the same copy of the program text.
	end_addr = ROUND(UTEXT+haout.a_text,BY2PG);
	beg_addr = UTEXT;
	for(;beg_addr<end_addr;beg_addr+=BY2PG) {
		if((r=read_map(fd,beg_addr-UTEXT,&addr))<0)
			return r;
	//	  Be sure to map the program text read-only in the child.
		if((r=sys_mem_map(0,addr,cid,beg_addr,PTE_P|PTE_U))<0)
			return r;
	}
	//
	//	- Set up the child process's data segment.  For each page,
	//	  allocate a page in the parent temporarily at TMPPAGE,
	//	  read() the appropriate block of the file into that page,
	//	  and then insert that page mapping into the child.
	//	  Look at init_stack() for inspiration.
	// The data segment starts where the text segment left off,starts on a page boundary.
	// The data size will always be a multiple of the page size,ends on a page boundary.
	if((r=seek(fd,haout.a_text))<0)
		return r;
	beg_addr = end_addr;
	end_addr = ROUND(beg_addr+haout.a_data,BY2PG);
	for(;beg_addr<end_addr;beg_addr+=BY2PG) {
		if((r=sys_mem_alloc(0,TMPPAGE,PTE_P|PTE_U|PTE_W))<0)
			return r;
		if((r=read(fd,TMPPAGE,BY2PG))<0)
			return r;
		if((r=sys_mem_map(0,TMPPAGE,cid,beg_addr,PTE_P|PTE_U|PTE_W))<0)
			return r;
	}
	//
	//Set up the child process's bss segment.
	// what need to be done here is sys_mem_alloc() the pages
	// directly into the child's address space, because sys_mem_alloc() 
	// automatically zeroes the pages it allocates.
	//
	// The bss will start page aligned (since it picks up where the
	// data segment left off), but it's length may not be a multiple
	// of the page size, so it may not end on a page boundary.
	// (But it's okay to map the whole last page
	// even though the program will only need part of it.)
	beg_addr = end_addr;
	end_addr = ROUND(beg_addr+haout.a_bss,BY2PG);
	for(;beg_addr<end_addr;beg_addr+=BY2PG) {
		if((r=sys_mem_alloc(cid,beg_addr,PTE_P|PTE_U|PTE_W))<0)
			return r;
	}
	// The bss is not read from the binary file.  It is simply 
	// allocated as zeroed memory.  There are bits in the file at
	// offset aout.a_text+aout.a_data, but they are not the
	// bss.  They are the symbol table, which is only for debuggers.
	// Do not use them.
	//
	// The exact location of the bss ?in the file? is a bit confusing, because
	// the linker lies to the loader about where it is.  
	// For example, in the copy of user/init that we have (yours
	// will depend on the size of your implementation of open and close),
	// i386-osclass-aout-nm claims that the bss starts at 0x8067c0
	// and ends at 0x807f40 (file offsets 0x67c0 to 0x7f40).
	// However, since this is not page aligned,
	// it lies to the loader, inserting some extra zeros at the end
	// of the data section to page-align the end, and then claims
	// that the data (which starts at 0x2000) is 0x5000 long, ending
	// at 0x7000, and that the bss is 0xf40 long, making it run from
	// 0x7000 to 0x7f40.  This has the same effect as far as the
	// loading of the program.  Offsets 0x8067c0 to 0x807f40 
	// end up being filled with zeros, but they come from different
	// places -- the ones in the 0x806 page come from the binary file
	// as part of the data segment, but the ones in the 0x807 page
	// are just fresh zeroed pages not read from anywhere.
	//
	// Remember that the symbol table,is not likely to match what's in the a.out header.
	// Use the a.out header alone.
	// Close the file
	if((r=close(fd))<0) {
		if(debug) printf("close %e!\n",r);
		return r;
	}
	
	
	//
	//Use the sys_set_trapframe() call to set up the correct initial
	// eip and esp register values in the child.
	// using envs[ENVX(cid)].env_tf as a template trapframe
	// in order to get the initial segment registers and such.
	bcopy(&envs[ENVX(cid)].env_tf,&tf,sizeof(struct Trapframe));
	tf.tf_esp=init_esp;
	tf.tf_eip=0x20+UTEXT;

	if((r=sys_set_trapframe(cid,&tf))<0)
		return r;
	int i;
	for(i=0;i<USTACKTOP/BY2PG;) {	/**/
		if(vpd[i>>10]&PTE_P){
			if((vpt[i]&PTE_P)&&(vpt[i]&PTE_LIBRARY))
				sys_mem_map(0,i*BY2PG,cid,BY2PG*i,PTE_USER);
			i++;
		}
		else {
			i += PDE2PD;
		}
	}
		
	//Start the child process running with sys_set_env_status().
	if((r=sys_set_env_status(cid,ENV_RUNNABLE))<0)
		return r;

	return cid;
	
	//demo2s_code_end;
}

// Spawn, taking command-line arguments array directly on the stack.
int
spawnl(char *prog, char *args, ...)
{
	return spawn(prog, &args);
}


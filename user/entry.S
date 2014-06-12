#include <inc/mmu.h>

.data
	.globl envs
	.set envs, UENVS
	.globl pages
	.set pages, UPAGES
	.globl vpt
	.set vpt, UVPT
	.globl vpd
	.set vpd, (UVPT+(UVPT>>12)*4)

.text
.globl _start
_start:
	// push empty argc, argv if none present
	cmpl $USTACKTOP, %esp
	jne args_exist
	pushl $0
	pushl $0

	// If no args, kernel set us up -- clear the bss
	// since the kernel did not.  This depends on the
	// bss not extending into a new page, since otherwise
	// we will page fault and die.
	movl $(end), %eax
	subl $(_edata), %eax
	pushl %eax
	pushl $(_edata)
	call bzero

args_exist:
	call libmain
1:      jmp 1b


.data
.globl __pgfault_handler
__pgfault_handler:
	.long 0

// On entry there is a trap frame on the stack:
//	[ 5 spare words ]
//	trap-time eip
//	trap-time eflags
//	trap-time esp
//	trap-time errcode
//	trap-time va		<-- %esp

.text
.globl __asm_pgfault_handler
__asm_pgfault_handler:
	// save the caller-save registers
	//	(your code here)

	// call the C page fault handler
	movl __pgfault_handler, %eax
	call *%eax

	// push trap-time eip, eflags onto trap-time stack
	//	(your code here)

	// restore the caller-save registers
	//	(your code here)

	// switch to the trap-time stack 
	//	(your code here)

	// restore eflags and eip from the stack
	//	(your code here)
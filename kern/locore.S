/* See COPYRIGHT for copyright information. */


#include <inc/asm.h>
#include <inc/mmu.h>
#include <inc/trap.h>
#include <kern/picirq.h>

###################################################################
# The kernel(this code) is linked at address ~(KERNBASE + 1 Meg), 
# but the bootloader load it at address	~1 Meg.
#	
# RELOC(x) maps a symbol x from its link address to its actual
# location in physical memory(its load address).	 
###################################################################
	
#define	RELOC(x) ((x) - KERNBASE)
	
###################################################################	
# See mmu.h for a complete description of these two.
###################################################################
.data
	.globl	_vpt
	.set	_vpt, VPT
	.globl	_vpd
	.set	_vpd, (VPT + SRL(VPT, 10))

###################################################################
# boot stack
###################################################################
	.p2align	PGSHIFT		# force page alignment
	.globl		bootstack
bootstack:
	.space		KSTKSIZE
	.globl		bootstacktop   
bootstacktop:

###################################################################
# setup the GDT	
###################################################################
.set CODE_SEL,0x8				# index of code seg within mygdt
.set DATA_SEL,0x10				# index of data seg within mygdt
.p2align	2				# force 4 byte alignment
mygdt:
	SEG_NULL				# null seg
	SEG(STA_X|STA_R, -KERNBASE, 0xffffffff)	# code seg
	SEG(STA_W, -KERNBASE, 0xffffffff)	# data seg
mygdtdesc:
	.word	0x17			# sizeof(mygdt) - 1
	.long	RELOC(mygdt)		# address mygdt
	
###################################################################
# entry point
###################################################################

.text
.globl		start
start:
	movw	$0x1234,0x472			# warm boot
	lgdt	RELOC(mygdtdesc)		# load descriptor
	movl	$DATA_SEL, %eax			# reload seg regs
	movw	%ax,%es				#
	movw	%ax,%ds				#
	movw	%ax,%ss				#
	ljmp	$CODE_SEL,$relocated		# reload CS
relocated:
	movl	$0x0,%ebp			# And nuke frame pointer

        # Leave a few words on the stack for the user trap frame UTF
	# XXX use sizeof(.)
	movl	$(bootstacktop-0x40),%esp	# set the stack pointer 

	/* Zero the BSS */
	xorl	%eax,%eax			# quickest way to get zero
	cld					# d=0 string op is ascending
	movl	$_edata,%edi			# destination of string op
	movl	$(3 + end),%ecx		# repeat count for "rep"
	subl	$_edata,%ecx			#  = ceil((_end-_edata)/4)
	shr	$2, %ecx
	rep
	 stosl					# store it!
	
	# now to C code
	call	i386_init
	
	
###################################################################
# exceptions/interrupts
###################################################################

/* For certain traps the CPU automatically pushes an error code, for 
 * all other traps the IDTFUNC_NOEC() pushes a 0 in place of the error code,
 * so the trap frame has the same format.
 */
#define IDTFNC(name,num)      ENTRY(name)           pushl $(num); jmp _alltraps
#define IDTFNC_NOEC(name,num) ENTRY(name) pushl $0; pushl $(num); jmp _alltraps 



.globl _clock_interrupt
_clock_interrupt:
	pushl	%eax		# push caller-saves
	pushl	%ecx		#  (clock() might trash these)
	pushl	%edx		#
	pushl   %ds		# load kernel seg regs 
	pushl	%es		#  saving the current contents 
	movl $GD_KD,%eax	
	movw %ax,%ds
	movw %ax,%es
	call	clock		# call C handler
	popl	%es		# restore seg regs
	popl	%ds
	popl	%edx
	popl	%ecx
	popl	%eax
	iret			# return to whence we came..

# demo2s_code_start;

IDTFNC_NOEC(divide,T_DIVIDE)	;
IDTFNC_NOEC(debug,T_DEBUG)	;
IDTFNC_NOEC(nmi,T_NMI)		;
IDTFNC_NOEC(brkpt,T_BRKPT)	;
IDTFNC_NOEC(oflow,T_OFLOW)	;
IDTFNC_NOEC(bound,T_BOUND)	;
IDTFNC_NOEC(illop,T_ILLOP)	;
IDTFNC_NOEC(device,T_DEVICE)	;
IDTFNC(dblflt,T_DBLFLT)		;
	;;
IDTFNC(tss,T_TSS)		;
IDTFNC(segnp,T_SEGNP)		;
IDTFNC(stack,T_STACK)		;
IDTFNC(gpflt,T_GPFLT)		;
IDTFNC(pgflt,T_PGFLT)		;
	;;
IDTFNC(fperr,T_FPERR)		;
IDTFNC(align,T_ALIGN)		;
IDTFNC(mchk,T_MCHK)		;
	;;

IDTFNC_NOEC(systemcall,T_SYSCALL)	; 
IDTFNC_NOEC(clockrpt,IRQ_OFFSET)	;
	
_alltraps:
	pushl	%ds
	pushl	%es
	pushal
	
	movl $GD_KD,%eax	
	movw %ax,%ds
	movw %ax,%es
	
	pushl %esp
	
	call trap
	
	popl %eax
	
	popal
	popl	%es	
	popl	%ds
	
	addl $8,%esp
		
	iret
# demo2s_code_end;

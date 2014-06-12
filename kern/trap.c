#include <inc/mmu.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/env.h>
#include <kern/sched.h>
#include <kern/console.h>
#include <kern/printf.h>
#include <kern/picirq.h>
#include <kern/kclock.h>
    //demo2s_code_start;

//exercise 2
//extern void _clock_interrupt(void);
//extern _clock_interrupt; //ERROR !!

extern void _divide(void);
extern void _debug(void);
extern void _nmi(void);
extern void _brkpt(void);
extern void _oflow(void);
extern void _bound(void);
extern void _illop(void);
extern void _device(void);
extern void _dblflt(void);
extern void _tss(void);
extern void _segnp(void);
extern void _stack(void);
extern void _gpflt(void);
extern void _pgflt(void);
extern void _fperr(void);
extern void _align(void);
extern void _mchk(void);

extern void _clockrpt(void);

//exercise 3
extern void clock(void);

//demo2s_code_end;


	
u_int page_fault_mode = PFM_NONE;
static struct Taskstate ts;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { {0}, };
struct Pseudodesc idt_pd =
{
	0, sizeof(idt) - 1, (unsigned long) idt,
};


void
idt_init(void)
{
	extern struct Segdesc gdt[];

	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	ts.ts_esp0 = KSTACKTOP;
	ts.ts_ss0 = GD_KD;

	// Love to put this code in the initialization of gdt, but
	// the compiler generates an error incorrectly.
	gdt[GD_TSS >> 3] = SEG16(STS_T32A, (u_long) (&ts),
					sizeof(struct Taskstate), 0);
	gdt[GD_TSS >> 3].sd_s = 0;

	// Load the TSS
	ltr(GD_TSS);

//demo2s_code_start;
    // exceptions
    SETGATE(idt[T_DIVIDE],0,0x8,_divide,0);
    SETGATE(idt[T_DEBUG],0,0x8,_debug,0);
    SETGATE(idt[T_NMI],0,0x8,_nmi,0);
    SETGATE(idt[T_BRKPT],0,0x8,_brkpt,3); //
    SETGATE(idt[T_OFLOW],0,0x8,_oflow,0);
    SETGATE(idt[T_BOUND],0,0x8,_bound,0);
    SETGATE(idt[T_ILLOP],0,0x8,_illop,0);
    SETGATE(idt[T_DEVICE],0,0x8,_device,0);
    SETGATE(idt[T_DBLFLT],0,0x8,_dblflt,0);
    SETGATE(idt[T_TSS],0,0x8,_tss,0);
    SETGATE(idt[T_SEGNP],0,0x8,_segnp,0);
    SETGATE(idt[T_STACK],0,0x8,_stack,0);
    SETGATE(idt[T_GPFLT],0,0x8,_gpflt,0);
    SETGATE(idt[T_PGFLT],0,0x8,_pgflt,0);
    SETGATE(idt[T_FPERR],0,0x8,_fperr,0);
    SETGATE(idt[T_ALIGN],0,0x8,_align,0);
    SETGATE(idt[T_MCHK],0,0x8,_mchk,0);

    // interrupt
    SETGATE(idt[IRQ_OFFSET],0,0x8,_clockrpt,0);
    
    

    //exercise 2
    //SETGATE(idt[0+IRQ_OFFSET],0,0x8,_clock_interrupt,0);//dpl ?
    
//demo2s_code_end;

	// Load the IDT
	asm volatile("lidt idt_pd+2");
}


static void
print_trapframe(struct Trapframe *tf)
{
	printf("TRAP frame at %p\n", tf);
	printf("	edi  0x%x\n", tf->tf_edi);
	printf("	esi  0x%x\n", tf->tf_esi);
	printf("	ebp  0x%x\n", tf->tf_ebp);
	printf("	oesp 0x%x\n", tf->tf_oesp);
	printf("	ebx  0x%x\n", tf->tf_ebx);
	printf("	edx  0x%x\n", tf->tf_edx);
	printf("	ecx  0x%x\n", tf->tf_ecx);
	printf("	eax  0x%x\n", tf->tf_eax);
	printf("	es   0x%x\n", tf->tf_es);
	printf("	ds   0x%x\n", tf->tf_ds);
	printf("	trap 0x%x\n", tf->tf_trapno);
	printf("	err  0x%x\n", tf->tf_err);
	printf("	eip  0x%x\n", tf->tf_eip);
	printf("	cs   0x%x\n", tf->tf_cs);
	printf("	flag 0x%x\n", tf->tf_eflags);
	printf("	esp  0x%x\n", tf->tf_esp);
	printf("	ss   0x%x\n", tf->tf_ss);
}

void
trap(struct Trapframe *tf)
{

    //demo2s_code_start;
  if (tf->tf_trapno == IRQ_OFFSET+0) {
		// irq 0 -- clock interrupt
		//panic("clock interrupt");
    // print_trapframe(tf);
      
    //exercise 3
    //clock();
    
    sched_yield();
    return;//sched_yield never returned
  }
  //demo2s_code_end;
  if (IRQ_OFFSET <= tf->tf_trapno 
			&& tf->tf_trapno < IRQ_OFFSET+MAX_IRQS) {
		// just ingore spurious interrupts
		printf("spurious interrupt on irq %d\n",
			tf->tf_trapno - IRQ_OFFSET);
		print_trapframe(tf);
		return;
	}

	// the user process or the kernel has a bug.
	print_trapframe(tf);
	panic("unhandled trap");
}




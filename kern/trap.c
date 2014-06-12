#include <inc/mmu.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/env.h>
#include <kern/sched.h>
#include <kern/console.h>
#include <kern/printf.h>
#include <kern/picirq.h>
#include <kern/kclock.h>

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

	if (tf->tf_trapno == IRQ_OFFSET+0) {
		// irq 0 -- clock interrupt
		panic("clock interrupt");
	}
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




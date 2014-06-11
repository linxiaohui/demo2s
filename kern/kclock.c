/* See COPYRIGHT for copyright information. */

/* The Run Time Clock and other NVRAM access functions that go with it. */
/* The run time clock is hard-wired to IRQ8. */

#include <inc/x86.h>
#include <inc/isareg.h>
#include <inc/timerreg.h>
#include <kern/picirq.h>
#include <kern/env.h>
#include <kern/printf.h>
#include <kern/kclock.h>


u_int
mc146818_read(void *sc, u_int reg)
{
	outb(IO_RTC, reg);
	return(inb(IO_RTC+1));
}

void
mc146818_write(void *sc, u_int reg, u_int datum)
{
	outb(IO_RTC, reg);
	outb(IO_RTC+1, datum);
}




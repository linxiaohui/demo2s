#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/picirq.h>
#include <kern/printf.h>

// Trivial temporary clock interrupt handler,
// called from clock_interrupt in locore.S
void
clock(void)
{
	printf("*");
}


// The real clock interrupt handler,
// implementing round-robin scheduling
void
sched_yield(void)
{
	assert(envs[0].env_status == ENV_RUNNABLE);
	env_run(&envs[0]);
}


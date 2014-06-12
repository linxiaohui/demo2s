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
    //demo2s_code_start;
#if 0
  if(curenv==NULL) {
    curenv=envs;
    while(curenv-envs<NENV) {
      if(curenv->env_status==ENV_RUNNABLE) {
        env_run(curenv);
      }
      else
        curenv++;
    }
    curenv=NULL;
  }//if
  else {
    while(curenv-envs<NENV) {
      curenv++;
      if(curenv->env_status==ENV_RUNNABLE) {
        env_run(curenv);
      }//if
    }//while
    curenv=envs;
    while(curenv-envs<NENV) {
      if(curenv->env_status==ENV_RUNNABLE)
        env_run(curenv);
      else
        curenv++;
    }//while
  }//else

#endif

  int i;

  if(curenv==NULL) {
    i=0;
    while(i<NENV) {
      if((envs+i)->env_status==ENV_RUNNABLE)
        env_run(envs+i);
      else
       i++;
    }//while
  }//if
  else {
    i=0;
    while(i<NENV-(curenv-envs)) {
      i++;
      if((curenv+i)->env_status==ENV_RUNNABLE) 
        env_run(curenv+i);
    }//while
    i=0;
    while(i<NENV) {
      if((envs+i)->env_status==ENV_RUNNABLE)
        env_run(envs+i);
      else
        i++;
    }//while
  }//else
    //demo2s_code_end;
}


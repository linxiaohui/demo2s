/* See COPYRIGHT for copyright information. */

#include <inc/asm.h>
#include <kern/trap.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/console.h>
#include <kern/printf.h>
#include <kern/picirq.h>
#include <kern/kclock.h>
#include <kern/sched.h>

void
i386_init(void)
{
	// can't call printf until after cons_init()
	cons_init();

	printf("6828 decimal is %o octal!\n", 6828);

	// Lab 2 initialization functions
	i386_detect_memory();
	i386_vm_init();
	page_init();
	page_check();

	// demo2s_code_start;
    // asm volatile("jmp ."); //the is added here to find the eip here
    printf("Here test the part 3\n");
    
    {
       int x=1,y=3,z=4;
      //int y=3,z=4,x=1;
      
      warn("x %d,y %x,z %d",x,y,z);
      //warn("x %d,y %x,z %d\n",z,y,x);

      u_int i=0x00646c72;
      warn("H%x Wo%s",57616,&i);

      asm volatile("movl $0x1,%eax\n\tmovl %eax,%ss:0x8(%esp)");
      warn("x=%d y=%d",3);
      
    }
	// demo2s_code_end;
	    
	// Lab 3 initialization functions
	idt_init();
	pic_init();
	kclock_init();
	env_init();

	// Temporary test code specific to LAB 3
#if defined(TEST_START)
	{
		// Don't touch this!  Used by the grading script.
		extern u_char TEST_START, TEST_END;
		env_create(&TEST_START, &TEST_END - &TEST_START);
	}
#elif defined(TEST_ALICEBOB)
	{
		// Don't touch this!  Used by the grading script.
		extern u_char _alice_start, _alice_end, _bob_start, _bob_end;
		env_create(&_alice_start, &_alice_end - &_alice_start);
		env_create(&_bob_start, &_bob_end - &_bob_start);

	//demo2s_code_start;
	extern u_char spin_start,spin_end;
	env_create(&spin_start,&spin_end-&spin_start);
	//demo2s_code_end;
	}
#else
	{
		// Do whatever you want here for your own testing purposes.
		extern u_char spin_start;
		extern u_char spin_end;
		env_create(&spin_start, &spin_end - &spin_start);
	}
#endif


	sched_yield();
	panic("init.c: end of i386_init() reached!");
}


// like bcopy(but doesn't handle overlapping src/dst)
void
bcopy(const void *src, void *dst, size_t len)
{
	void *max;

	max = dst + len;
	// copy machine words while possible
	while (dst + 3 < max)
		*(int *)dst++ = *(int *)src++;
	// finish remaining 0-3 bytes
	while (dst < max)
		*(char *)dst++ = *(char *)src++;
}

void
bzero(void *b, size_t len)
{
	void *max;

	max = b + len;
	// zero machine words while possible
	while (b + 3 < max)
		*(int *)b++ = 0;
	// finish remaining 0-3 bytes
	while (b < max)
		*(char *)b++ = 0;
}

// Ignore from here on down.  The functions below here are never
// called.  They are just here to get around a linking problem.

void
abort(void)
{
	panic("abort");
}

void *
malloc(size_t size)
{
	panic("malloc: size %d", size);
}

void
free(void *ptr)
{
	panic("free: ptr %p", ptr);
}

int
atexit(void(*function)(void))
{
	panic("atexit: function %p", function);
}

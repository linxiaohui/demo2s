#!/bin/sh


fault() {
	perl -e "print '$1: '"
	(
		rm kern/init.o
		echo gmake "DEFS=-DTEST_START=$2_start -DTEST_END=$2_end"
		gmake "DEFS=-DTEST_START=$2_start -DTEST_END=$2_end"
		ulimit -t 10
		(echo c; echo quit) | bochs-nogui 'parport1: enable=1, file="bochs.out"'
	) >/dev/null 2>/dev/null
	if grep "oesp 0xefbfffdc" bochs.out >/dev/null \
	&& grep "trap $3"'$' bochs.out >/dev/null \
	&& grep "err  $4"'$' bochs.out >/dev/null \
	&& grep "eip  $5"'$' bochs.out >/dev/null
	then
		score=`echo 5+$score | bc`
		echo OK
	else
		echo WRONG
	fi
}

preempt() {
	# Check that alice and bob are running together
	perl -e "print 'Scheduling: '"
	(sed -e "s/^env_pop_tf/x&/" <kern/env.c; 
	echo '
	
	void
	env_pop_tf(struct Trapframe *tf)
	{
		printf("%d\n", (struct Env*)tf - envs);
		asm volatile("movl %0,%%esp\n"
			"\tpopal\n"
			"\tpopl %%es\n"
			"\tpopl %%ds\n"
			"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
			"\tiret"
			:: "g" (tf) : "memory");
		panic("iret failed");  /* mostly to placate the compiler */
	}
	
	') >kern/env-test.c
	(	
		rm kern/kernel
		gmake 'DEFS=-DTEST_ALICEBOB' 'ENV=env-test'
		ulimit -t 10
		(echo c; echo quit) | bochs-nogui 'parport1: enable=1, file="bochs.out"'
	) >/dev/null 2>/dev/null

	x=`grep '^[01]$' bochs.out`
	x=`echo $x | tr -d ' '`
	case $x in
	*01010101010101010101*)
		score=`echo 15+$score | bc`
		echo OK
		;;
	*)
		echo WRONG
	esac	
}

		
score=0

# Try all the different fault tests
gmake clean >/dev/null 2>/dev/null
fault 'Divide by zero' div0 0x0 0x0 0x800022
fault 'Breakpoint' brkpt 0x3 0x0 0x800021
fault 'Bad data segment' badds 0xd 0x8 0x800025
fault 'Read nonexistent page' pgfault_rd_nopage 0xe 0xfffc 0x800020
fault 'Read kernel-only page' pgfault_rd_noperms 0xe 0xfffd 0x800020
fault 'Write nonexistent page' pgfault_wr_nopage 0xe 0xfffe 0x800020
fault 'Write kernel-only page' pgfault_wr_noperms 0xe 0xffff 0x800020

gmake clean >/dev/null 2>/dev/null
preempt

gmake clean >/dev/null 2>/dev/null

echo "Score: $score/50"


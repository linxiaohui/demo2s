/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/printf.h>

struct Env *envs = NULL;		/* All environments */
struct Env *curenv = NULL;	        /* the current env */

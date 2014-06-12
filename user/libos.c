// Called from entry.S to get us going.
// Entry.S took care of defining envs, pages, vpd, and vpt.

#include "lib.h"

extern void umain(int, char**);

struct Env *env;

void
libmain(int argc, char **argv)
{
	// set env to point at our env structure in envs[].
	env = 0;	// Your code here.

	// call user main routine
	umain(argc, argv);

	// exit gracefully
	sys_env_destroy();
}


#include "lib.h"

char buf[8192];

void
cathelp(int f, char *s)
{
	long n;
	int r;

	while((n=read(f, buf, (long)sizeof buf))>0)
		if((r=write(1, buf, n))!=n)
			panic("write error copying %s: %e", s, r);
	if(n < 0)
		panic("error reading %s: %e", s, n);
}

void umain(int args,char **argv)
{
	int n,i,r;
	
	argv0="help";
	n=0;
	
	r=open("help.msg",O_RDONLY);
	if(r<0)
		panic("can not open help.msg: %e",r);
	else {
		cathelp(r,"help.msg");
		close(r);
	}
}

		
		

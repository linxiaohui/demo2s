#include "lib.h"

char * msg ="usage: rm filenames....\n";

void
umain(int argc,char **argv)
{
	int i,r;

	if(argc==1) {
		write(1,msg,strlen(msg));
	}
	else {
		for(i=1;i<argc;i++)
			if((r=remove(argv[i]))<0) {
				printf("%C",F_RED);
				printf("removing %s error: %e\n",argv[i],r);
				printf("%C",F_DEFAULT);
			}
	}
	
}

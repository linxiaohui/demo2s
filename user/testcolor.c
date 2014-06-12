#include "lib.h"

void umain(int argc,char **argv)
{
	int b=0x0000;
	int f=0x0000;

	for(;b<=0x7000;b+=0x1000) {
		//f=0x0000;
		for(;f<=0x0f00;f+=0x0100) {
			printf("%C",b|f);
			printf("%s: %04x\n","color code for this is",b|f);
		}
	}
	
}

	

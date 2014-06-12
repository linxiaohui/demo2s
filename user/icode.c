#include "lib.h"
#include "inc/aout.h"

void
umain(void)
{
	int fd, n, r;
	char buf[512+1];

	struct Aout a;
	printf("icode: open /motd\n");
	if ((fd = open("/motd", O_RDONLY)) < 0)
		panic("icode: open /motd: %e", fd);

	printf("icode: read /motd\n");
	while ((n = read(fd, buf, sizeof buf-1)) > 0){
		buf[n] = 0;
		sys_cputs(buf);
	}

	printf("icode: close /motd\n");
	close(fd);

	printf("icode: spawn /init\n");
	if((fd=open("/init",O_RDONLY))<0)
		panic("icode: open /init: %e",fd);
	if((n=read(fd,&a,sizeof(struct Aout)))<0)
		panic("icode read: %e",n);
	printf("%08x\t%08x\t%08x\n",a.a_text,a.a_data,a.a_entry);
	if ((r = spawnl("/init", "init", "initarg1", "initarg2", (char*)0)) < 0)
		panic("icode: spawn /init: %e", r);

	printf("icode: exiting\n");
}

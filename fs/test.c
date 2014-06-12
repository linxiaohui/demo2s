#include "fs.h"
#include <inc/x86.h>

int
strecmp(char *a, char *b)
{
	while (*b)
		if(*a++ != *b++)
			return 1;
	return 0;
}

static char *msg = "This is the NEW message of the day!\n\n";

void
fs_test(void)
{
	struct File *f;
	int r;
	void *blk;
	u_int *bits;

	// back up bitmap
	if ((r=sys_mem_alloc(0, BY2PG, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_mem_alloc: %e", r);
	bits = (u_int*)BY2PG;
	bcopy(bitmap, bits, BY2PG);
	// allocate block
	if ((r = alloc_block()) < 0)
		panic("alloc_block: %e", r);
	// check that block was free
	assert(bits[r/32]&(1<<(r%32)));
	// and is not free any more
	assert(!(bitmap[r/32]&(1<<(r%32))));

#ifdef DEBUG	
	printf("alloc_block is good\n");
#endif
	
	if ((r = file_open("/not-found", &f)) < 0 && r != -E_NOT_FOUND)
		panic("file_open /not-found: %e", r);
	else if (r == 0)
		panic("file_open /not-found succeeded!");
	if ((r = file_open("/newmotd", &f)) < 0)
		panic("file_open /newmotd: %e", r);
	
#ifdef DEBUG	
	printf("file_open is good\n");
#endif
	
	if ((r = file_get_block(f, 0, &blk)) < 0)
		panic("file_get_block: %e", r);
	if(strecmp(blk, msg) != 0)
		panic("file_get_block returned wrong data");

#ifdef DEBUG	
	printf("file_get_block is good\n");
#endif
	
	*(volatile char*)blk = *(volatile char*)blk;
	assert((vpt[VPN(blk)]&PTE_D));
	file_flush(f);
	assert(!(vpt[VPN(blk)]&PTE_D));
	
#ifdef DEBUG	
	printf("file_flush is good\n");
#endif
	
	if ((r = file_set_size(f, 0)) < 0)
		panic("file_set_size: %e", r);

#ifdef DEBUG	
	printf("file_set_size\n");
#endif
	
	assert(f->f_direct[0] == 0);
	assert(!(vpt[VPN(f)]&PTE_D));

#ifdef DEBUG	
	printf("file_truncate is good\n");
#endif
	
	if ((r = file_set_size(f, strlen(msg))) < 0)
		panic("file_set_size 2: %e", r);
	assert(!(vpt[VPN(f)]&PTE_D));
	if ((r = file_get_block(f, 0, &blk)) < 0)
		panic("file_get_block 2: %e", r);
	strcpy((char*)blk, msg);	
	assert((vpt[VPN(blk)]&PTE_D));
	file_flush(f);
	assert(!(vpt[VPN(blk)]&PTE_D));
	file_close(f);
	assert(!(vpt[VPN(f)]&PTE_D));

#ifdef DEBUG	
	printf("file rewrite is good\n");
#endif	
}

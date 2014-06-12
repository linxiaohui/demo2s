/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/printf.h>
#include <kern/kclock.h>

u_long boot_cr3; /* Physical address of boot time pg dir */
Pde* boot_pgdir;
struct Page *pages;

/* These variables are set by i386_detect_memory() */
u_long maxpa;            /* Maximum physical address */
u_long npage;            /* Amount of memory(in pages) */
u_long basemem;          /* Amount of base memory(in bytes) */
u_long extmem;           /* Amount of extended memory(in bytes) */

static u_long freemem;    /* Pointer to next byte of free mem */
static struct Page_list page_free_list;	/* Free list of physical pages */

// Global descriptor table.
//
// The kernel and user segments are identical(except for the DPL).
// To load the SS register, the CPL must equal the DPL.  Thus,
// we must duplicate the segments for the user and the kernel.
//
struct Segdesc gdt[] =
{
	// 0x0 - unused(always faults--for trapping NULL far pointers)
	SEG_NULL,

	// 0x8 - kernel code segment
	[GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),

	// 0x10 - kernel data segment
	[GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),

	// 0x18 - user code segment
	[GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),

	// 0x20 - user data segment
	[GD_UD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 3),

	// 0x40 - tss, initialized in idt_init()
	[GD_TSS >> 3] =  SEG_NULL
};

struct Pseudodesc gdt_pd =
{
	0, sizeof(gdt) - 1, (unsigned long) gdt,
};

static int
nvram_read(int r)
{
	return mc146818_read(NULL, r) | (mc146818_read(NULL, r+1)<<8);
}

void
i386_detect_memory(void)
{
	// CMOS tells us how many kilobytes there are
	basemem = ROUNDDOWN(nvram_read(NVRAM_BASELO)*1024, BY2PG);
	extmem = ROUNDDOWN(nvram_read(NVRAM_EXTLO)*1024, BY2PG);

	// Calculate the maxmium physical address based on whether
	// or not there is any extended memory.  See comment in ../inc/mmu.h.
	if (extmem)
		maxpa = EXTPHYSMEM + extmem;
	else
		maxpa = basemem;

	npage = maxpa / BY2PG;

	printf("Physical memory: %dK available, ", (int)(maxpa/1024));
	printf("base = %dK, extended = %dK\n", (int)(basemem/1024), (int)(extmem/1024));
}

// --------------------------------------------------------------
// Set up initial memory mappings and turn on MMU.
// --------------------------------------------------------------

static void check_boot_pgdir(void);

//
// Allocate n bytes of physical memory aligned on an 
// align-byte boundary.  Align must be a power of two.
// Return kernel virtual address.  If clear != 0, zero
// the memory.
//
// If we're out of memory, alloc should panic.
// It's too early to run out of memory.
// 
static void *
alloc(u_int n, u_int align, int clear)
{
	extern char end[];// kern/kernel.lds中对文件进行了布局,end就是kernel部分的结尾，所以把freemem初始时指向它 
	void *v;

	// initialize freemem if this is the first time
	if (freemem == 0)
		freemem = (u_long)end;

	// demo2s_code_begin;
	//	Step 1: round freemem up to be aligned properly
	/*freemem+=(align-freemem%align)%align;*/
	freemem=ROUND(freemem,align);
	if((freemem+n>KERNBASE+maxpa)||freemem+n<freemem) 
		panic("out of memory!");
	
	//	Step 2: save current value of freemem as allocated chunk
	v=(void*)freemem;
	//	Step 3: increase freemem to record allocation
	freemem+=n;
	//	Step 4: clear allocated chunk if necessary
	if(clear!=0) {
		bzero(v,n);
	}
	//	Step 5: return allocated chunk
	return v;
	// demo2s_code_end;
}

//
// Given pgdir, the current page directory base,
// walk the 2-level page table structure to find
// the Pte for virtual address va.  Return a pointer to it.
//
// If there is no such directory page:
//	- if create == 0, return 0.
//	- otherwise allocate a new directory page and install it.
//
// This function is abstracting away the 2-level nature of
// the page directory for us by allocating new page tables
// as needed.
// 
// Boot_pgdir_walk cannot fail.  It's too early to fail.
// 
static Pte*
boot_pgdir_walk(Pde *pgdir, u_long va, int create)
{
// demo2s_code_begin;
	Pte * pte;
	/*PTE_P is defined in inc/mmu.h*/
	if(!(pgdir[PDX(va)]&PTE_P)) { /* not present*/
		if(create==0)
			return 0;
		/*allocate a new page table */
		pte=(Pte*)alloc(BY2PG,BY2PG,1);
		pgdir[PDX(va)]=PADDR(pte)|PTE_W|PTE_U|PTE_P;
	}
	return (Pte*)KADDR(PTE_ADDR(pgdir[PDX(va)]))+PTX(va);
// demo2s_code_end;
}

//
// Map [va, va+size) of virtual address space to physical [pa, pa+size)
// in the page table rooted at pgdir.  Size is a multiple of BY2PG.
// Use permission bits perm|PTE_P for the entries.
//
static void
boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm)
{
    // demo2s_code_begin;
	int i;
	for(i=0;i<size;i+=BY2PG) {
			*((u_long*)boot_pgdir_walk(pgdir,va+i,1))=PTE_ADDR(pa+i)|perm|PTE_P;/*!!!!!*/
	}
	// demo2s_code_end;
}

// Set up a two-level page table:
//    boot_pgdir is its virtual address of the root
//    boot_cr3 is the physical adresss of the root
// Then turn on paging.  Then effectively turn off segmentation.
// (i.e., the segment base addrs are set to zero).
// 
// This function only sets up the kernel part of the address space
// (ie. addresses >= UTOP).  The user part of the address space
// will be setup later.
//
// From UTOP to ULIM, the user is allowed to read but not write.
// Above ULIM the user cannot read (or write). 
void
i386_vm_init(void)
{
	Pde *pgdir;
	u_int cr0, n;
/*
	panic("i386_vm_init: This function is not finished\n");
*/
	//////////////////////////////////////////////////////////////////////
	// create initial page directory.
	pgdir = alloc(BY2PG, BY2PG, 1);
	boot_pgdir = pgdir;
	boot_cr3 = PADDR(pgdir);

	//////////////////////////////////////////////////////////////////////
	// Recursively insert PD in itself as a page table, to form
	// a virtual page table at virtual address VPT.
	// (For now, you don't have understand the greater purpose of the
	// following two lines.)

	// Permissions: kernel RW, user NONE
	pgdir[PDX(VPT)] = PADDR(pgdir)|PTE_W|PTE_P;

	// same for UVPT
	// Permissions: kernel R, user R 
	pgdir[PDX(UVPT)] = PADDR(pgdir)|PTE_U|PTE_P;

	//////////////////////////////////////////////////////////////////////
	// Map the kernel stack (symbol name "bootstack"):
	//   [KSTACKTOP-PDMAP, KSTACKTOP)  -- the complete VA range of the stack
	//     * [KSTACKTOP-KSTKSIZE, KSTACKTOP) -- backed by physical memory
	//     * [KSTACKTOP-PDMAP, KSTACKTOP-KSTKSIZE) -- not backed => faults
	//   Permissions: kernel RW, user NONE
	// demo2s_code_begin;
	boot_map_segment(pgdir,KSTACKTOP-KSTKSIZE,KSTKSIZE,PADDR(bootstack),PTE_W);

	int i;
	for(i=PDX(KSTACKTOP-PDMAP);i<PDX(KSTACKTOP-KSTKSIZE);i++)
		pgdir[i]=0;
	// demo2s_code_end;

	//////////////////////////////////////////////////////////////////////
	// Map all of physical memory at KERNBASE. 
	// Ie.  the VA range [KERNBASE, 2^32 - 1] should map to
	//      the PA range [0, 2^32 - 1 - KERNBASE]   
	// We might not have that many(ie. 2^32 - 1 - KERNBASE)    
	// bytes of physical memory.  But we just set up the mapping anyway.
	// Permissions: kernel RW, user NONE
	// demo2s_code_begin;
	boot_map_segment(pgdir,KERNBASE,-KERNBASE,0,PTE_W);
	// demo2s_code_end;
	//////////////////////////////////////////////////////////////////////
	// Make 'pages' point to an array of size 'npage' of 'struct Page'.   
	// Map this array read-only by the user at UPAGES (ie. perm = PTE_U | PTE_P)
	// Permissions:
	//    - pages -- kernel RW, user NONE
	//    - the image mapped at UPAGES  -- kernel R, user R
	// demo2s_code_begin;
	n=npage*sizeof(struct Page);
	pages=(struct Page*)alloc(n,BY2PG,1);
	boot_map_segment(pgdir,UPAGES,n,PADDR(pages),PTE_U);
    //boot_map_segment(pgdir,pages,n,PADDR(pages),PTE_W|PTE_U);
	// demo2s_code_end;
	//////////////////////////////////////////////////////////////////////
	// Make '__envs' point to an array of size 'NENV' of 'struct Env'.
	// Map this array read-only by the user at UENVS (ie. perm = PTE_U | PTE_P)
	// Permissions:
	//    - __envs -- kernel RW, user NONE
	//    - the image mapped at UENVS  -- kernel R, user R
	// demo2s_code_begin;
	n=NENV*sizeof(struct Env);
	envs=(struct Env*)alloc(n,BY2PG,1);
	boot_map_segment(pgdir,UENVS,n,PADDR(envs),PTE_U);
    //boot_map_segment(pgdir,envs,n,PADDR(envs),PTE_W|PTE_U);
	// demo2s_code_end;
	check_boot_pgdir();

/* Entries in the page directory 	
	for(n=0;n<PDE2PD;n++) { 
 		printf("%d\t%p\n",n,pgdir[n]); 
 	} 
  */  
	//////////////////////////////////////////////////////////////////////
	// On x86, segmentation maps a VA to a LA (linear addr) and
	// paging maps the LA to a PA.  I.e. VA => LA => PA.  If paging is
	// turned off the LA is used as the PA.  Note: there is no way to
	// turn off segmentation.  The closest thing is to set the base
	// address to 0, so the VA => LA mapping is the identity.

	// Current mapping: VA KERNBASE+x => PA x.
	//     (segmentation base=-KERNBASE and paging is off)

	// From here on down we must maintain this VA KERNBASE + x => PA x
	// mapping, even though we are turning on paging and reconfiguring
	// segmentation.

	// Map VA 0:4MB same as VA KERNBASE, i.e. to PA 0:4MB.
	// (Limits our kernel to <4MB)
	
	/* demo2s
		map the first entry of the page directory to the page table of the first 4 MB of RAM
		因为在前面已经将物理地址[0,2^32-1-KERNBASE]映射到虚拟地址[KERNBASE,2^32-1]
		在打开页式寻址模式(lcr0)后,重新装载段寄存器前还能继续正确寻址
	*/
	pgdir[0] = pgdir[PDX(KERNBASE)];
	// This mapping was only used after paging was turned on but
	// before the segment registers were reloaded.
	
	// Install page table.
	lcr3(boot_cr3);

	// Turn on paging.
	cr0 = rcr0();
	cr0 |= CR0_PE|CR0_PG|CR0_AM|CR0_WP|CR0_NE|CR0_TS|CR0_EM|CR0_MP;
	cr0 &= ~(CR0_TS|CR0_EM);
	lcr0(cr0);

	// Current mapping: KERNBASE+x => x => x.
	// (x < 4MB so uses paging pgdir[0])

	// Reload all segment registers.
	asm volatile("lgdt gdt_pd+2");// turn off segment addressing!!
	asm volatile("movw %%ax,%%gs" :: "a" (GD_UD|3));
	asm volatile("movw %%ax,%%fs" :: "a" (GD_UD|3));
	asm volatile("movw %%ax,%%es" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ds" :: "a" (GD_KD));
	asm volatile("movw %%ax,%%ss" :: "a" (GD_KD));
	asm volatile("ljmp %0,$1f\n 1:\n" :: "i" (GD_KT));  // reload cs
	/* asm volatile("lldt %0" :: "m" (0)); */ /* gcc 3.3 OK. 4.x Compile Error*/
	asm volatile("lldt %%ax"::"a"(0));

	// Final mapping: KERNBASE+x => KERNBASE+x => x.

	// This mapping was only used after paging was turned on but
	// before the segment registers were reloaded.
	pgdir[0] = 0;/*ATTIENTION!! PAGING ADDRESSING*/

	// Flush the TLB for good measure, to kill the pgdir[0] mapping.
	lcr3(boot_cr3);
	printf("i386_vm_init() finished!\n");
}

//
// Checks that the kernel part of virtual address space
// has been setup roughly correctly(by i386_vm_init()).
//
// This function doesn't test every corner case,
// in fact it doesn't test the permission bits at all,
// but it is a pretty good sanity check. 
//
static u_long va2pa(Pde *pgdir, u_long va);

static void
check_boot_pgdir(void)
{
	u_long i, n;
	Pde *pgdir;

	pgdir = boot_pgdir;

	// check pages array
	n = ROUND(npage*sizeof(struct Page), BY2PG);
	for(i=0; i<n; i+=BY2PG)
		assert(va2pa(pgdir, UPAGES+i) == PADDR(pages)+i);
	
    //	printf("pages OK!\n");
	
	// check envs array
	n = ROUND(NENV*sizeof(struct Env), BY2PG);
	for(i=0; i<n; i+=BY2PG)
		assert(va2pa(pgdir, UENVS+i) == PADDR(envs)+i);

	//printf("envs OK!\n");
	
	// check phys mem
	for(i=0; KERNBASE+i != 0; i+=BY2PG)
		assert(va2pa(pgdir, KERNBASE+i) == i);
	
    //	printf("phys mem OK!\n");

	// check kernel stack
	for(i=0; i<KSTKSIZE; i+=BY2PG)
		assert(va2pa(pgdir, KSTACKTOP-KSTKSIZE+i) == PADDR(bootstack)+i);

	//printf("kernel ktack OK!\n");
	
	// check for zero/non-zero in PDEs
	for (i = 0; i < PDE2PD; i++) {
		switch (i) {
		case PDX(VPT):
		case PDX(UVPT):
		case PDX(KSTACKTOP-1):
		case PDX(UPAGES):
		case PDX(UENVS):
			assert(pgdir[i]);
			break;
		default:
			if(i >= PDX(KERNBASE))
				assert(pgdir[i]);
			else
				assert(pgdir[i]==0);
			break;
		}
	}
	//printf("zero/non-zero in PDEs OK!\n");
	
	printf("check_boot_pgdir() succeeded!\n");
}

static u_long
va2pa(Pde *pgdir, u_long va)
{
	Pte *p;

	pgdir = &pgdir[PDX(va)];
	if (!(*pgdir&PTE_P))
		return ~0;
	p = (Pte*)KADDR(PTE_ADDR(*pgdir));
	if (!(p[PTX(va)]&PTE_P))
		return ~0;
	return PTE_ADDR(p[PTX(va)]);
}
		
// --------------------------------------------------------------
// Tracking of physical pages.
// --------------------------------------------------------------

static void page_initpp(struct Page *pp);

//  
// Initialize page structure and memory free list.
//
void
page_init(void)
{
	// The example code here marks all pages as free.
	// However this is not truly the case.  What memory is free?

	int i,n;
	LIST_INIT (&page_free_list);
	// demo2s_code_begin;
	//  1) Mark page 0 as in use
	pages[0].pp_ref=1;
	//LIST_INSERT_HEAD(&page_free_list, &pages[0], pp_link);
	
	//  2) Mark the rest of base memory as free.
	n=basemem/BY2PG;
	for (i = 1; i < n; i++) {
		pages[i].pp_ref = 0;
		LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
	}	
	//  3)  the IO hole [IOPHYSMEM, EXTPHYSMEM) :mark it as in use
	n=EXTPHYSMEM/BY2PG;
	for (; i < n; i++) {
		pages[i].pp_ref = 1;
	//	LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
	}
	//  4) Then extended memory(ie. >= EXTPHYSMEM):
	//     ==> some of it's in use some is free. Where is the kernel?
	//     Which pages are used for page tables and other data structures?    
	//
	// Change the code to reflect this.
	n=ROUND(freemem,BY2PG)/BY2PG;
	for (; i < n; i++) {
		pages[i].pp_ref = 1;
	}

	for(;i<npage;i++) {
		pages[i].pp_ref=0;
		LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
	}			
	// demo2s_code_end;
	
	/* ORIGINAL CODE	
    int i;
    LIST_INIT (&page_free_list);
    for (i = 0; i < npage; i++) {
            pages[i].pp_ref = 0;
            LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
    }
	*/
}

//
// Initialize a Page structure.
//
static void
page_initpp(struct Page *pp)
{
	bzero(pp, sizeof(*pp));
}

//
// Allocates a physical page.
//
// *pp -- is set to point to the Page struct of the newly allocated
// page
//
// RETURNS 
//   0 -- on success
//   -E_NO_MEM -- otherwise 
//
// Hint: use LIST_FIRST, LIST_REMOVE, page_initpp()
// Hint: pp_ref should not be incremented 
int
page_alloc(struct Page **pp)
{
	// demo2s_code_start;
	if((*pp=LIST_FIRST(&page_free_list))==NULL) 
		return -E_NO_MEM;
	LIST_REMOVE(*pp,pp_link);
	page_initpp(*pp);
	return 0;
	// demo2s_code_end;
}

//
// Return a page to the free list.
// (This function should only be called when pp->pp_ref reaches 0.)
//
void
page_free(struct Page *pp)
{
	// demo2s_code_start;
	page_initpp(pp);
	LIST_INSERT_HEAD(&page_free_list,pp,pp_link);
	// demo2s_code_end;
}

//
// Decrement the reference count on a page, freeing it if there are no more refs.
//
void
page_decref(struct Page *pp)
{
	if (--pp->pp_ref == 0)
		page_free(pp);
}
//
// This is boot_pgdir_walk with a different allocate function.
// Unlike boot_pgdir_walk, pgdir_walk can fail, so we have to
// return pte via a pointer parameter.
//
// Stores address of page table entry in *pte.
// Stores 0 if there is no such entry or on error.
// 
// RETURNS: 
//   0 on success
//   -E_NO_MEM, if page table couldn't be allocated
//
int
pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte)
{
	// demo2s_code_start;
	struct Page * new_page;
	if(!(pgdir[PDX(va)]&PTE_P)) { //??
		if(create==0) {
			*ppte=0;
			return 0; //??
		}
		if(page_alloc(&new_page)<0) {
			*ppte=0;
			return -E_NO_MEM;
		}
		new_page->pp_ref++;
		pgdir[PDX(va)]=page2pa(new_page)|PTE_W|PTE_U|PTE_P;
	}		
	*ppte=(Pte*)KADDR(PTE_ADDR(pgdir[PDX(va)]))+PTX(va);
	//*ppte=(Pte*)PTE_ADDR(pgdir[PDX(va)])+PTX(va);
	return 0;
	// demo2s_code_end;
}

//
// Map the physical page 'pp' at virtual address 'va'.
// The permissions (the low 12 bits) of the page table
//  entry should be set to 'perm|PTE_P'.
//
// Details
//   - If there is already a page mapped at 'va', it is page_remove()d.
//   - If necesary, on demand, allocates a page table and inserts it into 'pgdir'.
//   - pp->pp_ref should be incremented if the insertion succeeds
//
// RETURNS: 
//   0 on success
//   -E_NO_MEM, if page table couldn't be allocated
//
// Hint: The TA solution is implemented using
//   pgdir_walk() and and page_remove().
//
int
page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm) 
{
	// Fill this function in
	// demo2s_code_s
#ifdef __LAB__3__
	Pte * pte;
	int status;
	//if((pgdir[PDX(va)])&PTE_P) {
	//	page_remove(pgdir,va);
	//}
	page_remove(pgdir,va);
	status=pgdir_walk(pgdir,va,1,&pte);
	if(status<0)
		return -E_NO_MEM;
	 		
	*pte=page2pa(pp)|perm|PTE_P;

	pp->pp_ref++;
	return 0;
#else
	Pte * 	pte;
	int 	status;
	pp->pp_ref++;
	
	status = pgdir_walk(pgdir,va,1,&pte);
	
	if(status<0) {
		pp->pp_ref--;
		return -E_NO_MEM;
	}
	
	page_remove(pgdir,va);
	
	*pte=page2pa(pp)|perm|PTE_P;
	
	return 0;

#endif
	// demo2s_code_end;
}

//
// Return the page mapped at virtual address 'va'.
// If ppte is not zero, then we store in it the address
// of the pte for this page.  This is used by page_remove
// but should not be used by other callers.
//
// Return 0 if there is no page mapped at va.
//
// Hint: the TA solution uses pgdir_walk and pa2page.
//
struct Page*
page_lookup(Pde *pgdir, u_long va, Pte **ppte)
{
}
//
// Unmaps the physical page at virtual address 'va'.
//
// Details:
//   - The ref count on the physical page should decrement.
//   - The physical page should be freed if the refcount reaches 0.
//   - The pg table entry corresponding to 'va' should be set to 0.
//     (if such a PTE exists)
//   - The TLB must be invalidated if you remove an entry from
//	   the pg dir/pg table.
//
// Hint: The TA solution is implemented using page_lookup,
// 	tlb_invalidate, and page_decref.
//
// Hint: The TA solution is implemented using
//  pgdir_walk(), page_free(), tlb_invalidate()
//
void
page_remove(Pde *pgdir, u_long va) 
{
	// Fill this function in
	// demo2s_code_begin;
	Pte * pte;
	struct Page * page;
	if(va2pa(pgdir,va)==~0) {
		//printf("VIRTUAL address %p seems has not been mapped\n",va);
		return;
	}
	
	//printf("VIRTUAL address %p seems has been mapped\n",va);
	
	pgdir_walk(pgdir,va,0,&pte);
	if(pte==0) {
		printf("virtual address %p seems has not been mapped\n",va);
		return;
	}
	
	page=pa2page(*pte);
	
	page->pp_ref--;
	
	if(page->pp_ref==0)
		page_free(page);
	bzero(pte,sizeof(*pte));
	tlb_invalidate(pgdir,va);
	// demo2s_code_end;
}

//
// Invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor.
//
void
tlb_invalidate(Pde *pgdir, u_long va)
{
	// Flush the entry only if we're modifying the current address space.
	if (!curenv || curenv->env_pgdir == pgdir)
		invlpg(va);
}

void
page_check(void)
{
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	LIST_INIT(&page_free_list);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);
	
    printf("page_alloc() seems worked!\n");

	// there is no free memory, so we can't allocate a page table 
	assert(page_insert(boot_pgdir, pp1, 0x0, 0) < 0);

	// free pp0 and try again: pp0 should be used for page table
	page_free(pp0);
	assert(page_insert(boot_pgdir, pp1, 0x0, 0) == 0);
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(pp1->pp_ref == 1);

	//demo2s
	assert(page_alloc(&pp) < 0);//assert(page_alloc(&pp0) < 0); ===>error !! 
	
	// should be able to map pp2 at BY2PG because pp0 is already allocated for page table
	assert(page_insert(boot_pgdir, pp2, BY2PG, 0) == 0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp2));
	assert(pp2->pp_ref == 1);
	
	//demo2s
	assert(page_alloc(&pp) < 0);//assert(page_alloc(&pp0) < 0); ===>error !! 

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// should be able to map pp2 at BY2PG because it's already there
	assert(page_insert(boot_pgdir, pp2, BY2PG, 0) == 0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// pp2 should NOT be on the free list
	// could happen in ref counts are handled sloppily in page_insert
	assert(page_alloc(&pp) == -E_NO_MEM);
	
	// should not be able to map at PDMAP because need free page for page table
	assert(page_insert(boot_pgdir, pp0, PDMAP, 0) < 0);

	// insert pp1 at BY2PG (replacing pp2)
	assert(page_insert(boot_pgdir, pp1, BY2PG, 0) == 0);

	// should have pp1 at both 0 and BY2PG, pp2 nowhere, ...
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->pp_ref == 2);
	assert(pp2->pp_ref == 0);

	// pp2 should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at BY2PG
	page_remove(boot_pgdir, 0x0);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	// unmapping pp1 at BY2PG should free it
	page_remove(boot_pgdir, BY2PG);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, BY2PG) == ~0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// so it should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// forcibly take pp0 back
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	boot_pgdir[0] = 0;
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);

	printf("page_check() succeeded!\n");
}


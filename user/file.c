#include "lib.h"
#include <inc/fs.h>

// maximum number of open files
#define MAXFD	32
#define FILEBASE 0xd0000000

// open file structure on client side
struct Fileinfo
{
	u_int fi_va;		// virtual address of file contents
	u_int fi_busy;		// is the fd in use?
	u_int fi_size;		// how many bytes in the fd?
	u_int fi_omode;		// file open mode (O_RDONLY etc.)
	u_int fi_fileid;	// file id on server
	u_int fi_offset;	// r/w offset
};

// local information about open files
extern struct Fileinfo fdtab[MAXFD];

// Find an available struct Fileinfo in filetab
static int
fd_alloc(struct Fileinfo **fi)
{
	int i;

	// entry.S only gives us a page!
	assert(sizeof(fdtab) < BY2PG);

	for (i=0; i<MAXFD; i++)
		if (!fdtab[i].fi_busy) {
			if (fi)
				*fi = &fdtab[i];
			fdtab[i].fi_busy = 1;
			fdtab[i].fi_va = FILEBASE+PDMAP*i;
			
			return i;
		}
	return -E_MAX_OPEN;
}

// Lookup a Fileinfo struct given a file descriptor
static int
fd_lookup(int fd, struct Fileinfo **fi)
{
	if (fd < 0 || fd >= MAXFD || !fdtab[fd].fi_busy)
		return -E_INVAL;
	*fi = &fdtab[fd];
	return 0;
}

// Open a file (or directory),
// returning the file descriptor on success, < 0 on failure.
int
open(const char *path, int mode)
{
	//demo2s_code_start;
	int 	fd;
	int 	r;
	u_int 	fbno;
	u_int 	i;
	struct Fileinfo * 	finfo;
	//find an unused file descriptor
	if((fd=fd_alloc(&finfo))<0)
		return fd;
	//make an IPC request to file server to open a file
	if((r=fsipc_open(path,mode,&(finfo->fi_fileid),&(finfo->fi_size)))<0)
		return r;
	finfo->fi_omode	 = mode;
	finfo->fi_offset = 0;
	//map all the pages 
	fbno = (finfo->fi_size+BY2BLK-1)/BY2BLK;
	for(i=0;i<fbno;i++)
		if((r=fsipc_map(finfo->fi_fileid,i,finfo->fi_va+i*BY2BLK))<0)
			// those already mapped?
			return r;
	return fd;
	//demo2s_code_end;
}

// Close a file descriptor
int
close(int fd)
{
	//demo2s_code_start;
	int 	r;
	u_int 	fbno;
	u_int 	i;
	struct Fileinfo * 	fi;
	if((r=fd_lookup(fd,&fi))<0)
		return r;
	//notify the file server pages that has been modified
	fbno = (fi->fi_size+BY2BLK-1)/BY2BLK;
	for(i=0;i<fbno;i++)
		if(vpt[fi->fi_va/BY2PG+i]&PTE_D)
			fsipc_dirty(fi->fi_fileid,i);
	//make a request to close the file
	if((r=fsipc_close(fi->fi_fileid))<0)
		return r;
	//unmap all mapped pages in the reserved file-mapping region
	for (i =0; i <PDMAP; i+=BY2PG)
		if ((r = sys_mem_unmap(0, fi->fi_va+i)) < 0)
			panic("close: sys_mem_unmap %08x: %e",
				fi->fi_va+i, r);
	return 0;
	//demo2s_code_end;
}

// Read 'n' bytes from 'fd' at the current seek position into 'buf'.
// Since files are memory-mapped, this amounts to a bcopy()
// surrounded by a little red tape to handle the file size and seek pointer.
int
read(int fd, void *buf, u_int n)
{
	int r;
	struct Fileinfo *fi;

	if ((r = fd_lookup(fd, &fi)) < 0)
		return r;
	if ((fi->fi_omode & O_ACCMODE) == O_WRONLY)
		return -E_INVAL;

	// avoid reading past the end of file
	if (fi->fi_offset > fi->fi_size)
		return 0;
	if (fi->fi_offset+n > fi->fi_size)
		n = fi->fi_size - fi->fi_offset;

	// read the data by copying from the file mapping
	bcopy((char*)fi->fi_va + fi->fi_offset, buf, n);
	fi->fi_offset += n;
	return n;
}

// Find the virtual address of the page
// that maps the file block starting at 'offset'.
int
read_map(int fd, u_int offset, void **blk)
{
	int r;
	struct Fileinfo *fi;

	if ((r = fd_lookup(fd, &fi)) < 0)
		return r;
	if (offset > MAXFILESIZE)
		return -E_NO_DISK;
	*blk = (void*)(fi->fi_va + offset);
	return 0;
}

// Write 'n' bytes from 'buf' to 'fd' at the current seek position.
int
write(int fd, const void *buf, u_int n)
{
	int r;
	u_int tot;
	struct Fileinfo *fi;

	if ((r = fd_lookup(fd, &fi)) < 0)
		return r;

	// only if the file is writable...
	if ((fi->fi_omode & O_ACCMODE) == O_RDONLY)
		return -E_INVAL;

	// don't write past the maximum file size
	tot = fi->fi_offset + n;
	if (tot > MAXFILESIZE)
		return -E_NO_DISK;

	// increase the file's size if necessary
	if (tot > fi->fi_size) {
		if ((r = ftruncate(fd, tot)) < 0)
			return r;
	}

	// write the data
	bcopy(buf, (char*)fi->fi_va+fi->fi_offset, n);
	fi->fi_offset += n;
	return n;
}

// Seek to an absolute file position.
// (note: no 'whence' parameter as in POSIX seek.)
int
seek(int fd, u_int offset)
{
	int r;
	struct Fileinfo *fi;

	if ((r = fd_lookup(fd, &fi)) < 0)
		return r;

	fi->fi_offset = offset;
	return 0;
}

// Truncate or extend an open file to 'size' bytes
int
ftruncate(int fd, u_int size)
{
	int i, r;
	struct Fileinfo *fi;
	u_int oldsize;

	if (size > MAXFILESIZE)
		return -E_NO_DISK;

	if ((r = fd_lookup(fd, &fi)) < 0)
		return r;

	oldsize = fi->fi_size;
	if ((r = fsipc_set_size(fi->fi_fileid, size)) < 0)
		return r;

	// Map any new pages needed if extending the file
	for (i = ROUND(oldsize, BY2PG); i < ROUND(size, BY2PG); i += BY2PG) {
		if ((r = fsipc_map(fi->fi_fileid, i, fi->fi_va+i)) < 0) {
			fsipc_set_size(fi->fi_fileid, oldsize);
			return r;
		}
	}

	// Unmap pages if truncating the file
	for (i = ROUND(size, BY2PG); i < ROUND(oldsize, BY2PG); i+=BY2PG)
		if ((r = sys_mem_unmap(0, fi->fi_va+i)) < 0)
			panic("ftruncate: sys_mem_unmap %08x: %e",
				fi->fi_va+i, r);

	fi->fi_size = size;
	return 0;
}

// Delete a file
int
remove(const char *path)
{
	return fsipc_remove(path);
}

// Synchronize disk with buffer cache
int
sync(void)
{
	return fsipc_sync();
}


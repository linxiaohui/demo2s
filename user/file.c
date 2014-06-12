#include "lib.h"
#include <inc/fs.h>

// maximum number of open files
#define debug 0

static int file_close(struct Fd *fd);
static int file_read(struct Fd *fd, void *buf, u_int n, u_int offset);
static int file_write(struct Fd *fd, const void *buf, u_int n, u_int offset);
static int file_stat(struct Fd *fd, struct Stat *stat);

// open file structure on client side
struct Dev devfile =
{
.dev_id=	'f',
.dev_name=	"file",
.dev_read=	file_read,
.dev_write=	file_write,
.dev_close=	file_close,
.dev_stat=	file_stat,
};



// Open a file (or directory),
// returning the file descriptor on success, < 0 on failure.
int
open(const char *path, int mode)
{
	//demo2s_code_start;
	int 	r;
	u_int 	fbno;
	u_int 	i;
	struct Fd * fd;
	struct Filefd * ffd;

	//find an unused file descriptor
	if((r=fd_alloc(&fd))<0) {
		if(debug) printf("fd_alloc error\n");
		return r;
	}

	//make an IPC request to file server to open a file
	
	if((r=fsipc_open(path,mode,fd))<0) {
		if(debug) printf("fs_ipc_open error\n");
		return r;
	}
	

	ffd=(struct Filefd *)fd;

	
	//map all the pages 
	fbno = (ffd->f_file.f_size+BY2BLK-1)/BY2BLK;
	for(i=0;i<fbno;i++)
		if((r=fsipc_map(ffd->f_fileid,i,fd2data(fd)+i*BY2BLK))<0) {
			if(debug) printf("fs_ipc_map %d error\n",i);
			// those already mapped?
			return r;
		}
	
	return fd2num(fd);
	//demo2s_code_end;
}

// Close a file descriptor
int
file_close(struct Fd *fd)
{
	//demo2s_code_start;
	int 	r;
	u_int 	fbno;
	u_int 	i;
	int basepage;

	basepage=fd2data(fd)/BY2PG;

	struct Filefd *ffd;

	ffd=(struct Filefd*)fd;

	//notify the file server pages that has been modified
	fbno = (ffd->f_file.f_size+BY2BLK-1)/BY2BLK;
	for(i=0;i<fbno;i++)
		if(vpt[basepage+i]&PTE_D)
			fsipc_dirty(ffd->f_fileid,i);
	
	//make a request to close the file
	if((r=fsipc_close(ffd->f_fileid))<0)
		return r;
	
	//unmap all mapped pages in the reserved file-mapping region
	for (i =0; i <PDMAP; i+=BY2PG)
		if ((r = sys_mem_unmap(0, basepage*BY2PG+i)) < 0)
		panic("close: sys_mem_unmap %08x: %e",fd2data(fd)+i, r);
	return 0;
	//demo2s_code_end;
}

// Read 'n' bytes from 'fd' at the current seek position into 'buf'.
// Since files are memory-mapped, this amounts to a bcopy()
// surrounded by a little red tape to handle the file size and seek pointer.
static int
file_read(struct Fd *fd, void *buf, u_int n, u_int offset)
{
	u_int size;
	struct Filefd *f;

	f = (struct Filefd*)fd;

	// avoid reading past the end of file
	size = f->f_file.f_size;
	if (offset > size)
		return 0;
	if (offset+n > size)
		n = size - offset;

	// read the data by copying from the file mapping
	bcopy((char*)fd2data(fd)+offset, buf, n);
	return n;
}

// Find the virtual address of the page
// that maps the file block starting at 'offset'.
int
read_map(int fdnum, u_int offset, void **blk)
{
	int r;
	u_int va;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0)
		return r;
	if (fd->fd_dev_id != devfile.dev_id)
		return -E_INVAL;
	va = fd2data(fd) + offset;
	if (offset >= MAXFILESIZE)
		return -E_NO_DISK;
	if (!(vpd[PDX(va)]&PTE_P) || !(vpt[VPN(va)]&PTE_P))
		return -E_NO_DISK;
	*blk = (void*)va;
	return 0;
}

// Write 'n' bytes from 'buf' to 'fd' at the current seek position.
static int
file_write(struct Fd *fd, const void *buf, u_int n, u_int offset)
{
	int r;
	u_int tot;
	struct Filefd *f;

	f = (struct Filefd*)fd;

	// only if the file is writable...

	// don't write past the maximum file size
	tot = offset + n;
	if (tot > MAXFILESIZE)
		return -E_NO_DISK;

	// increase the file's size if necessary
	if (tot > f->f_file.f_size) {
		if ((r = ftruncate(fd2num(fd), tot)) < 0)
			return r;
	}

	// write the data
	bcopy(buf, (char*)fd2data(fd)+offset, n);
	return n;
}

// Seek to an absolute file position.
// (note: no 'whence' parameter as in POSIX seek.)
static int
file_stat(struct Fd *fd, struct Stat *st)
{
	struct Filefd *f;

	f = (struct Filefd*)fd;

	strcpy(st->st_name, f->f_file.f_name);
	st->st_size = f->f_file.f_size;
	st->st_isdir = f->f_file.f_type==FTYPE_DIR;
	return 0;
}

// Truncate or extend an open file to 'size' bytes
int
ftruncate(int fdnum, u_int size)
{
	int i, r;
	struct Fd *fd;
	struct Filefd *f;
	u_int oldsize, va, fileid;

	if (size > MAXFILESIZE)
		return -E_NO_DISK;

	if ((r = fd_lookup(fdnum, &fd)) < 0)
		return r;
	if (fd->fd_dev_id != devfile.dev_id)
		return -E_INVAL;

	f = (struct Filefd*)fd;
	fileid = f->f_fileid;
	oldsize = f->f_file.f_size;
	if ((r = fsipc_set_size(fileid, size)) < 0)
		return r;

	va = fd2data(fd);
	// Map any new pages needed if extending the file
	for (i = ROUND(oldsize, BY2PG); i < ROUND(size, BY2PG); i += BY2PG) {
		if ((r = fsipc_map(fileid, i, va+i)) < 0) {
			fsipc_set_size(fileid, oldsize);
			return r;
		}
	}

	// Unmap pages if truncating the file
	for (i = ROUND(size, BY2PG); i < ROUND(oldsize, BY2PG); i+=BY2PG)
		if ((r = sys_mem_unmap(0, va+i)) < 0)
			panic("ftruncate: sys_mem_unmap %08x: %e", va+i, r);

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


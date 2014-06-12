/*
 * File system server main loop -
 * serves IPC requests from other environments.
 */

#include "fs.h"
#include <inc/x86.h>

#define debug 0

struct Open {
	struct File *o_file;	// mapped descriptor for open file
	u_int o_fileid;		// file id
	int o_mode;		// open mode
	struct Filefd *o_ff;	// va of filefd page
};

// Max number of open files in the file system at once
#define MAXOPEN	1024
#define FILEVA 0xD0000000

// initialize to force into data section
struct Open opentab[MAXOPEN] = { { 0, 0, 1 } };

// Virtual address at which to receive page mappings containing client requests.
#define REQVA	0x0ffff000

void
serve_init(void)
{
	int i;
	u_int va;
	va = FILEVA;
	for (i=0; i<MAXOPEN; i++) {
		opentab[i].o_fileid = i;
		opentab[i].o_ff = (struct Filefd*)va;
		va += BY2PG;
	}
}
// Allocate an open file.
int
open_alloc(struct Open **o)
{
	int i, r;

	// Find an available open-file table entry
	for (i = 0; i < MAXOPEN; i++) {
		switch (pageref(opentab[i].o_ff)) {
		case 0:
			if ((r = sys_mem_alloc(0, (u_int)opentab[i].o_ff, PTE_P|PTE_U|PTE_W)) < 0)
				return r;
		case 1:
			opentab[i].o_fileid += MAXOPEN;
			*o = &opentab[i];
			bzero((void*)opentab[i].o_ff, BY2PG);
			return (*o)->o_fileid;
		}
	}
	return -E_MAX_OPEN;
}

// Look up an open file for envid.
int
open_lookup(u_int envid, u_int fileid, struct Open **po)
{
	struct Open *o;
	o = &opentab[fileid%MAXOPEN];
	if (pageref(o->o_ff) == 1 || o->o_fileid != fileid)
		return -E_INVAL;
	*po = o;
	return 0;
}

// Serve requests, sending responses back to envid.
// To send a result back, ipc_send(envid, r, 0, 0).
// To include a page, ipc_send(envid, r, srcva, perm).

void
serve_open(u_int envid, struct Fsreq_open *rq)
{
	if (debug) printf("serve_open %08x %s 0x%x\n", envid, rq->req_path, rq->req_omode);

	u_char path[MAXPATHLEN];
	struct File *f;
	struct Filefd *ff;
	int fileid;
	int r;
	u_int mode;
	struct Open *o;

	// Copy in the path, making sure it's null-terminated
	bcopy(rq->req_path, path, MAXPATHLEN);
	path[MAXPATHLEN-1] = 0;
	mode=rq->req_omode;

	// Find a file id.
	if ((r = open_alloc(&o)) < 0) {
		if (debug) printf("open_alloc failed: %e", r);
		goto out;
	}
	fileid = r;
//demo2s_code;
	// Open the file.
	if(!(mode&O_CREAT)) {
		if ((r=file_open(path, &f))<0) {
			if(!(mode&O_WRONLY)) {
				if (debug) printf("file_open failed: %e", r);
				goto out;
			}

			if ((r=file_create(path,&f))<0) {
				if(debug) printf("file_create failed: %e",r);
				goto out;
			}
		}
	}
	else {
		if ((r=file_create(path,&f))<0) {
			if(debug) printf("file_create failed: %e",r);
			goto out;
		}
	}
//demo2s_code_end;
	// Save the file pointer.
	o->o_file = f;
	ff = (struct Filefd*)o->o_ff;
	bcopy(f,&ff->f_file,sizeof(ff->f_file));
	ff->f_fileid = o->o_fileid;
	o->o_mode = rq->req_omode;
	ff->f_fd.fd_omode = o->o_mode;
	ff->f_fd.fd_dev_id = devfile.dev_id;

	if (debug) printf("sending success, page %08x\n", (u_int)o->o_ff);
	ipc_send(envid, 0, (u_int)o->o_ff, PTE_LIBRARY|PTE_P|PTE_U|PTE_W);
	return;

out:
	ipc_send(envid, r, 0, 0);
}

void
serve_map(u_int envid, struct Fsreq_map *rq)
{
	if (debug) printf("serve_map %08x %08x %08x\n", envid, rq->req_fileid, rq->req_offset);

//demo2s_code_start;
	int 	fileid;
	int 	r;
	u_int 	offset;
	u_int* 	dva;
	struct Open * 	o;
	dva=0;
	fileid = rq->req_fileid;
	offset = rq->req_offset;//the offset of the block

	if((r=open_lookup(envid,fileid,&o))<0) {
		printf("open_lookup error\n");
	}
	else {
		if(o->o_file==0)
			r = -E_INVAL;
		else
			r = file_get_block(o->o_file,offset,&dva);
	}
	//return the result
	ipc_send(envid,r,dva,PTE_LIBRARY|PTE_W|PTE_U|PTE_P);
//demo2s_code_end;
}

void
serve_set_size(u_int envid, struct Fsreq_set_size *rq)
{
	if (debug) printf("serve_set_size %08x %08x %08x\n", envid, rq->req_fileid, rq->req_size);

//demo2s_code_start;
	int 	fileid;
	int 	r;
	u_int 	size;
	struct Open * 	o;
	fileid = rq->req_fileid;
	size   = rq->req_size;

	if((r=open_lookup(envid,fileid,&o))<0)
		;
	else {
		if(o->o_file==0)
			r = -E_INVAL;
		else
			r = file_set_size(o->o_file,size);
	}
	//return the result
	ipc_send(envid,r,0,0);
	//demo2s_code_end;
}

void
serve_close(u_int envid, struct Fsreq_close *rq)
{
	if (debug) printf("serve_close %08x %08x\n", envid, rq->req_fileid);

//demo2s_code_start;
	int 		fileid;
	int 		r;
	struct Open * 	o;
	fileid = rq->req_fileid;

		r = 0;
	
	if((r=open_lookup(envid,fileid,&o))<0)
		;
	else {
		/*printf("is this the file?\n");
		printf("file name %s\n",o->o_file->f_name);
		printf("the open struct is %d\n",o-opentab);
		printf("ref is %d\n",pages[PPN(vpt[VPN(o->o_ff)])].pp_ref); //??
		*/
	}
	
	//return the result
	ipc_send(envid,r,0,0);
	//demo2s_code_end;
}

void
serve_remove(u_int envid, struct Fsreq_remove *rq)
{
	if (debug) printf("serve_map %08x %s\n", envid, rq->req_path);

//demo2s_code_start;
	u_char path[MAXPATHLEN];
	int r;
	// Copy in the path, making sure it's null-terminated
	bcopy(rq->req_path, path, MAXPATHLEN);
	path[MAXPATHLEN-1] = 0;
	r = file_remove(path);
	//return the result
	ipc_send(envid,r,0,0);
	
//demo2s_code_end;
}

void
serve_dirty(u_int envid, struct Fsreq_dirty *rq)
{
	if (debug) printf("serve_dirty %08x %08x %08x\n", envid, rq->req_fileid, rq->req_offset);

//demo2s_code_start;
	int 	fileid;
	int 	r;
	u_int 	offset;
	struct Open * 	o;
	fileid = rq->req_fileid;
	offset = rq->req_offset;//the offset of the block

	if((r=open_lookup(envid,fileid,&o))<0)
		;
	else {
		if(o->o_file==0)
			r = -E_INVAL;
		else
			r = file_dirty(o->o_file,offset);
	}
	ipc_send(envid,r,0,0);
//demo2s_code_end;
}

void
serve_sync(u_int envid)
{
	fs_sync();
	ipc_send(envid, 0, 0, 0);
}

void
serve(void)
{
	u_int req, whom, perm;

	for(;;) {
		perm = 0;
		req = ipc_recv(&whom, REQVA, &perm);
		if (debug)
			printf("fs req %d from %08x [page %08x: %s]\n",
				req, whom, vpt[VPN(REQVA)], REQVA);

		// All requests must contain an argument page
		if (!(perm & PTE_P)) {
			printf("Invalid request from %08x: no argument page\n",whom);
			continue; // just leave it hanging...
		}

		switch (req) {
		case FSREQ_OPEN:
			serve_open(whom, (struct Fsreq_open*)REQVA);
			break;
		case FSREQ_MAP:
			serve_map(whom, (struct Fsreq_map*)REQVA);
			break;
		case FSREQ_SET_SIZE:
			serve_set_size(whom, (struct Fsreq_set_size*)REQVA);
			break;
		case FSREQ_CLOSE:
			serve_close(whom, (struct Fsreq_close*)REQVA);
			break;
		case FSREQ_DIRTY:
			serve_dirty(whom, (struct Fsreq_dirty*)REQVA);
			break;
		case FSREQ_REMOVE:
			serve_remove(whom, (struct Fsreq_remove*)REQVA);
			break;
		case FSREQ_SYNC:
			serve_sync(whom);
			break;
		default:
			printf("Invalid request code %d from %08x\n", whom, req);
			break;
		}
		sys_mem_unmap(0, REQVA);
	}
}

void
umain(void)
{
	assert(sizeof(struct File)==256);
	printf("FS is running\n");

	// Check that we are able to do I/O
	outw(0x8A00, 0x8A00);
	printf("FS can do I/O\n");

	serve_init();
	fs_init();

	fs_test();

	printf("File System server test OK!\n\n");
	serve();
}


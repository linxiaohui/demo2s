#include "lib.h"

int debug = 0;

//
// get the next token from string s
// set *p1 to the beginning of the token and
// *p2 just past the token.
// return:
//	0 for end-of-string
//	> for >
//	| for |
//	w for a word
//
// eventually (once we parse the space where the nul will go),
// words get nul-terminated.
#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

int
_gettoken(char *s, char **p1, char **p2)
{
	int t;

	if (s == 0) {
		if (debug > 1) printf("GETTOKEN NULL\n");
		return 0;
	}

	if (debug > 1) printf("GETTOKEN: %s\n", s);

	*p1 = 0;
	*p2 = 0;

	while(strchr(WHITESPACE, *s))
		*s++ = 0;
	if(*s == 0) {
		if (debug > 1) printf("EOL\n");
		return 0;
	}
	if(strchr(SYMBOLS, *s)){
		t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		if (debug > 1) printf("TOK %c\n", t);
		return t;
	}
	*p1 = s;
	while(*s && !strchr(WHITESPACE SYMBOLS, *s))
		s++;
	*p2 = s;
	if (debug > 1) {
		t = **p2;
		**p2 = 0;
		printf("WORD: %s\n", *p1);
		**p2 = t;
	}
	return 'w';
}

int
gettoken(char *s, char **p1)
{
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 16
void
runcmd(char *s)
{
	char *argv[MAXARGS], *t;
	int argc, c, i, r, p[2], fd, rightpipe;

	rightpipe = 0;
	gettoken(s, 0);
again:
	argc = 0;
	for(;;){
		c = gettoken(0, &t);
		switch(c){
		case 0:
			goto runit;
		case 'w':
			if(argc == MAXARGS){
				printf("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;
		case '<':
			if(gettoken(0, &t) != 'w'){
				printf("syntax error: < not followed by word\n");
				exit();
			}
			// Your code here -- open t for reading,
			// dup it onto fd 0, and then close the fd you got.
			//demo2s_code_start;
			if((fd=open(t,O_RDONLY))<0) {
				panic("%e\t%s",fd,t);
				break;
			}
			if((r=dup(fd,0))<0) {
				panic("can not dup! %e",r);
				break;
			}
			if((r=close(fd))<0) {
				panic("can not close %d! %e",fd,r);
				break;
			}

			break;
		case '>':
			if(gettoken(0, &t) != 'w'){
				printf("syntax error: > not followed by word\n");
				exit();
			}
			// Your code here -- open t for writing,
			// dup it onto fd 1, and then close the fd you got.
			if((fd=open(t,O_WRONLY))<0) {
				panic("%e\t%s",fd,t);
				break;
			}
			/*if((r=ftruncate(fd,0))<0) {
				printf("can not truncate! %e",r);
				break;
			}
			if((r=close(fd))<0) {
				panic("can not close %d! %e",fd,r);
				break;
			}
			*/
			if((fd=open(t,O_WRONLY))<0) {
				panic("%e\t%s",fd,t);
				break;
			}
			if((r=dup(fd,1))<0) {
				panic("can not dup! %e",r);
				break;
			}
			if((r=close(fd))<0) {
				panic("can not close %d! %e",fd,r);
				break;
			}
			break;
		case '|':
			// Your code here.
			// 	First, allocate a pipe.
			if((r=pipe(p))<0) {
				panic("can not pipe! %e",r);
				break;
			}
			//	Then fork.
			if((r=fork())<0) {
				panic("fork %e",r);
				break;
			}
			//	the child runs the right side of the pipe:
			//		dup the read end of the pipe onto 0
			//		close the read end of the pipe
			//		close the write end of the pipe
			//		goto again, to parse the rest of the command line
			if(r==0) {
				if((r=dup(p[0],0))<0) {
					panic("dup %e",r);
					break;
				}
				if((r=close(p[0]))<0) {
					panic("close read end %e",r);
					break;
				}
				if((r=close(p[1]))<0) {
					panic("close write end %e",r);
					break;
				}
				goto again;
			}
			//	the parent runs the left side of the pipe:
			//		dup the write end of the pipe onto 1
			//		close the write end of the pipe
			//		close the read end of the pipe
			//		set "rightpipe" to the child envid
			//		goto runit, to execute this piece of the pipeline
			//			and then wait for the right side to finish
			rightpipe=r;
			if((r=dup(p[1],1))<0) {
				panic("dup %e",r);
				break;
			}
			if((r=close(p[0]))<0) {
				panic("close write end %e",r);
				break;
			}
			if((r=close(p[1]))<0) {
				panic("close read end %e",r);
				break;
			}
			goto runit;
			//demo2s_code_end;
			break;
		}
	}

runit:
	if(argc == 0) {
		if (debug) printf("EMPTY COMMAND\n");
		return;
	}
	argv[argc] = 0;
	if (debug) {
		printf("[%08x] SPAWN:", env->env_id);
		for (i=0; argv[i]; i++)
			printf(" %s", argv[i]);
		printf("\n");
	}
	if ((r = spawn(argv[0], argv)) < 0)
		printf("spawn %s: %e\n", argv[0], r);
	close_all();
	if (r >= 0) {
		if (debug) printf("[%08x] WAIT %s %08x\n", env->env_id, argv[0], r);
		wait(r);
		if (debug) printf("[%08x] wait %08x finished\n", env->env_id,r);
	}
	if (rightpipe) {
		if (debug) printf("[%08x] WAIT right-pipe %08x\n", env->env_id, rightpipe);
		wait(rightpipe);
		if (debug) printf("[%08x] wait finished\n", env->env_id);
	}
	exit();
}

void
readline(char *buf, u_int n)
{
	int i, r;

	r = 0;
	for(i=0; i<n; i++){
		if((r = read(0, buf+i, 1)) != 1){
			if(r < 0)
				printf("read error: %e", r);
			exit();
		}
		if(buf[i] == '\b'){
			if(i > 0)
				i -= 2;
			else
				i = 0;
		}
		if(buf[i] == '\n'){
			buf[i] = 0;
			return;
		}
	}
	printf("line too long\n");
	while((r = read(0, buf, 1)) == 1 && buf[0] != '\n')
		;
	buf[0] = 0;
}	

char buf[1024];

void
usage(void)
{
	printf("usage: sh [-dix] [command-file]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int r, interactive, echocmds;

	interactive = '?';
	echocmds = 0;
	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();
	if(argc == 1){
		close(0);
		if ((r = open(argv[1], O_RDONLY)) < 0)
			panic("open %s: %e", r);
		assert(r==0);
	}
	if(interactive == '?')
		interactive = iscons(0);
	for(;;){
		if (interactive)
			fprintf(1, "$ ");
		readline(buf, sizeof buf);
		if (debug) printf("LINE: %s\n", buf);
		if (buf[0] == '#')
			continue;
		if (echocmds)
			fprintf(1, "# %s\n", buf);
		if ((r = fork()) < 0)
			panic("fork: %e", r);
		if (r == 0) {
			runcmd(buf);
			exit();
		} else
			wait(r);
	}
}


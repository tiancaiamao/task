#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include "task.h"
#include "impl.h"

enum {
	MAXFD = 1024
};
static struct pollfd fdarray[MAXFD];
static struct task* polltask[MAXFD];
static int nfds;
static int init = 0;

static void fdtask(void *arg) {
	int nready;
	int i;

	task_daemon();
	while(1) {
		while(task_yield() != 0);	//the return value means whether the task been hand off, 0 means not

		nready = poll(fdarray,nfds,-1);
	        if(nready < 0) {
			if(errno == EINTR) 
				continue;
			perror("poll error");
		}
		i = 0;
		//wake up the ready tasks
		while(i<nfds && nready>0) {
			if(fdarray[i].revents) {
				nready--;
				task_ready(polltask[i]);
				--nfds;
				fdarray[i] = fdarray[nfds];
				polltask[i] = polltask[nfds];
				continue;
			}
			i++;
		}
	}
}

int fdnoblock(int fd) {
	return fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)|O_NONBLOCK);
}

static void fdwait(int fd,int rw) {
	int bits;
	if(init == 0) {
		init = 1;
		task_create(fdtask,0,4096);
	}
	if(nfds >= MAXFD) {
		fprintf(stderr,"too many poll fds");
		abort();
	}
	switch(rw) {
		case 'r':
			bits |= POLLIN;
			break;
		case 'w':
			bits |= POLLOUT;
			break;
	}
	
	polltask[nfds] = running;
	fdarray[nfds].fd = fd;
	fdarray[nfds].events = bits;
	fdarray[nfds].revents = 0;
	nfds++;
	//don't put it in ready queue because it's blocked by I/O. so we call swapcontext directlly
	running->status = BLOCKED;
	swapcontext(&running->context,&schedule_context);
}

int fdread(int fd,void *buf,int n) {
	int m;
	
	while((m=read(fd,buf,n) < 0) && errno==EAGAIN)
		fdwait(fd,'r');
	return m;
}

int fdwrite(int fd,void *buf,int n) {
	int m,tot;

	for(tot=0; tot<n; tot+=m) {
		while((m=write(fd,(char*)buf+tot,n-tot))<0 && errno==EAGAIN)
			fdwait(fd,'w');
		if(m < 0)
			return m;
		if(m == 0)
			break;
	}
	return tot;
}


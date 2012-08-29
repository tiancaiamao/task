#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "impl.h"
#include "task.h"

static const char *status_string[] = {"running","ready","blocked","exiting"};
struct task **all;
uint allsize;
static char *schedule_stack;

struct list {
	struct list *next;
};
static struct list *free_slot;

static struct task *ready_head;
static struct task *ready_tail;
struct task *running;
ucontext_t schedule_context;

static const char* task_status(struct task *t) {
	if(t==NULL || t->status >= TASK_STATUS_MAX) {
		return "error status";
	}
	return status_string[t->status];
}
//the reverse process of init. tackle with resource release, also delete dead lock!
void task_exit (int val) {
	struct list *p;
	int i;
	int nblock = 0;

	while(free_slot != NULL) {
		p = free_slot;
		free_slot = free_slot->next;
		p->next = NULL;
	}
	for(i=1; i<allsize; i++) {
		if(all[i] != NULL) {
			if(all[i]->status == BLOCKED) {
				nblock++;
				free(all[i]);
				all[i] = NULL;
			} else {
				if(!all[i]->daemon)
					fprintf(stderr,"fatal error: some task have error status,pid=%d,status=%s\n",i,task_status(all[i]));
			}
		}
	}
	if(nblock != 0) {
		fprintf(stderr,"some task is blocked when program exit,maybe a dead lock!");
	}
	free(schedule_stack);
	exit(val);
}
void task_daemon() {
	running->daemon = 1;
}
static void task_helper(uint tid);
static void schedule() {
	struct task *rt;

	fprintf(stderr,"run here in schedule the first time\n");
	while(1) {
		rt = ready_head;
		if(rt == NULL) {
//			printf("no task to run now exit");
			task_exit(-1);
		}
		running = ready_head;
		ready_head = ready_head->next;
		running->status = RUNNING;

		if(swapcontext(&schedule_context,&rt->context) < 0) {
			fprintf(stderr,"swapcontext failed");
			exit(0);
		}
		//RUNNING is not permited,maybe an error happened!  only three possible status here:
		//READY means the running called task_yield
		//BLOCKED means task blocked by channel or I/O
		//EXITING means task finished and it's resources need to be collected
		if(running->status == EXITING) {
			free(running);
			running = NULL;
		}
	}
}
//NOTE: delay the release of memory here,because this function is run in the task's stack!
static void _task_exit() {
	struct list *p;

	running->status = EXITING;
	p = (struct list*)&all[running->tid];
	p->next = free_slot;
	free_slot = p;
}
static void task_helper(uint tid) {
	struct task *rt;

	rt = all[tid];
	(rt->fn)(rt->arg);

	_task_exit();
}
static void slot_init_range(struct list *s,uint from,uint to) {
	int i;
	for(i=from; i<to; i++) {
		s[i].next = &s[i+1];
	}
	s[to-1].next = NULL;
}
static void init() {
	srand(time(NULL));
	all = calloc(64,sizeof(struct task*));

	slot_init_range((struct list*)all,0,64);
	free_slot = (struct list*)all;
	allsize = 64;
	ready_tail = NULL;
	ready_head = NULL;
	running = NULL;
}
/* 
 * this function do two things:
 * 1.create the scheduler which have it's own stack and context,but the scheduler is not a task
 * 2.obtain the main's task struct by calling swapcontext
 * */
int task_init() {
	char *stack;
	int main;

	init();
	schedule_stack = malloc(DEFAULT_STACKSIZE);
	if(schedule_stack == NULL) {
		fprintf(stderr,"out of memory in task_init\n");
		return -1;
	}
	if(getcontext(&schedule_context) < 0) {
		perror("getcontext error in task_init:");
		return -1;
	}
	schedule_context.uc_stack.ss_sp = schedule_stack;
	schedule_context.uc_stack.ss_size = DEFAULT_STACKSIZE;
	schedule_context.uc_stack.ss_flags = 0;
	makecontext(&schedule_context,(void(*)())schedule,0);

	main = task_create(NULL,NULL,0);
	swapcontext(&all[main]->context,&schedule_context);
	return 0;
}
void task_ready(struct task *t) {
	if(ready_head == NULL) {
		ready_head = t;
		ready_tail = t;
	} else {
		ready_tail->next = t;
		ready_tail = t;
	}
	t->status = READY;
}
int task_create(void (*f)(void*),void *arg,int stacksize) {
	struct task *ret;
	uint id;

	ret = malloc(sizeof(struct task) + stacksize);
	if(ret == NULL) {
		fprintf(stderr,"out of memory");
		exit(-1);
	}

	ret->stksize = stacksize;
	ret->fn = f;
	ret->arg = arg;
	ret->daemon = 0;
	ret->next = NULL;

	if(getcontext(&ret->context) < 0) {
		perror("getcontext");
		exit(-1);
	}
	ret->context.uc_stack.ss_sp = ret->stk;
	ret->context.uc_stack.ss_size = ret->stksize;
	ret->context.uc_stack.ss_flags = 0;
	ret->context.uc_link = &schedule_context;

	task_ready(ret);
	//put ret in all queue
	if(free_slot == NULL) {
		int i;
		all = realloc(all,(allsize+16)*sizeof(struct task*));
		if(all== NULL) {
			fprintf(stderr,"out of memory");	
			exit(-1);
		}
		slot_init_range((struct list*)all,allsize,allsize + 16);
		free_slot = (struct list*)&all[allsize];
		allsize += 16;
	}
	id = (struct task**)free_slot - all;
	ret->tid = id;
	free_slot = free_slot->next;
	all[id]= ret;
	makecontext(&ret->context,(void(*)())task_helper,1,id);

	return id;
}
int task_yield(void) {
	int ret = (ready_head != NULL);

	task_ready(running);
	swapcontext(&running->context,&schedule_context);
	return ret;
}
/*
static void task_init(char *argv[]) {
	uint tid;
	tid = task_create((void (*)())task_main,argv,DEFAULT_STACKSIZE);
	running = all[0];
}

int main(int argc,char *argv[]) {
	init();
	task_create((void (*)())task_main,argv,DEFAULT_STACKSIZE);
	schedule();
	return 0;
}*/


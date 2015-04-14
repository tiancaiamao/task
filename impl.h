/*
 * the structs used by implemention are all in this file
 */
#ifndef _IMPL_H

#include "context.h"
#define DEFAULT_STACKSIZE 2048

enum TASK_STATUS {
	RUNNING,
	READY,
	BLOCKED,
	EXITING,
	TASK_STATUS_MAX
};

struct task {
	struct task *next;	//linked ready tasks
	int tid;
	int stksize;
	void (*fn)(void *arg);
	void *arg;
	struct Context context;
	void *p;	//for some data exchange between task and channel
	char status;
	char daemon;
	char *stk;
};

struct taskarray {
	struct task **array;
	unsigned cap;
	unsigned use;
};

struct channel {
	unsigned elemsize;
	unsigned elemnum;
	struct taskarray sender;	//those task that write this chan and blocked
	struct taskarray receiver;	//those who that this chan and blocked
	unsigned head;
	unsigned use;
	void *p;
	char buf[0];
};

extern struct Context schedule_context;
extern struct task *running;
void task_ready(struct task *t);

#endif

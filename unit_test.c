#include <stdio.h>

#ifdef _TASK_TEST
#include "task.h"
void hello(void *arg) {
	int stack;
	printf("hello world %d!\n",stack);
}
int main() {
	task_init();
	task_create(hello,NULL,31242);
	task_yield();
	task_exit(0);
}
#endif

#ifdef _CHAN_TEST

void hello(void *arg) {
	struct channel *c = arg;
	int val = 34;

	chan_send(c,&val);
}

int main() {
	struct channel *c;
	int p;

	task_init();
	c = chan_create(sizeof(int),1);
	task_create(hello,c,4096);
	chan_recv(c,&p);
	printf("received val: %d\n",p);
	chan_free(c);
	task_exit(0);
}
#endif

#ifdef _FD_TEST

void hello(void *arg) {
	fprintf(stderr,"the other task run!");
}
int main() {
	char buf[55555];

	task_init();
	task_create(hello,NULL,1024);
	fdnoblock(0);
	fdread(0,buf,55555);
	task_exit(0);
}
#endif

#include <stdio.h>

#ifdef _TASK_TEST
#include <stdio.h>
#include "task.h"

void P(void *ptr) {
    int id = *(int *)ptr;
    int i;

    for (i = 0; i < 10; i++) {
        printf("%d loop %d\n", id, i);
        TaskYield();
    }
}

void TaskMain(void *argv) {
    int m = 1;
    int p = 2;
    TaskCreate(P, &p);
    P(&m);
}
#endif

#ifdef _CHAN_TEST

void hello(void *arg) {
    struct channel *c = arg;
    int val = 34;

    chan_send(c, &val);
}

int main() {
    struct channel *c;
    int p;

    task_init();
    c = chan_create(sizeof(int), 1);
    TaskCreate(hello, c, 4096);
    chan_recv(c, &p);
    printf("received val: %d\n", p);
    chan_free(c);
    TaskExit(0);
}
#endif

#ifdef _FD_TEST

void hello(void *arg) { fprintf(stderr, "the other task run!"); }
int main() {
    char buf[55555];

    task_init();
    TaskCreate(hello, NULL, 1024);
    fdnoblock(0);
    fdread(0, buf, 55555);
    TaskExit(0);
}
#endif

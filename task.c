#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "impl.h"
#include "task.h"

static const char *status_string[] = {"running", "ready", "blocked", "exiting"};
struct task **all;
uint allsize;
static char *schedule_stack;

struct list {
    struct list *next;
};
static struct list *free_slot;

struct queue {
    struct task *head;
    struct task *tail;
};

static void _queue_push(struct queue *q, struct task *t) {
    if (q->head == NULL) {
        q->head = t;
        q->tail = t;
    } else {
        q->tail->next = t;
        q->tail = t;
    }
}

static struct task *_queue_pop(struct queue *q) {
    struct task *ret;

    if (q->head == NULL) {
        return NULL;
    }

    ret = q->head;
    ret->next = NULL;
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }

    return ret;
}

static struct queue ready;
// static struct task *ready_head;
// static struct task *ready_tail;
struct task *running;
struct Context schedule_context;

static const char *task_status(struct task *t) {
    if (t == NULL || t->status >= TASK_STATUS_MAX) {
        return "error status";
    }
    return status_string[t->status];
}

// the reverse process of init. tackle with resource release, also delete dead
// lock!
void TaskExit(int val) {
    struct list *p;
    int i;
    int nblock = 0;

    while (free_slot != NULL) {
        p = free_slot;
        free_slot = free_slot->next;
        p->next = NULL;
    }
    for (i = 1; i < allsize; i++) {
        if (all[i] != NULL) {
            if (all[i]->status == BLOCKED) {
                nblock++;
                free(all[i]);
                all[i] = NULL;
            } else {
                if (!all[i]->daemon)
                    fprintf(stderr, "fatal error: some task have error "
                                    "status,pid=%d,status=%s\n",
                            i, task_status(all[i]));
            }
        }
    }
    if (nblock != 0) {
        fprintf(stderr,
                "some task is blocked when program exit,maybe a dead lock!");
    }
    free(schedule_stack);
    exit(val);
}
void TaskDaemon() { running->daemon = 1; }

static void schedule() {
    struct task *rt;

    fprintf(stderr, "run here in schedule the first time\n");
    while (1) {
        rt = _queue_pop(&ready);
        if (rt == NULL) {
            fprintf(stderr, "no task to run now exit\n");
            TaskExit(-1);
        }

        running = rt;
        running->status = RUNNING;

        SwapContext(&schedule_context, &rt->context);

        // RUNNING is not permited,maybe an error happened!  only three possible
        // status here:
        // READY means the running called TaskYield
        // BLOCKED means task blocked by channel or I/O
        // EXITING means task finished and it's resources need to be collected
        if (running->status == EXITING) {
            free(running);
            running = NULL;
        }
    }
}

// NOTE: delay the release of memory here,because this function is run in the
// task's stack!
static void _TaskExit() {
    struct list *p;

    running->status = EXITING;
    p = (struct list *)&all[running->tid];
    p->next = free_slot;
    free_slot = p;

    SwapContext(&running->context, &schedule_context);
}

static void task_helper(void *ptr) {
    struct task *rt;

    rt = *((struct task **)&ptr + 2);
    (rt->fn)(rt->arg);

    _TaskExit();
}
static void slot_init_range(struct list *s, uint from, uint to) {
    int i;
    for (i = from; i < to; i++) {
        s[i].next = &s[i + 1];
    }
    s[to - 1].next = NULL;
}
static void init() {
    // srand(time(NULL));
    all = calloc(64, sizeof(struct task *));

    slot_init_range((struct list *)all, 0, 64);
    free_slot = (struct list *)all;
    allsize = 64;

    ready.tail = NULL;
    ready.head = NULL;
    running = NULL;
}

void task_ready(struct task *t) {
    t->status = READY;
    _queue_push(&ready, t);
}

int TaskCreate(void (*f)(void *), void *arg) {
    struct task *ret;
    int stacksize = DEFAULT_STACKSIZE;
    uint id;

    ret = malloc(sizeof(struct task));
    if (ret == NULL) {
        fprintf(stderr, "out of memory");
        exit(-1);
    }

    char *stack = malloc(stacksize);
    if (stack == NULL) {
        fprintf(stderr, "out of memory");
        exit(-1);
    }

    // setup stack
    *(uint64_t *)&stack[stacksize - 8] = (uint64_t)ret;
    *(uint64_t *)&stack[stacksize - 16] = (uint64_t)task_helper;
    ret->context.rsp = (uint64_t)&stack[stacksize - 16];

    // field of task struct
    ret->stk = stack;
    ret->stksize = stacksize;
    ret->fn = f;
    ret->arg = arg;
    ret->daemon = 0;
    ret->next = NULL;

    task_ready(ret);

    // put ret in all queue
    if (free_slot == NULL) {
        int i;
        all = realloc(all, (allsize + 16) * sizeof(struct task *));
        if (all == NULL) {
            fprintf(stderr, "out of memory");
            exit(-1);
        }
        slot_init_range((struct list *)all, allsize, allsize + 16);
        free_slot = (struct list *)&all[allsize];
        allsize += 16;
    }
    id = (struct task **)free_slot - all;
    ret->tid = id;
    free_slot = free_slot->next;
    all[id] = ret;

    return id;
}

int TaskYield(void) {
    if (ready.head == NULL) {
        return 0;
    }

    task_ready(running);
    SwapContext(&running->context, &schedule_context);
    return 1;
}

int main(int argc, char *argv[]) {
    init();
    TaskCreate((void (*)(void *))TaskMain, argv);
    schedule();
    return 0;
}

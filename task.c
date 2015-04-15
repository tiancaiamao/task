#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "impl.h"
#include "task.h"

static const char *status_string[] = {"running", "ready", "blocked", "exiting"};
static const char *task_status(struct task *t);

struct list {
    struct list *next;
};

static void slot_init_range(struct list *s, uint from, uint to) {
    int i;
    for (i = from; i < to; i++) {
        s[i].next = &s[i + 1];
    }
    s[to - 1].next = NULL;
}

// get task by id && assign id to task
struct taskmap {
    struct task **all;
    int allsize;
    struct list *free_slot;
};

void taskmap_init(struct taskmap *tm) {
    tm->all = calloc(64, sizeof(struct task *));
    slot_init_range((struct list *)tm->all, 0, 64);
    tm->free_slot = (struct list *)tm->all;
    tm->allsize = 64;
}

void taskmap_exit(struct taskmap *tm) {
    struct list *p;
    int i;
    int nblock = 0;

    // firstly, set slots in free list to NULL
    while (tm->free_slot != NULL) {
        p = tm->free_slot;
        tm->free_slot = tm->free_slot->next;
        p->next = NULL;
    }

    // secondly, free the remain non-NULL slot in all array
    for (i = 1; i < tm->allsize; i++) {
        if (tm->all[i] != NULL) {
            if (tm->all[i]->status == BLOCKED) {
                nblock++;
                free(tm->all[i]);
                tm->all[i] = NULL;
            } else {
                if (!tm->all[i]->daemon)
                    fprintf(stderr, "fatal error: some task have error "
                                    "status,pid=%d,status=%s\n",
                            i, task_status(tm->all[i]));
            }
        }
    }
    if (nblock != 0) {
        fprintf(stderr,
                "some task is blocked when program exit,maybe a dead lock!");
    }
}

void taskmap_release(struct taskmap *tm, int tid) {
    struct list *p = (struct list *)&tm->all[tid];
    p->next = tm->free_slot;
    tm->free_slot = p;
}

int taskmap_new(struct taskmap *tm, struct task *t) {
    int id;

    if (tm->free_slot == NULL) {
        int i;
        tm->all = realloc(tm->all, (tm->allsize + 16) * sizeof(struct task *));
        if (tm->all == NULL) {
            goto OOM;
        }

        slot_init_range((struct list *)tm->all, tm->allsize, tm->allsize + 16);
        tm->free_slot = (struct list *)&tm->all[tm->allsize];
        tm->allsize += 16;
    }
    id = (struct task **)tm->free_slot - tm->all;

    tm->free_slot = tm->free_slot->next;
    tm->all[id] = t;

    return id;

OOM:
    fprintf(stderr, "out of memory");
    exit(-1);
}

// TODO
struct task *taskmap_grab(struct taskmap *tm, int tid) {
    return NULL;
}

struct queue {
    struct task *head;
    struct task *tail;
};

static void _queue_init(struct queue *q) {
    q->head = NULL;
    q->tail = NULL;
}

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
static struct taskmap tm;

struct task *running;
struct Context schedule_context;

static const char *task_status(struct task *t) {
    if (t == NULL || t->status >= TASK_STATUS_MAX) {
        return "error status";
    }
    return status_string[t->status];
}

// the reverse process of init.
// tackle with resource release, also delete dead lock!
void TaskExitAll(int val) {
    taskmap_exit(&tm);
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
            TaskExitAll(-1);
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
            free(running->stk);
            free(running);
            running = NULL;
        }
    }
}

// NOTE: delay the release of memory here,because this function is run in the
// task's stack!
void TaskExit() {
    running->status = EXITING;
    taskmap_release(&tm, running->tid);
    SwapContext(&running->context, &schedule_context);
}

static void task_helper(void *ptr) {
    struct task *rt;

    rt = *((struct task **)&ptr + 2);
    (rt->fn)(rt->arg);

    TaskExit();
}

static void init() {
    // srand(time(NULL));
    taskmap_init(&tm);
    _queue_init(&ready);
    running = NULL;
}

void task_ready(struct task *t) {
    t->status = READY;
    _queue_push(&ready, t);
}

int TaskCreate(void (*f)(void *), void *arg) {
    struct task *ret;
    int stacksize = DEFAULT_STACKSIZE;
    int tid;

    ret = malloc(sizeof(struct task));
    if (ret == NULL) {
        goto OOM;
    }

    char *stack = malloc(stacksize);
    if (stack == NULL) {
        goto OOM;
    }

    tid = taskmap_new(&tm, ret);

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
    ret->tid = tid;
    task_ready(ret);

    return tid;

OOM:
    fprintf(stderr, "out of memory");
    exit(-1);
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

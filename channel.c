#include <stdlib.h>
#include <string.h>
#include "impl.h"
#include "task.h"

void taskarray_add(struct taskarray *ta,struct task * t) {
	if(ta->use == ta->cap) {
		if(ta->array == NULL) {
			ta->array = malloc(16 * sizeof(struct task*));
			ta->cap = 16;
		} else {
			ta->array = realloc(ta->array,ta->cap + 16);
			ta->cap += 16;
		}
	}
	ta->array[ta->use] = t;
	ta->use++;
}
void taskarray_del(struct taskarray *ta,int id) {
	ta->array[id] = ta->array[ta->use-1];
	ta->use--;
}
struct channel*	chan_create(int elemsize, int elemnum) {
	struct channel *ret;

	ret = malloc(sizeof(struct channel) + elemsize*elemnum);
	ret->elemsize = elemsize;
	ret->elemnum = elemnum;
	ret->sender.array = NULL;
	ret->sender.cap = 0;
	ret->sender.use = 0;
	ret->receiver.array = NULL;
	ret->receiver.cap = 0;
	ret->receiver.use = 0;
	ret->p = NULL;
	ret->head = 0;
	ret->use = 0;

	return ret;
}
void chan_free(struct channel *c) {
	if(c->sender.array != NULL)
		free(c->sender.array);
	if(c->receiver.array != NULL)
		free(c->receiver.array);
	free(c);
}
int	chan_nbrecv(struct channel *c, void *v);
void*	chan_nbrecvp(struct channel *c);
unsigned long	chan_nbrecvul(struct channel *c);
int	chan_nbsend(struct channel *c, void *v);
int	chan_nbsendp(struct channel *c, void *v);
int	chan_nbsendul(struct channel *c, unsigned long v);
/*
void*	chan_recvp(struct channel *c) {
	void *ret;
	int pick;
	while(c->p == NULL) {
		taskarray_add(&c->receiver,running);
		running->status = BLOCKED;
		swapcontext(&running->context,&schedule_context);
	}
	ret = c->p;
	c->p = NULL;
	//wake up one blocked task
	if(c->sender.use != 0) {
		pick = rand() % c->sender.use;
		taskarray_del(&c->sender,pick);
		task_ready(c->sender.array[pick]);
		return ret;
	}
}*/
int chan_recv(struct channel *c,void *v) {
	int pick;
	char *tail;
	if(c->use == 0) {
		taskarray_add(&c->receiver,running);	
		running->status = BLOCKED;
		running->p = v;
		swapcontext(&running->context,&schedule_context);
		return 1;
	} else {
		memcpy(v,c->buf+c->head*c->elemsize,c->elemsize);
		c->head = (c->head+1) % c->elemnum;
		c->use--;
		if(c->sender.use != 0) {
			pick = rand() % c->sender.use;
			tail = &c->buf[((c->head+c->use)%c->elemnum) * c->elemsize];
			memcpy(tail, c->sender.array[pick]->p, c->elemsize);
			task_ready(c->sender.array[pick]);
			c->use++;
			taskarray_del(&c->sender,pick);
		}
		return 0;
	}
}
unsigned long	chan_recvul(struct channel *c);
/*
int chan_sendp(struct channel *c,void *v) {
	int pick;

	while(c->p != NULL) {
		taskarray_add(&c->sender,running);
		running->status = BLOCKED;
		swapcontext(&running->context,&schedule_context);
	}
	c->p = v;
	if(c->receiver.use != 0) {
		pick = rand() % c->receiver.use;
		taskarray_del(&c->receiver,pick);
		task_ready(c->receiver.array[pick]);
	}
	return 0;
}*/
int chan_send(struct channel *c, void *v) {
	char *tail;
	int pick;
	if(c->use == c->elemnum) {
		taskarray_add(&c->sender,running);
		running->status = BLOCKED;
		running->p = v;
		swapcontext(&running->context,&schedule_context);
		return 1;
	} else {
		tail = &c->buf[((c->head+c->use)%c->elemnum) * c->elemsize];
		memcpy(tail,v,c->elemsize);
		c->use++;
		if(c->receiver.use != 0) {
			pick = rand() % c->receiver.use;
			memcpy(c->receiver.array[pick]->p,c->buf+c->head*c->elemsize,c->elemsize);
			c->head = (c->head+1) % c->elemnum;
			c->use--;
			task_ready(c->receiver.array[pick]);
			taskarray_del(&c->receiver,pick);
		}
		return 0;
	}
}
int	chan_sendul(struct channel *c, unsigned long v);



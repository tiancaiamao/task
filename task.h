/*
 * this file define the interface of the task library.
 */
#ifndef _TASK_H

#ifdef __cplusplus
extern "C"
{
#endif
	struct task;
	struct channel;

	//int		anyready(void);
	int	TaskCreate(void (*f)(void *arg), void *arg);
	void TaskExit();
	void TaskExitAll(int);
	void TaskMain(void*);
	int	TaskYield(void);
	void TaskDaemon(void);
	//void		tasksystem(void);
	//unsigned int	taskdelay(unsigned int);
	//unsigned int	taskid(void);

	enum
	{
		CHANEND,
		CHANSND,
		CHANRCV,
	};

	struct alt {
		struct channel *c;
		void		*v;
		int		op;
	};
	
	struct channel*	chan_create(int elemsize, int elemcnt);
	int	chan_send(struct channel *c,void *v);
	int	chan_recv(struct channel *c,void *v);
	int chan_select(struct alt *arr);
	void chan_close(struct channel *c, void *v);
	void	chan_free(struct channel *c);
	//int	chan_nbrecv(struct channel *c, void *v);
	//void*	chan_nbrecvp(struct channel *c);
	//unsigned long	chan_nbrecvul(struct channel *c);
	//int	chan_nbsend(struct channel *c, void *v);
	//int	chan_nbsendp(struct channel *c, void *v);
	//int	chan_nbsendul(struct channel *c, unsigned long v);
	//void*	chan_recvp(struct channel *c);
	//unsigned long	chan_recvul(struct channel *c);
	//int	chan_sendp(struct channel *c, void *v);
	//int	chan_sendul(struct channel *c, unsigned long v);

	int fdnoblock(int fd);
	int fdwrite(int fd,void *buf,int n);
	int fdread(int fd,void *buf,int n);

#ifdef __cpluspluse
}
#endif

#endif

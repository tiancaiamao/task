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
	int	task_create(void (*f)(void *arg), void *arg,int stacksize);
	void	task_exit(int);
	void	task_exitall(int);
	void	task_main(int argc, char *argv[]);
	int	task_yield(void);
	void	task_exit(int val);
	void	task_daemon();
	//void		tasksystem(void);
	//unsigned int	taskdelay(unsigned int);
	//unsigned int	taskid(void);

	struct channel*	chan_create(int elemsize, int elemcnt);
	void	chan_free(struct channel *c);
	//int	chan_nbrecv(struct channel *c, void *v);
	//void*	chan_nbrecvp(struct channel *c);
	//unsigned long	chan_nbrecvul(struct channel *c);
	//int	chan_nbsend(struct channel *c, void *v);
	//int	chan_nbsendp(struct channel *c, void *v);
	//int	chan_nbsendul(struct channel *c, unsigned long v);
	int	chan_recv(struct channel *c,void *v);
	//void*	chan_recvp(struct channel *c);
	//unsigned long	chan_recvul(struct channel *c);
	int	chan_send(struct channel *c,void *v);
	//int	chan_sendp(struct channel *c, void *v);
	//int	chan_sendul(struct channel *c, unsigned long v);

	int fdnoblock(int fd);
	int fdwrite(int fd,void *buf,int n);
	int fdread(int fd,void *buf,int n);

#ifdef __cpluspluse
}
#endif

#endif

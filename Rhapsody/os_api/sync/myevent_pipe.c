/*
*
*myevent_pipe.c 利用select pipe实现一个事件封装,uclinux下面pthread_cond_xxx这几个函数好像有问题,原因不明
*
*/


#include "myevent.h"

#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

#include "mypipe.h"
#include "mymutex.h"
#include "mylog.h"


typedef struct __myevent_t_
{
	HMYMEMPOOL hm;

	/*
	* 当前有几个线程在等待
	*/
	volatile int nThrWait;
	
	/*
	* nThrWait的保护锁
	*/
	HMYMUTEX protecter;
	
	/*
	* 做为信号使用的管道
	*/
	HMYPIPE hpipeEvent;	
}myevent_t;


static void pipe_event_broadcast(myevent_t * me)
{
	volatile int nThrWait = 0;

	char a[1] = {0};
	fd_set write_mask;
	struct timeval timeout = {1};

	assert(me);

	while(1)
	{		
		/* 获取当前等待进程的个数 */
		MyMutexLock(me->protecter);
		nThrWait = me->nThrWait;	
		/* 解锁 */
		MyMutexUnLock(me->protecter);
		
		LOG_DEBUG(("%x - %d", me, nThrWait));
		if(!nThrWait)
			break;
		
		LOG_DEBUG(("write pipe"));
		MyPipeWrite(me->hpipeEvent, a, sizeof(a));
		LOG_DEBUG(("end write pipe"));

		/* 再次获取当前等待进程的个数 */
		MyMutexLock(me->protecter);
		nThrWait = me->nThrWait;	
		/* 解锁 */
		MyMutexUnLock(me->protecter);

		if(!nThrWait)
			break;
		
		/* 等待可写事件发生 */
		FD_ZERO(&write_mask);
		FD_SET(MyPipeGetReadFD(me->hpipeEvent), &write_mask);
		select(MyPipeGetReadFD(me->hpipeEvent) + 1, NULL, &write_mask, NULL, &timeout);
		
		LOG_DEBUG(("%d", nThrWait));
	}
}

static void destroy(myevent_t * me)
{
	if(NULL == me)
		return;

	pipe_event_broadcast(me);

	MyPipeDestruct(me->hpipeEvent);

	MyMutexDestruct(me->protecter);

	MyMemPoolFree(me->hm, me);
}

/*
*
*创建事件/条件
*
*/
HMYEVENT MyEventRealConstruct(HMYMEMPOOL hm, int bNotAutoReset, const char * pcname)
{
	myevent_t * me = (myevent_t *)MyMemPoolMalloc(hm, sizeof(*me));
	if(NULL == me)
		return NULL;

	me->nThrWait = 0;
	me->hm = hm;
	me->protecter = MyMutexConstruct(me->hm);
	me->hpipeEvent = MyPipeConstruct(me->hm);
	
	if(NULL == me->protecter || NULL == me->hpipeEvent)
	{
		LOG_WARN(("fail create event"));

		destroy(me);
		
		return NULL;
	}

	return (HMYEVENT)me;	
}

/*
*
*销毁事件/条件
*
*/
void MyEventDestruct(HMYEVENT he)
{
	myevent_t * me = (myevent_t *)he;
	if(NULL == me)
		return;

	destroy(me);
}

/*
*
*把事件设置成signaled状态
*
*/
void MyEventSetSignaled(HMYEVENT he)
{
	char a[1] = {0};

	myevent_t * me = (myevent_t *)he;
	if(NULL == me)
		return;

	MyPipeWrite(me->hpipeEvent, a, sizeof(a));
}

/*
*
*广播事件发生
*
*/
void MyEventBroadCast(HMYEVENT he)
{
	myevent_t * me = (myevent_t *)he;
	if(NULL == me)
		return;
	
	pipe_event_broadcast(me);
}

/*
*
*把事件设置成非signaled状态
*
*/
void MyEventSetNoSignaled(HMYEVENT he){}

/*
*
* 等待事件发生, 
*
*param millsecond:单位毫秒, -1表示无限等待,直至事件发生
*/
int MyEventWait(HMYEVENT he, mytv_t * timeout)
{
	char a[1];
	fd_set read_mask;
	struct timeval tv = {0};

	myevent_t * me = (myevent_t *)he;
	if(NULL == me)
		return -1;

	/* 加锁 */
	MyMutexLock(me->protecter);
	me->nThrWait += 1;	
	/* 解锁 */
	MyMutexUnLock(me->protecter);

	LOG_INFO(("in %x - %d", me, me->nThrWait));

	FD_ZERO(&read_mask);
	FD_SET(MyPipeGetReadFD(me->hpipeEvent), &read_mask);

	if(NULL == timeout)
		select(MyPipeGetReadFD(me->hpipeEvent) + 1, &read_mask, NULL, NULL, NULL);
	else
	{
		tv.tv_sec = timeout->tv_sec;
		tv.tv_usec = timeout->tv_usec;
		select(MyPipeGetReadFD(me->hpipeEvent) + 1, &read_mask, NULL, NULL, &tv);
	}

	if(FD_ISSET(MyPipeGetReadFD(me->hpipeEvent), &read_mask))
		MyPipeRead(me->hpipeEvent, a, sizeof(a));

	/* 加锁 */
	MyMutexLock(me->protecter);
	me->nThrWait -= 1;	
	assert(me->nThrWait >= 0);
	/* 解锁 */
	MyMutexUnLock(me->protecter);


	LOG_INFO(("out %x - %d", me, me->nThrWait));
	
	return 0;
}

















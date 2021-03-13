/*
*
*myevent_pipe.c ����select pipeʵ��һ���¼���װ,uclinux����pthread_cond_xxx�⼸����������������,ԭ����
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
	* ��ǰ�м����߳��ڵȴ�
	*/
	volatile int nThrWait;
	
	/*
	* nThrWait�ı�����
	*/
	HMYMUTEX protecter;
	
	/*
	* ��Ϊ�ź�ʹ�õĹܵ�
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
		/* ��ȡ��ǰ�ȴ����̵ĸ��� */
		MyMutexLock(me->protecter);
		nThrWait = me->nThrWait;	
		/* ���� */
		MyMutexUnLock(me->protecter);
		
		LOG_DEBUG(("%x - %d", me, nThrWait));
		if(!nThrWait)
			break;
		
		LOG_DEBUG(("write pipe"));
		MyPipeWrite(me->hpipeEvent, a, sizeof(a));
		LOG_DEBUG(("end write pipe"));

		/* �ٴλ�ȡ��ǰ�ȴ����̵ĸ��� */
		MyMutexLock(me->protecter);
		nThrWait = me->nThrWait;	
		/* ���� */
		MyMutexUnLock(me->protecter);

		if(!nThrWait)
			break;
		
		/* �ȴ���д�¼����� */
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
*�����¼�/����
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
*�����¼�/����
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
*���¼����ó�signaled״̬
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
*�㲥�¼�����
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
*���¼����óɷ�signaled״̬
*
*/
void MyEventSetNoSignaled(HMYEVENT he){}

/*
*
* �ȴ��¼�����, 
*
*param millsecond:��λ����, -1��ʾ���޵ȴ�,ֱ���¼�����
*/
int MyEventWait(HMYEVENT he, mytv_t * timeout)
{
	char a[1];
	fd_set read_mask;
	struct timeval tv = {0};

	myevent_t * me = (myevent_t *)he;
	if(NULL == me)
		return -1;

	/* ���� */
	MyMutexLock(me->protecter);
	me->nThrWait += 1;	
	/* ���� */
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

	/* ���� */
	MyMutexLock(me->protecter);
	me->nThrWait -= 1;	
	assert(me->nThrWait >= 0);
	/* ���� */
	MyMutexUnLock(me->protecter);


	LOG_INFO(("out %x - %d", me, me->nThrWait));
	
	return 0;
}

















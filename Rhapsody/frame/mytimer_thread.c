/*
*
*mytimer_thread.c 定时器线程 lin shao chuan
*
*/
#include "mytimer_thread.h"

#include <assert.h>
#ifdef WIN32
	#include <winsock2.h>
#endif

#include "mythread.h"
#include "myevent.h"
#include "gettimeofday.h"
#include "mylog.h"


typedef struct __mytimer_thread_t_
{
	HMYTHREAD hthr;
	HMYTIMERHEAP htmhp;

	HMYMEMPOOL hm;

	HMYEVENT wait_evt;
	
	int bNotExit;
}mytimer_thread_t;


static void * timer_thread_fun(void * param)
{
	mytimer_thread_t * tm_thr = (mytimer_thread_t *)param;
	struct timeval tv_earliest = {0};
	struct timeval tv_now = {0};

	assert(tm_thr && tm_thr->htmhp);
	assert(tm_thr->wait_evt);

	while(tm_thr->bNotExit)
	{
		if(0 == MyTimerHeapGetEarliestExpire(tm_thr->htmhp, (mytv_t *)&tv_earliest))
		{
			gettimeofday(&tv_now, NULL);
			if(timeval_smaller(tv_now, tv_earliest))
			{
				timeval_minus(tv_earliest, tv_now);
				MyEventWait(tm_thr->wait_evt, (mytv_t *)&tv_earliest);
			}
			else
				LOG_INFO(("null loop"));
		}
		else
			MyEventWait(tm_thr->wait_evt, NULL);
		
		gettimeofday(&tv_now, NULL);
		MyTimerHeapRunExpire(tm_thr->htmhp, (mytv_t *)&tv_now);
	}

	return NULL;
}

static __INLINE__ void timer_thread_inter_destroy(mytimer_thread_t * tm_thr)
{
	if(NULL == tm_thr)
		return;

	tm_thr->bNotExit = 0;
	MyEventSetSignaled(tm_thr->wait_evt);
	MyThreadJoin(tm_thr->hthr);

	if(tm_thr->hthr)
		MyThreadDestruct(tm_thr->hthr);
	tm_thr->hthr = NULL;

	if(tm_thr->htmhp)
		MyTimerHeapDestruct(tm_thr->htmhp);
	tm_thr->htmhp = NULL;

	if(tm_thr->wait_evt)
		MyEventDestruct(tm_thr->wait_evt);
	tm_thr->wait_evt = NULL;

	MyMemPoolFree(tm_thr->hm, tm_thr);
}


/*
*
*定时器线程构造
*
*/
HMYTIMER_THREAD MyTimerThreadConstruct(HMYMEMPOOL hm)
{
	mytimer_thread_t * tm_thr = (mytimer_thread_t *)MyMemPoolMalloc(hm, sizeof(*tm_thr));
	if(NULL == tm_thr)
		return NULL;

	tm_thr->hm = hm;
	tm_thr->wait_evt = MyEventConstruct(hm);
	tm_thr->htmhp = MyTimerHeapConstruct(hm);
	tm_thr->hthr = MyThreadConstruct(timer_thread_fun, tm_thr, 1, hm);
	tm_thr->bNotExit = 1;

	if(NULL == tm_thr->hthr || NULL == tm_thr->hthr || NULL == tm_thr->wait_evt)
		goto MyTimerThreadConstruct_err_;

	return (HMYTIMER_THREAD)tm_thr;

MyTimerThreadConstruct_err_:

	timer_thread_inter_destroy(tm_thr);
	
	return NULL;
}

/*
*
*定时器线程析构
*
*/
void MyTimerThreadDestruct(HMYTIMER_THREAD htm_thr)
{
	mytimer_thread_t * tm_thr = (mytimer_thread_t *)htm_thr;
	if(NULL == tm_thr)
		return;

	timer_thread_inter_destroy(tm_thr);
}

/*
*
*定时器线程析构
*
*/
void MyTimerThreadRun(HMYTIMER_THREAD htm_thr)
{
	mytimer_thread_t * tm_thr = (mytimer_thread_t *)htm_thr;
	if(NULL == tm_thr)
		return;

	MyThreadRun(tm_thr->hthr);
}

/*
*
*添加定时器
*
*/
HTIMERID MyTimerThreadAddTimer(HMYTIMER_THREAD htm_thr, mytimer_node_t * node)
{
	HTIMERID timer_id = NULL;
	mytimer_thread_t * tm_thr = (mytimer_thread_t *)htm_thr;
	if(NULL == tm_thr || NULL == tm_thr->htmhp || NULL == node || NULL == node->timeout_cb)
		return NULL;

	gettimeofday((struct timeval *)&node->abs_expire, NULL);
	timeval_add(node->abs_expire, node->first_expire);

	//先添加,再唤醒
	timer_id = MyTimerHeapAdd(tm_thr->htmhp, node);

	if(MyTimeHeapGetEarliestKey(tm_thr->htmhp) == timer_id)
		MyEventSetSignaled(tm_thr->wait_evt);

	return timer_id;
}

/*
*
*删除定时器
*
*/
int MyTimerThreadDelTimer(HMYTIMER_THREAD htm_thr, HTIMERID timer_id)
{
	mytimer_thread_t * tm_thr = (mytimer_thread_t *)htm_thr;
	if(NULL == tm_thr || NULL == tm_thr->htmhp)
		return -1;

	assert(tm_thr->htmhp);

	//删除之后,不存在有节点的超时比当前等待超时更短的情况,所以不用唤醒信号
	return MyTimerHeapDel(tm_thr->htmhp, timer_id);
}

/*
*
*重置定时器
*
*/
HTIMERID MyTimerThreadResetTimer(HMYTIMER_THREAD htm_thr, HTIMERID timer_id, mytimer_node_t * node)
{
	mytimer_thread_t * tm_thr = (mytimer_thread_t *)htm_thr;
	if(NULL == tm_thr || NULL == tm_thr->htmhp)
		return NULL;

	//没有任何定时器节点,无法重置定时器
	if(MyTimerHeapGetCount(tm_thr->htmhp) == 0)
		return NULL;

	gettimeofday((struct timeval *)&node->abs_expire, NULL);
	timeval_add(node->abs_expire, node->first_expire);

	timer_id = MyTimerHeapReset(tm_thr->htmhp, timer_id, node);

	if(NULL == timer_id)
		return NULL;

	//如果发现 <新的超时> 比 <当前最短的超时> 还要短,表示需要唤醒定时器线程
	if(MyTimeHeapGetEarliestKey(tm_thr->htmhp) == timer_id)
		MyEventSetSignaled(tm_thr->wait_evt);

	return timer_id;
}

/*
*
*获取定时器的个数
*
*/
int MyTimerThreadGetTimerCount(HMYTIMER_THREAD htm_thr)
{
	mytimer_thread_t * tm_thr = (mytimer_thread_t *)htm_thr;
	if(NULL == tm_thr || NULL == tm_thr->htmhp)
		return -1;

	return (int)MyTimerHeapGetCount(tm_thr->htmhp);
}

/*
*
*获取定时器的个数
*
*/
void MyTimerThreadPrint(HMYTIMER_THREAD htm_thr)
{
	mytimer_thread_t * tm_thr = (mytimer_thread_t *)htm_thr;
	MyTimerHeapPrint(tm_thr->htmhp);
}






















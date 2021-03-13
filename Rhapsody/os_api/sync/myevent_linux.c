/*
*
*myevent_linux.c 条件(cond)/事件(event) lin shao chuan
*
* change log:
* <lsccsl@tom.com> 2008-09-17
*	增加了引用计数与占有锁的线程记录,同一个线程可以多次获取锁
*/


#include "myevent.h"

#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

#include "mylog.h"


typedef struct __myevent_t_
{
	HMYMEMPOOL hm;
	
	pthread_cond_t cond;
	pthread_mutex_t mtx;
}myevent_t;


/*
*
*创建事件/条件
*
*/
HMYEVENT MyEventRealConstruct(HMYMEMPOOL hm, int bNotAutoReset, const char * pcname)
{
	int ret = 0;
	int ret1 = 0;
	myevent_t * me = (myevent_t *)MyMemPoolMalloc(hm, sizeof(*me));
	if(NULL == me)
		return NULL;

	me->hm = hm;
	ret = pthread_cond_init(&me->cond, NULL);
	ret1 = pthread_mutex_init(&me->mtx, NULL);
	
	if(0 != ret || 0 != ret1)
	{
		LOG_WARN(("fail create event"));

		MyMemPoolFree(hm, me);
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
	struct timespec abstime = {0};
	myevent_t * me = (myevent_t *)he;
	if(NULL == me)
		return;

	if(ETIMEDOUT == pthread_cond_timedwait(&me->cond, &me->mtx, &abstime))
		pthread_cond_signal(&me->cond);

	pthread_cond_destroy(&me->cond);
	pthread_mutex_destroy(&me->mtx);

	MyMemPoolFree(me->hm, me);
}

/*
*
*把事件设置成signaled状态
*
*/
void MyEventSetSignaled(HMYEVENT he)
{
	myevent_t * me = (myevent_t *)he;
	if(NULL == me)
		return;

	pthread_cond_signal(&me->cond);
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

	pthread_cond_broadcast(&me->cond);
}

/*
*
*把事件设置成非signaled状态
*
*/
void MyEventSetNoSignaled(HMYEVENT he)
{}

/*
*
* 等待事件发生, 
*
*param millsecond:单位毫秒, -1表示无限等待,直至事件发生
*/
int MyEventWait(HMYEVENT he, mytv_t * timeout)
{
	/*
	* The  pthread_cond_timedwait()  and  pthread_cond_wait()  functions  shall block on a condition variable. They shall be called with mutex locked by the calling thread or
    * undefined behavior results.
	*
	* These functions atomically release mutex and cause the calling thread to block on the condition variable cond; atomically here means "atomically with respect to  access
	* by  another  thread  to the mutex and then the condition variable". That is, if another thread is able to acquire the mutex after the about-to-block thread has released
	* it, then a subsequent call to pthread_cond_broadcast() or pthread_cond_signal() in that thread shall behave as if it were issued after  the  about-to-block  thread  has
	* blocked.
	*
	* Upon successful return, the mutex shall have been locked and shall be owned by the calling thread.
	*
	* The pthread_cond_timedwait() function allows an application to give up waiting for a particular condition after a given amount of time. An example of its use follows:
	*
	*	(void) pthread_mutex_lock(&t.mn);
	*		t.waiters++;
	*		clock_gettime(CLOCK_REALTIME, &ts);
	*		ts.tv_sec += 5;
	*		rc = 0;
	*		while (! mypredicate(&t) && rc == 0)
	*			rc = pthread_cond_timedwait(&t.cond, &t.mn, &ts);
	*			t.waiters--;
	*		if (rc == 0) setmystate(&t);
	*	(void) pthread_mutex_unlock(&t.mn);
	*
	*          pthread_cond_wait(mutex, cond):
	*             value = cond->value; // 1 
	*             pthread_mutex_unlock(mutex); // 2 
	*             pthread_mutex_lock(cond->mutex); // 10 
	*             if (value == cond->value) { // 11 
	*                 me->next_cond = cond->waiter;
	*                 cond->waiter = me;
	*                 pthread_mutex_unlock(cond->mutex);
	*                 unable_to_run(me);
	*             } else
	*                 pthread_mutex_unlock(cond->mutex); // 12 
	*             pthread_mutex_lock(mutex); // 13 
	*
	*         pthread_cond_signal(cond):
	*             pthread_mutex_lock(cond->mutex); // 3 
	*             cond->value++; // 4 
	*             if (cond->waiter) { // 5 
	*                 sleeper = cond->waiter; // 6 
	*                 cond->waiter = sleeper->next_cond; // 7 
	*                 able_to_run(sleeper); // 8 
	*             }
	*             pthread_mutex_unlock(cond->mutex); // 9 
	*/

	int ret = 0;
	myevent_t * me = (myevent_t *)he;
	if(NULL == me)
		return -1;

	if(NULL == timeout)
	{
		pthread_mutex_lock(&me->mtx);
		ret = pthread_cond_wait(&me->cond, &me->mtx);
		pthread_mutex_unlock(&me->mtx);
	}
	else
	{
		struct timespec abstime = {0};
		struct timeval now = {0};
		gettimeofday(&now, NULL);

		abstime.tv_nsec = (now.tv_usec + timeout->tv_usec) * 1000;
		abstime.tv_sec = now.tv_sec + timeout->tv_sec;

		if(abstime.tv_nsec > 1000 * 1000 * 1000)
		{
			abstime.tv_sec += 1;
			abstime.tv_nsec = abstime.tv_nsec % (1000 * 1000 * 1000);
		}

		pthread_mutex_lock(&me->mtx);
		ret = pthread_cond_timedwait(&me->cond, &me->mtx,
			&abstime);
		pthread_mutex_unlock(&me->mtx);
	}
	
	return ret;
}


















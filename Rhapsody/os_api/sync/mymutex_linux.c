/*
*
*mymutex_linux.c 互斥锁 lin shao chuan
*
*/


#include "mymutex.h"

#include <pthread.h>

#include "mylog.h"


typedef struct __mymutex_t_
{
	pthread_mutex_t mtx;
	HMYMEMPOOL hm;

	/* 持锁的线程,以及该线程对锁的引用计数 */
	volatile pthread_t thr_owner;
	volatile int ref_count;
}mymutex_t;


/*
*
*创建互斥锁
*
*/
HMYMUTEX MyMutexRealConstruct(HMYMEMPOOL hm, const char * pcname)
{
	int ret = 0;
	mymutex_t * mt = (mymutex_t *)MyMemPoolMalloc(hm, sizeof(*mt));
	if(NULL == mt)
		return NULL;

	mt->hm = hm;
	mt->thr_owner = 0;
	mt->ref_count = 0;

	ret = pthread_mutex_init(&mt->mtx, NULL);
	if(0 != ret)
	{
		LOG_WARN(("fail create mutex"));

		MyMemPoolFree(hm, mt);
		return NULL;
	}

	return (HMYMUTEX)mt;
}

/*
*
*锁毁互斥锁
*
*/
void MyMutexDestruct(HMYMUTEX hmx)
{
	mymutex_t * mt = (mymutex_t *)hmx;
	if(NULL == mt)
		return;

	pthread_mutex_destroy(&mt->mtx);
	MyMemPoolFree(mt->hm, mt);
}

/*
*
*加锁 0:成功, 非0:失败
*
*/
int MyMutexLock(HMYMUTEX hmx)
{
	mymutex_t * mt = (mymutex_t *)hmx;
	if(NULL == mt)
		return -1;

	//LOG_DEBUG(("lock mutex:%x, thread:%d", mt, pthread_self()));

	if(0 == mt->thr_owner || pthread_self() != mt->thr_owner)
	{
		int ret = -1;

		//if(0 == mt->thr_owner)
		//	LOG_DEBUG(("mutex %x first taken by %dd", mt, pthread_self()));

		ret = pthread_mutex_lock(&mt->mtx);
		if(0 == ret)
		{
			assert(0 == mt->ref_count);
			assert(0 == mt->thr_owner);

			mt->thr_owner = pthread_self();
			mt->ref_count ++;

			//LOG_DEBUG(("mutex %x has not been taken, take it thr_owner:%d - %d, ref_count:%d", 
			//	mt, mt->thr_owner, pthread_self(), mt->ref_count));

			return 0;
		}

		return -1;
	}
	else
	{
		assert(0 != mt->ref_count);
		mt->ref_count ++;

		LOG_DEBUG(("mutex %x has been taken by : %d, current taker : %d, don't take it ref_count:%d", mt, 
			mt->thr_owner, pthread_self(), mt->ref_count));

		return 0;
	}
}

int MyMutexTryLock(HMYMUTEX hmx)
{
	mymutex_t * mt = (mymutex_t *)hmx;
	if(NULL == mt)
		return -1;

	return pthread_mutex_trylock(&mt->mtx);
}

/*
*
*解锁
*
*/
int MyMutexUnLock(HMYMUTEX hmx)
{
	mymutex_t * mt = (mymutex_t *)hmx;
	if(NULL == mt)
		return -1;

	//LOG_DEBUG(("unlock mutex:%x, thread:%d", mt, pthread_self()));

	if(0 == mt->thr_owner)
	{
		LOG_WARN(("mutex %x has not been taken", mt));
		return 0;
	}
	else if(pthread_self() != mt->thr_owner)
	{
		LOG_WARN(("mutex %x thr_owner : %d, current thread : %d, unlocking can't be done", 
			mt,  mt->thr_owner, pthread_self()));
		return -1;
	}
	else
	{
		mt->ref_count --;

		//LOG_DEBUG(("mutex %x thr_owner : %d, current thread : %d, unlocking can be done, ref_count:%d", 
		//	mt,  mt->thr_owner, pthread_self(), mt->ref_count));

		if(0 == mt->ref_count)
		{
			mt->thr_owner = 0;
			return pthread_mutex_unlock(&mt->mtx);
		}

		return 0;
	}
}





















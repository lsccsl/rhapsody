/*
*
*mythread_win32.c 封装线程接口 lin shao chuan
*
*/
#include "mythread.h"

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

#include "mylog.h"


typedef struct __mythread_t_
{
	//本对象内存池标识
	HMYMEMPOOL hm;

	//起始地址与用户参数
	MY_THREAD_FUN fun;
	void * userdata;

	//线程id与句柄
	volatile pthread_t thr;
	volatile int run;

	//是否挂起创建
	int bsuspend;
	sem_t s;

	/* 通知构造返回的信号量 */
	sem_t s_ret;
}mythread_t;


/**
 * @brief 线程退出处理
 */
static void  thread_exit(void * param)
{
	mythread_t * mt = (mythread_t *)param;
	assert(mt);
	
	LOG_DEBUG(("thread_exit %x, %d", mt, mt->thr));
}

static void * threadfun(void * data)
{
	mythread_t * mt = (mythread_t *)data;
	assert(mt && mt->fun);

	mt->thr = pthread_self();
	LOG_INFO(("new thread %d %d %x", pthread_self(), mt->thr, mt));
	sem_post(&mt->s_ret);

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	//压入释放操作,不允许cancel线程
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);	
	pthread_cleanup_push(thread_exit, mt);	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	if(mt->bsuspend)
	{
		LOG_INFO(("suspend %d", pthread_self()));
		sem_wait(&mt->s);
	}

	mt->fun(mt->userdata);

	pthread_cleanup_pop(1);

	return NULL;
}


/*
*
*创建线程
*
*/
HMYTHREAD MyThreadConstruct(MY_THREAD_FUN fun, void * data, int bsuspend, HMYMEMPOOL hm)
{
	pthread_t pt;
	mythread_t * mt = (mythread_t *)MyMemPoolMalloc(hm, sizeof(*mt));
	if(NULL == mt)
		return NULL;

	assert(fun);

	mt->fun = fun;
	mt->userdata = data;
	mt->hm = hm;
	mt->run = 0;
	mt->thr = 0;
	mt->bsuspend = bsuspend;

	if(bsuspend)
		sem_init(&mt->s, 0, 0);
	
	sem_init(&mt->s_ret, 0, 0);

	//是否挂起创建
	if(0 != pthread_create(&pt, NULL, threadfun, mt))
	{
		LOG_WARN(("pthread_create fail"));
		
		if(mt->bsuspend)
			sem_destroy(&mt->s);
		
		sem_destroy(&mt->s_ret);
		MyMemPoolFree(mt->hm, mt);
		return NULL;
	}

	sem_wait(&mt->s_ret);
	sem_destroy(&mt->s_ret);

	LOG_DEBUG(("MyThreadConstruct:%d, %x", mt->thr, mt));

	return (HMYTHREAD)mt;
}

/*
*
*销毁线程
*
*/
void MyThreadDestruct(HMYTHREAD ht)
{
	mythread_t * mt = (mythread_t *)ht;
	if(NULL == mt)
		return;

	LOG_DEBUG(("MyThreadDestruct:%d %x", mt->thr, mt));

	if(mt->thr)
	{
		pthread_cancel(mt->thr);
		pthread_join(mt->thr, NULL);
	}

	if((!mt->run) && mt->bsuspend)
		sem_destroy(&mt->s);

	MyMemPoolFree(mt->hm, mt);
}

/*
*
*运行线程
*
*/
void MyThreadRun(HMYTHREAD ht)
{
	mythread_t * mt = (mythread_t *)ht;	
	if(NULL == mt)
		return;

	LOG_INFO(("thread:%x %d run:%d %d", mt, mt->thr, mt->run, mt->bsuspend));

	if(mt->run)
		return;

	mt->run = 1;
	if(mt->bsuspend)
	{
		sem_post(&mt->s);
		sem_destroy(&mt->s);
	}
}

/*
*
*停止线程运行
*
*/
void MyThreadSuspend(HMYTHREAD ht)
{
	LOG_INFO(("no support"));
}

/*
*
*等待线程退出
*
*/
void MyThreadJoin(HMYTHREAD ht)
{
	mythread_t * mt = (mythread_t *)ht;
	if(NULL == mt)
		return;

	LOG_DEBUG(("MyThreadJoin:%d", mt->thr));

	if(mt->thr)
		pthread_join(mt->thr, NULL);

	mt->thr = 0;
}

/*
*
* 判断呼叫此函数的线程是否就是ht本身
* 1:表示是, 0:表示否
*
*/
int MyThreadInMyContext(HMYTHREAD ht)
{
	pthread_t temp;
	mythread_t * mt = (mythread_t *)ht;
	if(NULL == mt)
		return 0;

	temp = pthread_self();

	LOG_DEBUG(("MyThreadInMyContext:%d - %d %x", temp, mt->thr, mt));
	return ((temp == mt->thr) ? 1 : 0);
}











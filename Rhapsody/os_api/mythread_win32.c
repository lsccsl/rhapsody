/*
*
*mythread_win32.c 封装线程接口 lin shao chuan
*
*/
#include "mythread.h"

#include <assert.h>
#include <process.h>
#include <windows.h>

#include "mylog.h"


enum THREAD_EXCEPTION
{
	THREAD_EC_CANCEL,
};


typedef struct __mythread_t_
{
	//本对象内存池标识
	HMYMEMPOOL hm;

	//起始地址与用户参数
	MY_THREAD_FUN fun;
	void * userdata;

	//线程id与句柄
	DWORD thr_id;
	HANDLE hthr;
}mythread_t;


static int
ExceptionFilter (EXCEPTION_POINTERS * ep)
{
	return EXCEPTION_EXECUTE_HANDLER;
}

static int __stdcall threadfun(void * data)
{
	//运行用户的线程函数;
	//利用捕捉异常机制自然退出线程

	mythread_t * mt = (mythread_t *)data;
	assert(mt && mt->fun);

	__try
	{
		mt->fun(mt->userdata);
	}
	__except( ExceptionFilter(GetExceptionInformation()) )
	{}

	return 0;
}

static void throw_exception(DWORD ec, DWORD p)
{
	DWORD exceptionInformation = (DWORD) (p);
# pragma warning (disable:4133)
	RaiseException ( ec, 0, 1, &exceptionInformation );
}

static void throw_exception_cancel()
{
	//c代码,ExitThread是一个安全的函数, 如果c++将会导致析构函数无法调用
	ExitThread(0);
	//throw_exception(THREAD_EC_CANCEL, 0);

	//never reach
	assert(0);
}

static void mythread_cancel(mythread_t * mt)
{
	//挂起线程
	SuspendThread(mt->hthr);

	{
		//from pthread_win32
		CONTEXT context;
		context.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext( mt->hthr, &context );
# pragma warning (disable:4311)
		context.Eip = (DWORD)throw_exception_cancel;
		SetThreadContext( mt->hthr, &context );
	}

	ResumeThread(mt->hthr);
}


/*
*
*创建线程
*
*/
HMYTHREAD MyThreadConstruct(MY_THREAD_FUN fun, void * data, int bsuspend, HMYMEMPOOL hm)
{
	mythread_t * mt = MyMemPoolMalloc(hm, sizeof(*mt));
	if(NULL == mt)
		return NULL;

	mt->fun = fun;
	mt->userdata = data;
	mt->hm = hm;
	//挂起创建
	mt->hthr = (HANDLE)_beginthreadex(NULL, 0, threadfun, (void *)mt, CREATE_SUSPENDED, &mt->thr_id);

	if(NULL == mt->hthr)
	{
		LOG_WARN(("fail create thread"));

		MyMemPoolFree(hm, mt);
		return NULL;
	}

	if(!bsuspend)
		ResumeThread(mt->hthr);

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

	assert(mt->hthr);

	//利用异常自然退出线程
	mythread_cancel(mt);

	CloseHandle(mt->hthr);

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

	assert(mt->hthr);

	ResumeThread(mt->hthr);
}

/*
*
*线程挂起
*
*/
void MyThreadSuspend(HMYTHREAD ht)
{
	mythread_t * mt = (mythread_t *)ht;
	if(NULL == mt)
		return;

	assert(mt->hthr);

	SuspendThread(mt->hthr);
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

	assert(mt->hthr);

	WaitForSingleObject(mt->hthr, INFINITE);
}

/*
*
* 判断呼叫此函数的线程是否就是ht本身
* 1:表示是, 0:表示否
*
*/
int MyThreadInMyContext(HMYTHREAD ht)
{
	unsigned long temp = 0;

	mythread_t * mt = (mythread_t *)ht;
	if(NULL == mt)
		return 0;

	temp = GetCurrentThreadId();

	return ((temp == mt->thr_id) ? 1 : 0);
}



















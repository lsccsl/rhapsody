/*
*
*mythread_win32.c ��װ�߳̽ӿ� lin shao chuan
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
	//�������ڴ�ر�ʶ
	HMYMEMPOOL hm;

	//��ʼ��ַ���û�����
	MY_THREAD_FUN fun;
	void * userdata;

	//�߳�id����
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
	//�����û����̺߳���;
	//���ò�׽�쳣������Ȼ�˳��߳�

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
	//c����,ExitThread��һ����ȫ�ĺ���, ���c++���ᵼ�����������޷�����
	ExitThread(0);
	//throw_exception(THREAD_EC_CANCEL, 0);

	//never reach
	assert(0);
}

static void mythread_cancel(mythread_t * mt)
{
	//�����߳�
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
*�����߳�
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
	//���𴴽�
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
*�����߳�
*
*/
void MyThreadDestruct(HMYTHREAD ht)
{
	mythread_t * mt = (mythread_t *)ht;
	if(NULL == mt)
		return;

	assert(mt->hthr);

	//�����쳣��Ȼ�˳��߳�
	mythread_cancel(mt);

	CloseHandle(mt->hthr);

	MyMemPoolFree(mt->hm, mt);
}

/*
*
*�����߳�
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
*�̹߳���
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
*�ȴ��߳��˳�
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
* �жϺ��д˺������߳��Ƿ����ht����
* 1:��ʾ��, 0:��ʾ��
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



















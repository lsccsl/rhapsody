/*
*
*myevent_win32.c 条件(cond)/事件(event) lin shao chuan
*
* change log:
* <lsccsl@tom.com> 2008-09-17
*	增加了引用计数与占有锁的线程记录,同一个线程可以多次获取锁
*/


#include "__event_win32.h"
#include <assert.h>
#include <windows.h>
#include "mylog.h"


typedef struct __myevent_t_
{
	HMYMEMPOOL hm;
	HANDLE hevt;
}myevent_t;


/*
*
*创建事件/条件
*
*/
HMYEVENT MyEventRealConstruct(HMYMEMPOOL hm, int bNotAutoReset, const char * pcname)
{
	myevent_t * me = MyMemPoolMalloc(hm, sizeof(*me));
	if(NULL == me)
		return NULL;

	if(pcname)
	{
		me->hevt = OpenEvent(EVENT_ALL_ACCESS, FALSE, pcname);
		if(NULL == me->hevt)
			me->hevt = CreateEvent(NULL, bNotAutoReset, FALSE, pcname);
	}
	else
		me->hevt = CreateEvent(NULL, bNotAutoReset, FALSE, NULL);
	me->hm = hm;
	if(NULL == me->hevt)
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
	myevent_t * me = (myevent_t *)he;
	if(NULL == me)
		return;

	assert(me->hevt);

	if(WaitForSingleObject(me->hevt, 0) == WAIT_TIMEOUT)
		SetEvent(me->hevt);

	CloseHandle(me->hevt);

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
	if(NULL == me || NULL == me->hevt)
		return;

	SetEvent(me->hevt);
}

/*
*
*把事件设置成非signaled状态
*
*/
void MyEventSetNoSignaled(HMYEVENT he)
{
	myevent_t * me = (myevent_t *)he;
	if(NULL == me || NULL == me->hevt)
		return;

	ResetEvent(me->hevt);
}

/*
*
* 等待事件发生, 
*
*param millsecond:单位毫秒, -1表示无限等待,直至事件发生
*/
int MyEventWait(HMYEVENT he, mytv_t * timeout)
{
	DWORD ret = 0;
	myevent_t * me = (myevent_t *)he;
	if(NULL == me || NULL == me->hevt)
		return -1;

	if(NULL == timeout)
		ret = WaitForSingleObject(me->hevt, INFINITE);
	else
		ret = WaitForSingleObject(me->hevt, timeout->tv_sec * 1000 + timeout->tv_usec/1000);

	if(WAIT_OBJECT_0 == ret)
		return 0;
	else
		return -1;
}


/*
*
*广播事件发生
*
*/
void MyEventBroadCast(HMYEVENT he)
{
	myevent_t * me = (myevent_t *)he;
	if(NULL == me || NULL == me->hevt)
		return;

	PulseEvent(me->hevt);
}


/**
 * @brief 获取句柄的句柄
 */
int MyEventWin32GetHandle(HMYEVENT he)
{
	myevent_t * me = (myevent_t *)he;
	if(NULL == me)
		return -1;

#pragma warning(disable:4311)
	return (int)me->hevt;
}

















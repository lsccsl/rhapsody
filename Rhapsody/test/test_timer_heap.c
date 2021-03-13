#ifdef __cplusplus
extern "C"
{
#endif
#include "mylog.h"
#include "mytimerheap.h"
#include "mytimer_thread.h"
#include "myevent.h"
#ifdef __cplusplus
}
#endif

#include <time.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) sleep(x/1000)
#endif
#include "gettimeofday.h"

HTIMERID timer_id_period = NULL;

static int test_timer_heap_cb(unsigned long context_data, 
							  unsigned long timer_user_data,
							  void * timerid)
{
	HMYTIMERHEAP hthp = (HMYTIMERHEAP)timer_user_data;

	LOG_INFO(("context:%d %x", 
		context_data, hthp));

	if(context_data == 10)
	{
		//mytimer_node_t node = {0};
		//node.timeout_cb = test_timer_heap_cb;
		//struct timeval tv = {0};
		//struct timeval tv_period = {0};
		//gettimeofday(&tv, NULL);
		//tv.tv_sec += 5;
		//tv.tv_usec += 0;
		//tv_period.tv_sec = 5;
		//tv_period.tv_usec = 0;
		//MyTimerHeapReset(hthp, timer_id_period, &tv, &tv_period, 5, hthp);
	}

	return 0;
}

static int test_timer_thread_cb(unsigned long context_data, 
							  unsigned long timer_user_data,
							  void * timerid)
{
	HMYTIMERHEAP hthp = (HMYTIMERHEAP)timer_user_data;

	struct timeval now = {0};
	gettimeofday(&now, NULL);
		
	LOG_INFO(("context:%d timer:%d %d %d", 
		context_data, timer_user_data, now.tv_sec, now.tv_usec));

	return 0;
}
/*static void
ExceptionFilter (EXCEPTION_POINTERS * ep)
{}*/
void test_timer_heap()
{
	HMYEVENT he = MyEventConstruct(NULL);
	HMYTIMERHEAP hthp = MyTimerHeapConstruct(NULL);
	HTIMERID timer_id = NULL;

	//{
	//	tv.tv_sec = 0;
	//	tv.tv_usec = 5000;
	//	MyTimerHeapAdd(hthp, &tv, NULL, 0, 500);
	//}

	{
		mytimer_node_t node = {0};
		node.timeout_cb = test_timer_heap_cb;
		gettimeofday((struct timeval *)&node.abs_expire, NULL);

		node.context_data = 1;
		node.timer_user_data = (unsigned long)hthp;
		node.abs_expire.tv_sec += 1;

		MyTimerHeapAdd(hthp, &node);
	}

	{
		mytimer_node_t node = {0};
		node.timeout_cb = test_timer_heap_cb;
		gettimeofday((struct timeval *)&node.abs_expire, NULL);

		node.context_data = 1;
		node.timer_user_data = (unsigned long)hthp;
		node.abs_expire.tv_sec += 1;

		MyTimerHeapAdd(hthp, &node);
	}

	{
		mytimer_node_t node = {0};
		node.timeout_cb = test_timer_heap_cb;
		gettimeofday((struct timeval *)&node.abs_expire, NULL);

		node.context_data = 2;
		node.timer_user_data = (unsigned long)hthp;
		node.abs_expire.tv_sec += 2;

		MyTimerHeapAdd(hthp, &node);
	}

	{
		mytimer_node_t node = {0};
		node.timeout_cb = test_timer_heap_cb;
		gettimeofday((struct timeval *)&node.abs_expire, NULL);

		node.context_data = 3;
		node.timer_user_data = (unsigned long)hthp;
		node.abs_expire.tv_sec += 3;

		timer_id = MyTimerHeapAdd(hthp, &node);
	}

	{
		mytimer_node_t node = {0};
		node.timeout_cb = test_timer_heap_cb;
		gettimeofday((struct timeval *)&node.abs_expire, NULL);

		node.context_data = 4;
		node.timer_user_data =(unsigned long) hthp;
		node.abs_expire.tv_sec += 4;

		MyTimerHeapAdd(hthp, &node);
	}

	{
		mytimer_node_t node = {0};
		node.timeout_cb = test_timer_heap_cb;
		gettimeofday((struct timeval *)&node.abs_expire, NULL);

		node.context_data = 10;
		node.timer_user_data = (unsigned long)hthp;
		node.abs_expire.tv_sec += 10;

		MyTimerHeapAdd(hthp, &node);
	}

	LOG_INFO(("run expire end timer:%d %x", 
		MyTimerHeapGetCount(hthp), hthp));

	for(;;)
	{
		struct timeval tv_out = {0};
		struct timeval tv_now = {0};
		int ret = 0;
		gettimeofday((struct timeval *)&tv_now, NULL);
		ret = MyTimerHeapGetEarliestExpire(hthp, (mytv_t *)&tv_out);
		
		if(timeval_smaller(tv_now, tv_out))
		{
			timeval_minus(tv_out, tv_now);			
			MyEventWait(he, (mytv_t *)&tv_out);
		}

		gettimeofday(&tv_now, NULL);
		MyTimerHeapRunExpire(hthp, (mytv_t *)&tv_now);
		MyTimerHeapDel(hthp, timer_id);

		if(-1 == ret)
			break;
	}

	MyMemPoolMemReport(0);

	MyTimerHeapDestruct(hthp);

	MyEventDestruct(he);

	MyMemPoolMemReport(1);
}

void test_timer_thread()
{
	HMYTIMER_THREAD htmthr = MyTimerThreadConstruct(NULL);
	
	MyTimerThreadRun(htmthr);
	
	{
		HTIMERID timerid = NULL;

		mytimer_node_t node = {0};
		node.timeout_cb = test_timer_thread_cb;

		node.context_data = 1231;
		node.timer_user_data = 1231;
		node.first_expire.tv_sec = 3;
		node.first_expire.tv_usec = 1000 * 500;
		node.period.tv_sec = 3;
		node.period.tv_usec = 1000 * 500;
		
		{
	struct timeval now = {0};
	gettimeofday(&now, NULL);
		
	LOG_INFO((" %d %d", 
		now.tv_sec, now.tv_usec));
			}
		timerid = MyTimerThreadAddTimer(htmthr, &node);
		LOG_INFO(("add timer %d %d", 
			node.first_expire.tv_sec, node.first_expire.tv_usec));

		Sleep(4 * 1000);

		node.context_data = 1232;
		node.timer_user_data = 1232;
		node.first_expire.tv_sec = 1;
		node.first_expire.tv_usec = 0;
		node.period.tv_sec = 1;
		node.period.tv_usec = 0;

		LOG_INFO(("reset timer %d %d", 
			node.first_expire.tv_sec, node.first_expire.tv_usec));
		timerid = MyTimerThreadResetTimer(htmthr, timerid, &node);

		Sleep(1000);
		node.context_data = 1235;
		node.timer_user_data = 1235;
		node.first_expire.tv_sec = 1;
		node.first_expire.tv_usec = 0;
		node.period.tv_sec = 0;
		node.period.tv_usec = 0;

		LOG_INFO(("add timer %d %d", 
			node.first_expire.tv_sec, node.first_expire.tv_usec));
		MyTimerThreadAddTimer(htmthr, &node);

		Sleep(20 * 1000);

		LOG_INFO(("del timer"));
		MyTimerThreadDelTimer(htmthr, timerid);
	}

	Sleep(12 * 1000);

	MyMemPoolMemReport(0);

	MyTimerThreadDestruct(htmthr);

	MyMemPoolMemReport(1);

}


static HTIMERID htimerid_test = NULL;
static HMYTIMER_THREAD htimer_test = NULL;

static int test_timer_cb(unsigned long timer_data, unsigned long context_data, void * timerid)
{
	printf("time out\r\n");
	MyTimerThreadDelTimer(htimer_test, htimerid_test);

	{
		mytimer_node_t node = {0};

		MyTimerThreadRun(htimer_test);

		node.timeout_cb = test_timer_cb;	
		node.context_data = 0;
		node.timer_user_data = 0;
		node.first_expire.tv_sec = 1;
		node.period.tv_sec = 1;
		
		MyTimerThreadAddTimer(htimer_test, &node);
	}

	return 0;
}

void test_timer_thread_del()
{
	mytimer_node_t node = {0};

	htimer_test = MyTimerThreadConstruct(NULL);
	MyTimerThreadRun(htimer_test);

	node.timeout_cb = test_timer_cb;	
	node.context_data = 0;
	node.timer_user_data = 0;
	node.first_expire.tv_sec = 1;
	node.period.tv_sec = 1;

	htimerid_test = MyTimerThreadAddTimer(htimer_test, &node);

	Sleep(3000);

	htimerid_test = MyTimerThreadAddTimer(htimer_test, &node);

	Sleep(3000);

	MyTimerThreadDestruct(htimer_test);

	MyMemPoolMemReport(1);
}



















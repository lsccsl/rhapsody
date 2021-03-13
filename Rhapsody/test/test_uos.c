#include "mypipe.h"
#include "mylog.h"
#include "mynamepipe.h"
#include "mysock.h"
#include "mythread.h"
#include "myevent.h"
#include "mymutex.h"
#include "mysem.h"
#include "mymmap.h"
#include "myprocess.h"

#ifdef WIN32
	#include <windows.h>
#else
	#include <pthread.h>
#endif


static void test_pipe()
{
	char actemp[32] = {0};
	int ret = 0;
	HMYPIPE hp = MyPipeConstruct(NULL);

	MyPipeWrite(hp, "abc", 3);
	LOG_DEBUG(("write abc to pipe"));

	ret = MyPipeRead(hp, actemp, sizeof(actemp));
	LOG_DEBUG(("read %s %d", actemp, ret));

	MyPipeDestruct(hp);

	MyMemPoolMemReport(1);
}


static void test_named_pipe()
{
	char actemp[64] = {0};
	int ret = 0;

	HMYNAMEPIPE hnmp = MyNamePipeConstruct(NULL, "test", 1);
	HMYNAMEPIPE hnmp1 = MyNamePipeConstruct(NULL, "test", 0);

	MyNamePipeWrite(hnmp1, "client -> srv", strlen("client -> srv"));
	ret = MyNamePipeRead(hnmp, actemp, sizeof(actemp));
	LOG_DEBUG(("client read [%s] [%d]", actemp, ret));

	MyNamePipeWrite(hnmp1, "client -> client", strlen("client -> client"));
	ret = MyNamePipeRead(hnmp, actemp, sizeof(actemp));
	LOG_DEBUG(("client read [%s] [%d]", actemp, ret));

	MyNamePipeWrite(hnmp, "srv -> client", strlen("srv -> client"));
	ret = MyNamePipeRead(hnmp1, actemp, sizeof(actemp));
	LOG_DEBUG(("client read [%s] [%d]", actemp, ret));

	//MyNamePipeWrite(hnmp, "srv -> srv", strlen("srv -> srv"));
	//ret = MyNamePipeRead(hnmp, actemp, sizeof(actemp));
	//LOG_DEBUG(("self read %s %d", actemp, ret));

	MyNamePipeDestruct(hnmp);
	MyNamePipeDestruct(hnmp1);

	MyMemPoolMemReport(1);
}


static void test_udp()
{
	int ret = 0;
	char actemp[32] = {0};
	char ip[32] = {0};
	unsigned short port = 0;
	HUOSUDP hudp = NULL;
	HUOSUDP hudp1 = NULL;
	port = 10000;

	LOG_DEBUG(("test sock, make sure your system support it"));

	hudp = UOS_UDPConstruct(NULL, "0.0.0.0", &port); 
	port = 10001;
	hudp1 = UOS_UDPConstruct(NULL, "0.0.0.0", &port);

	port = 0;
#ifdef WIN32
	UOS_UDPWriteTo(hudp, "abc", 3, "192.168.35.85", 10001);
#else
	UOS_UDPWriteTo(hudp, "abc", 3, "0.0.0.0", 10001);
#endif
	ret = UOS_UDPReadFrom(hudp1, actemp, sizeof(actemp), ip, sizeof(ip), &port);
	LOG_DEBUG(("read from [%s:%d] [%s] [%d]", ip, port, actemp, ret));

	UOS_UDPDestruct(hudp);
	UOS_UDPDestruct(hudp1);
	MyMemPoolMemReport(1);
}

void * tcp_accept_thread_fun(void * param)
{
	char acip[32] = {0};
	char acbuf[32] = {0};
	unsigned short port = 10000;
	HUOSTCP htcp_accept = NULL;
#ifdef WIN32
	HUOSTCP htcp = UOS_TCPConstruct(NULL, "192.168.35.85", &port);
#else
	HUOSTCP htcp = UOS_TCPConstruct(NULL, "0.0.0.0", &port);
#endif

	UOS_TCPListerner(htcp);

	htcp_accept = UOS_TCPAccept(htcp, acip, sizeof(acip) - 1, &port);
	LOG_INFO(("recv connect from %s:%d", acip, port));

	UOS_TCPRead(htcp_accept, acbuf, sizeof(acbuf));
	LOG_INFO(("recv data %s", acbuf));

	UOS_TCPWrite(htcp_accept, "cba", strlen("cba"));

	os_msleep(3000);
	UOS_TCPDestruct(htcp);
	UOS_TCPDestruct(htcp_accept);
}

void * tcp_connect_thread_fun(void * param)
{
	char acbuf[32] = {0};
	unsigned short port = 10001;
#ifdef WIN32
	HUOSTCP htcp = UOS_TCPConstruct(NULL, "192.168.35.85", &port);
#else
	HUOSTCP htcp = UOS_TCPConstruct(NULL, "0.0.0.0", &port);
#endif

#ifdef WIN32
	UOS_TCPConnect(htcp, "192.168.35.85", 10000);
#else
	UOS_TCPConnect(htcp, "0.0.0.0", 10000);
#endif

	UOS_TCPWrite(htcp, "abc", strlen("abc"));

	UOS_TCPRead(htcp, acbuf, sizeof(acbuf));
	LOG_INFO(("recv data %s", acbuf));

	os_msleep(3000);
	UOS_TCPDestruct(htcp);
}

static void test_tcp()
{
	HMYTHREAD hthrd1 = MyThreadConstruct(tcp_accept_thread_fun, NULL, 0, NULL);
	HMYTHREAD hthrd2 = MyThreadConstruct(tcp_connect_thread_fun, NULL, 1, NULL);

	MyThreadRun(hthrd2);

	MyThreadJoin(hthrd1);
	MyThreadJoin(hthrd2);

	MyThreadDestruct(hthrd1);
	MyThreadDestruct(hthrd2);

	MyMemPoolMemReport(1);
}

static void test_sock()
{
	test_tcp();
	test_udp();
}


typedef struct __test_thread_t_
{
	HMYTHREAD hthrd1;
	HMYTHREAD hthrd2;
	HMYTHREAD hthrd3;
}test_thread_t;

static void * test_thread_fun(void * data)
{
	test_thread_t * t = (test_thread_t *)data;
	int i = 0;
	HMYTHREAD hthrd = NULL;

	if(MyThreadInMyContext(t->hthrd1))
		hthrd = t->hthrd1;
	else if(MyThreadInMyContext(t->hthrd2))
		hthrd = t->hthrd2;
	else if(MyThreadInMyContext(t->hthrd3))
		hthrd = t->hthrd3;
	else
	{
		LOG_WARN(("who i am?"));
		exit(0);
	}

	for(i = 0; i < 10; i ++)
	{
		printf("[%s:%d]%x\r\n", __FILE__, __LINE__, hthrd);
		os_msleep(1000);
	}

	return NULL;
}

static void test_thread()
{
	test_thread_t t = {0};

	t.hthrd1 = MyThreadConstruct(test_thread_fun, &t, 1, NULL);
	t.hthrd2 = MyThreadConstruct(test_thread_fun, &t, 1, NULL);
	t.hthrd3 = MyThreadConstruct(test_thread_fun, &t, 1, NULL);

	MyThreadRun(t.hthrd1);
	MyThreadRun(t.hthrd2);
	MyThreadRun(t.hthrd3);

	MyThreadJoin(t.hthrd1);
	MyThreadJoin(t.hthrd2);
	MyThreadJoin(t.hthrd3);

	MyThreadDestruct(t.hthrd1);
	MyThreadDestruct(t.hthrd2);
	MyThreadDestruct(t.hthrd3);
	MyMemPoolMemReport(1);
}


static void * test_thread_event_fun1(void * data)
{
	HMYEVENT he = (HMYEVENT)data;

	os_msleep(1000);

	MyEventSetSignaled(he);

	return NULL;
}

static void * test_thread_event_fun2(void * data)
{
	mytv_t tv = {0};
	HMYEVENT he = (HMYEVENT)data;

	MyEventWait(he, NULL);
	printf("[%s:%d] wait event end\r\n", __FILE__, __LINE__);

	tv.tv_sec = 2;
	MyEventWait(he, &tv);
	printf("[%s:%d] wait event timeout\r\n", __FILE__, __LINE__);

	return NULL;
}

static void test_event()
{
	HMYEVENT he = MyEventConstruct(NULL);
	HMYTHREAD hthrd1 = MyThreadConstruct(test_thread_event_fun1, he, 0, NULL);
	HMYTHREAD hthrd2 = MyThreadConstruct(test_thread_event_fun2, he, 0, NULL);

	MyThreadRun(hthrd1);
	MyThreadRun(hthrd2);

	MyThreadJoin(hthrd1);
	MyThreadJoin(hthrd2);

	MyThreadDestruct(hthrd1);
	MyThreadDestruct(hthrd2);
	MyEventDestruct(he);
	MyMemPoolMemReport(1);
}


static void * test_thread_mutex_fun(void * data)
{
	int i;
	HMYMUTEX hmtx = (HMYMUTEX)data;

	LOG_DEBUG(("%x", hmtx));

	printf("jbegin \r\n");
	for(i = 0; i < 10; i ++)
	{
		os_msleep(0);
		printf("j:%d ", i);
	}
	printf("jend \r\n");

	os_msleep(1000);

	MyMutexLock(hmtx);
	    
	printf("begin ");
	for(i = 0; i < 10; i ++)
	{
		os_msleep(0);
		printf("i:%d ", i);
	}
	printf("end \r\n");
        
	MyMutexUnLock(hmtx);	
}

static void test_mutex()
{
	HMYMUTEX hmtx = MyMutexConstruct(NULL);
	HMYTHREAD hthrd1 = MyThreadConstruct(test_thread_mutex_fun, hmtx, 0, NULL);
	HMYTHREAD hthrd2 = MyThreadConstruct(test_thread_mutex_fun, hmtx, 0, NULL);

	MyThreadRun(hthrd1);
	MyThreadRun(hthrd2);

	MyThreadJoin(hthrd1);
	MyThreadJoin(hthrd2);

	MyThreadDestruct(hthrd1);
	MyThreadDestruct(hthrd2);
	MyMutexDestruct(hmtx);
	MyMemPoolMemReport(1);
}


static void * test_thread_mutex_recursion_fun(void * data)
{
	int i = 5;
	HMYMUTEX hmtx = (HMYMUTEX)data;

	while(i)
	{
		MyMutexLock(hmtx);
		MyMutexLock(hmtx);
		MyMutexLock(hmtx);
		MyMutexLock(hmtx);

		printf("\r\n\r\n\r\n\r\n");
#ifdef WIN32
		LOG_DEBUG(("%d got it!", GetCurrentThreadId()));
#else
		LOG_DEBUG(("%d got it!", pthread_self()));
#endif
		os_msleep(300);

		MyMutexUnLock(hmtx);
		MyMutexUnLock(hmtx);
		MyMutexUnLock(hmtx);
		MyMutexUnLock(hmtx);

		i --;
	}
}

void test_mutex_recursion()
{
	HMYMUTEX hmtx = MyMutexConstruct(NULL);
	HMYTHREAD hthrd1 = MyThreadConstruct(test_thread_mutex_recursion_fun, hmtx, 1, NULL);
	HMYTHREAD hthrd2 = MyThreadConstruct(test_thread_mutex_recursion_fun, hmtx, 1, NULL);
	HMYTHREAD hthrd3 = MyThreadConstruct(test_thread_mutex_recursion_fun, hmtx, 1, NULL);

	MyThreadRun(hthrd1);
	MyThreadRun(hthrd2);
	MyThreadRun(hthrd3);

	MyThreadJoin(hthrd1);
	MyThreadJoin(hthrd2);
	MyThreadJoin(hthrd3);

	MyThreadDestruct(hthrd1);
	MyThreadDestruct(hthrd2);
	MyThreadDestruct(hthrd3);

	MyMutexDestruct(hmtx);
	MyMemPoolMemReport(1);
}

static void * test_thread_sem_fun1(void * data)
{
	HMYSEM hsem = (HMYSEM)data;

	os_msleep(5000);

	MySemInCrease(hsem);
}

static void * test_thread_sem_fun2(void * data)
{
	int i;

	HMYSEM hsem = (HMYSEM)data;

	for(i = 0; i < 6; i ++)
	{
		MySemDeCrease(hsem);
		printf("MySemDeCrease %d \r\n", i);
	}

	printf("over\r\n");
}

static void test_sem()
{
	HMYSEM hsem = MySemConstruct(NULL, 5);
	HMYTHREAD hthrd1 = MyThreadConstruct(test_thread_sem_fun1, hsem, 0, NULL);
	HMYTHREAD hthrd2 = MyThreadConstruct(test_thread_sem_fun2, hsem, 0, NULL);

	MyThreadJoin(hthrd1);
	MyThreadJoin(hthrd2);

	MyThreadDestruct(hthrd1);
	MyThreadDestruct(hthrd2);
	MySemDestruct(hsem);
	MyMemPoolMemReport(1);
}


static void test_mmap()
{
	char * p1 = NULL;
	char * p2 = NULL;
	HUOS_MMAP hmmap1 = UOS_MMapCreate(NULL, "test_mm", 1024, NULL); 
	HUOS_MMAP hmmap2 = UOS_MMapOpen(NULL, "test_mm", 1024, NULL);

	p1 = UOS_MMapGetBuf(hmmap1);
	p2 = UOS_MMapGetBuf(hmmap2);

	if(p1)
		strncpy(p1, "test mmap", 1024);
	else
		LOG_WARN(("fail create mmap1"));

	if(p2)
		LOG_INFO(("mmap2 see [%s]", p2));
	else
		LOG_WARN(("fail create mmap2"));

	UOS_MMapClose(hmmap1);
	UOS_MMapClose(hmmap2);

	MyMemPoolMemReport(1);
}


static void test_process()
{
	static char * actemp1[4] = {
		"wai",
		"mem",
		"bab",
		NULL,
	};

	static char * actemp2[4] = {
		"don",
		"wai",
		"mee",
		NULL,
	};

#ifdef WIN32
	UOS_CreateProcessAndWait("X:\\uos\\Rhapsody\\test\\Debug",
		"X:\\uos\\Rhapsody\\test\\Debug\\test_test.exe", actemp1);

	UOS_CreateProcess("F:\\tool\\",
		"F:\\tool\\chektool.exe", actemp2);
#else


	printf("UOS_CreateProcessAndWait\r\n");

	UOS_CreateProcessAndWait(NULL,
		"/ramdisk/test_test", actemp1);

	//UOS_CreateProcess("./",
	//	"/home/ist/lsc/uos/Rhapsody/test/test_test", actemp2);
#endif

	MyMemPoolMemReport(1);
}


//static void test_process_share_obj1()
//{
//#ifndef WIN32
//	pthread_mutexattr_t ma;
//	pthread_mutex_t * mtx;
//
//	HUOS_MMAP hmmap1 = UOS_MMapCreate(NULL, "test_mm", 1024, NULL); 
//
//	mtx = (pthread_mutex_t *)UOS_MMapGetBuf(hmmap1);
//	LOG_DEBUG(("malloc share mem:%x", mtx));
//
//	pthread_mutexattr_init(&ma);
//	pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);
//	pthread_mutex_init(mtx, &ma);
//
//	pthread_mutex_lock(mtx);
//	LOG_DEBUG(("i got mtx"));
//
//	pthread_mutex_lock(mtx);
//	LOG_DEBUG(("i regot it"));
//
//	UOS_MMapClose(hmmap1);
//
//	MyMemPoolMemReport(1);
//	exit(0);
//#endif
//}
//
//
//static void test_process_share_obj2()
//{
//#ifndef WIN32
//	char * p1 = NULL;
//	HUOS_MMAP hmmap1 = UOS_MMapOpen(NULL, "test_mm", 1024, NULL); 
//	pthread_mutex_t * mtx = (pthread_mutex_t *)UOS_MMapGetBuf(hmmap1);
//
//	LOG_DEBUG(("get share mem:%x", mtx));
//	pthread_mutex_unlock(mtx);
//
//	UOS_MMapClose(hmmap1);
//	MyMemPoolMemReport(1);
//	exit(0);
//#endif
//}

//#ifdef WIN32
//static void *  test_wait_for_mul_obj_thrdfun(void * param)
//{
//	HANDLE * h = (HANDLE *)param;
//
//	SetEvent(h[1]);
//	SetEvent(h[2]);
//}
//
//static void test_wait_for_mul_obj()
//{
//	int i = 0;
//	HANDLE h[10] = {0};
//	HANDLE h1[10] = {0};
//	unsigned long ret = 0;
//	HMYTHREAD hthrd = MyThreadConstruct(test_wait_for_mul_obj_thrdfun, h, 1, NULL);
//
//	for(i = 0; i < sizeof(h) / sizeof(h[0]); i ++)
//	{
//		h[i] = CreateEvent(NULL, 0, 0, NULL);
//	}
//
//	memcpy(h1, h, sizeof(h1));
//
//	MyThreadRun(hthrd);
//
//	Sleep(3000);
//
//	while(1)
//	{
//		LOG_DEBUG(("begin wait"));
//		ret = WaitForMultipleObjects(sizeof(h) / sizeof(h[0]), h, 0, 10000);
//		LOG_DEBUG(("end wait"));
//	}
//}
//#endif


void test_uos()
{
	LOG_INFO(("test_mutex_recursion begin"));
	test_mutex_recursion();

	test_event();
	LOG_INFO(("test_event END"));

	LOG_INFO(("test_mutex begin"));
	test_mutex();
	LOG_INFO(("test_mutex END"));

	test_mmap();
	LOG_INFO(("test_mmap END"));

	test_sem();
	LOG_INFO(("test_sem END"));

	test_thread();
	LOG_INFO(("test_thread END"));

	test_named_pipe();
	LOG_INFO(("test_named_pipe END"));

	test_pipe();
	LOG_INFO(("test_pipe END"));

	//LOG_INFO(("test_process"));
	//test_process();
	//Sleep(1000);

	//test_sock();
	//LOG_INFO(("test_sock END"));

	MyMemPoolMemReport(1);
}
















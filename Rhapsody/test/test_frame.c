/*
*
* HMYLISTERNER /frame/mylisterner.h
* "监听器"测试程序
*
*/
#include "mylisterner.h"
#ifdef WIN32
#include <windows.h>
#define snprintf _snprintf
#else
#include <unistd.h>
#define Sleep(x) sleep(x/1000)
#endif
#include "mylog.h"
#include <assert.h>
#include "mypipe.h"
#include "mysysvmsg.h"
#include "myevent.h"
#include "mythread.h"
#include "mysock.h"

static void test_timer_interface(HMYLISTERNER lsn);
static void test_msg(HMYLISTERNER lsn);
static void test_fd(HMYLISTERNER lsn);
static void test_sys_msg(HMYLISTERNER lsn);

void test_listerner()
{
	/*
	* 构造监听器, 
	* NULL表示不使用内存池, 
	* 0表示使用默认的消息队列大小(优先级与不带优先级)
	*/
	HMYLISTERNER lsn = MyListernerConstruct(NULL, 0);

	/*
	* 运行监听线程
	*/
	MyListernerRun(lsn);
	
	/*
	* 测试sysvmsg
	*/
#ifndef WIN32
	test_sys_msg(lsn);
#endif

	/*
	* 测试消息接口
	*/
	test_msg(lsn);

	/*
	* 测试fd接口
	*/
	test_fd(lsn);

	/*
	* 测试定时器接口
	*/
	test_timer_interface(lsn);

	Sleep(10 * 1000);

	MyListernerDestruct(lsn);
	
	MyMemPoolMemReport(1);
}

static int test_timer_thread_cb(unsigned long context_data, 
								unsigned long timer_user_data,
								void * timerid)
{
	struct timeval now = {0};
	gettimeofday(&now, NULL);

	LOG_DEBUG(("context:%d timer:%d %d %d\r\n", 
		context_data, timer_user_data, now.tv_sec, now.tv_usec));
		
}
										
static void test_timer_interface(HMYLISTERNER lsn)
{
	HTIMERID timer_id = NULL;
	HTIMERID timer_id1 = NULL;
	/* 添加定时器 */
	{
		mytimer_node_t node = {0};
		
		node.timeout_cb = test_timer_thread_cb;
		node.context_data = 1231;
		node.timer_user_data = 1231;
		node.first_expire.tv_sec = 1;
		node.first_expire.tv_usec = 0;
		node.period.tv_sec = 5;
		node.period.tv_usec = 500 * 1000;

		timer_id1 = MyListernerAddTimer(lsn, &node);
	}
	
	Sleep(2 * 1000);
	
	{
		mytimer_node_t node = {0};
		
		node.timeout_cb = test_timer_thread_cb;
		node.context_data = 1235;
		node.timer_user_data = 1235;
		node.first_expire.tv_sec = 1;
		node.first_expire.tv_usec = 0;
		node.period.tv_sec = 1;
		node.period.tv_usec = 0;

		timer_id = MyListernerAddTimer(lsn, &node);
	}

	Sleep(2 * 1000);
	MyListernerDelTimer(lsn, timer_id);
	
	Sleep(3 * 1000);
	{
		mytimer_node_t node = {0};
		
		node.timeout_cb = test_timer_thread_cb;
		node.context_data = 1236;
		node.timer_user_data = 1236;
		node.first_expire.tv_sec = 1;
		node.first_expire.tv_usec = 0;
		node.period.tv_sec = 0;
		node.period.tv_usec = 500 * 1000;

		timer_id = MyListernerResetTimer(lsn, timer_id1, &node);
	}
}

static int cb_test_lsn_promsg(unsigned long context_data, void * msg)
{
	printf("%d ", (int)msg);
		
	return 0;
}

static void test_msg(HMYLISTERNER lsn)
{
	int i = 0;

	LOG_INFO(("测试带优先级的消息队列"));
	for(i = 0; i < 50; i ++)
	{
		MyListernerAddProrityMsg(lsn, i, (void *)i, 1,
			cb_test_lsn_promsg);
		//if(!(i % 25))
		//	Sleep(1 * 1000);
	}
	printf("\n");

	Sleep(1000);

	LOG_INFO(("测试不带优先级的消息队列"));
	for(i = 0; i < 50; i ++)
	{
		MyListernerAddMsg(lsn, (void *)i, 2,
			cb_test_lsn_promsg);
		//if(!(i % 25))
		//	Sleep(1 * 1000);
	}
	printf("\n");
}

#ifndef WIN32
static int cb_listerner_input(unsigned long context_data, int fd)
{
	char actemp[256] = {0};
	int ret = 0;

	ret = read(fd, actemp, sizeof(actemp));
	LOG_INFO(("context_data:%d fd:%d [%d:%s]", context_data, fd, ret, actemp));
	return 0;	
}
#else
static HUOSUDP hudp1 = NULL;
static HUOSUDP hudp2 = NULL;
static int cb_listerner_input(unsigned long context_data, int fd)
{
	char acip[32] = {0};
	unsigned int port = 0;
	char actemp[256] = {0};
	int ret = 0;

	if(UOS_UDPGetFd(hudp1) == fd)
	{
		UOS_UDPReadFrom(hudp1, actemp, sizeof(actemp),
			acip, sizeof(acip), &port);
	}

	if(UOS_UDPGetFd(hudp2) == fd)
	{
		UOS_UDPReadFrom(hudp2, actemp, sizeof(actemp),
			acip, sizeof(acip), &port);
	}

	LOG_INFO(("context_data:%d fd:%d [%d:%s] [ip:%s port:%d]", context_data, fd, ret, actemp,
		acip, port));
}
#endif

#ifndef WIN32
static void test_fd(HMYLISTERNER lsn)
{
	int i = 0;
	HMYPIPE hpipe1 = MyPipeConstruct(NULL);
	HMYPIPE hpipe2 = MyPipeConstruct(NULL);

	{
		event_handle_t evt_handle = {cb_listerner_input, NULL, NULL, 1};
		MyListernerAddFD(lsn, MyPipeGetReadFD(hpipe1), E_FD_READ, &evt_handle);
	}

	{
		event_handle_t evt_handle = {cb_listerner_input, NULL, NULL, 2};
		MyListernerAddFD(lsn, MyPipeGetReadFD(hpipe2), E_FD_READ, &evt_handle);
	}
	
	for(i = 0; i < 10; i ++)
	{
		int j = 0;
		char actemp[] = "pipe1 msg";
		snprintf(actemp, sizeof(actemp), "pipe%d msg", i);
		MyPipeWrite(hpipe1, actemp, sizeof(actemp));
		MyPipeWrite(hpipe1, actemp, sizeof(actemp));
		//usleep(1);
		//Sleep(1000);
	}

	for(i = 0; i < 10; i ++)
	{
		int j = 0;
		char actemp[] = "pipe1 msg";
		snprintf(actemp, sizeof(actemp), "pipe%d msg", i);
		MyPipeWrite(hpipe2, actemp, sizeof(actemp));
		MyPipeWrite(hpipe2, actemp, sizeof(actemp));
		//usleep(1);;
		//Sleep(1000);
	}

	Sleep(3 * 1000);
	
	MyListernerDelFD(lsn, MyPipeGetReadFD(hpipe1));
	MyListernerDelFD(lsn, MyPipeGetReadFD(hpipe2));

	{
		char actemp[] = "pipe1 msg";
		snprintf(actemp, sizeof(actemp), "pipe%d msg", 1);
		MyPipeWrite(hpipe2, actemp, sizeof(actemp));
		Sleep(1000);
	}
	{
		char actemp[] = "pipe1 msg";
		snprintf(actemp, sizeof(actemp), "pipe%d msg", 1);
		MyPipeWrite(hpipe1, actemp, sizeof(actemp));
		Sleep(1000);
	}

	MyPipeDestruct(hpipe1);
	MyPipeDestruct(hpipe2);
}
#else
static void test_fd(HMYLISTERNER lsn)
{
	int i = 0;
	unsigned int port1 = 10000;
	unsigned int port2 = 10001;
	hudp1 = UOS_UDPConstruct(NULL, "192.168.35.85", &port1);
	hudp2 = UOS_UDPConstruct(NULL, "192.168.35.85", &port2);

	//{
	//	char acip[32] = {0};
	//	unsigned int port = 0;
	//	char actemp[256] = {0};
	//	UOS_UDPWriteTo(hudp1, "abc", strlen("abc"), "192.168.35.85", 10001);
	//	UOS_UDPReadFrom(hudp2, actemp, sizeof(actemp),
	//		acip, sizeof(acip), &port);
	//	
	//	printf("a");
	//}

	{
		event_handle_t evt_handle = {cb_listerner_input, NULL, NULL, 1};
		MyListernerAddFD(lsn, UOS_UDPGetFd(hudp1), E_FD_READ, &evt_handle);
	}

	{
		event_handle_t evt_handle = {cb_listerner_input, NULL, NULL, 2};
		MyListernerAddFD(lsn, UOS_UDPGetFd(hudp2), E_FD_READ, &evt_handle);
	}

	for(i = 0; i < 10; i ++)
	{
		int j = 0;
		char actemp[32] = "fd2 msg";
		snprintf(actemp, sizeof(actemp), "fd2 %d msg", i);
		UOS_UDPWriteTo(hudp1, actemp, strlen(actemp), "192.168.35.85", 10001);
		UOS_UDPWriteTo(hudp1, actemp, strlen(actemp), "192.168.35.85", 10001);
	}

	for(i = 0; i < 10; i ++)
	{
		int j = 0;
		char actemp[32] = "fd1 msg";
		snprintf(actemp, sizeof(actemp), "fd1 %d msg", i);
		UOS_UDPWriteTo(hudp2, actemp, strlen(actemp), "192.168.35.85", 10000);
		UOS_UDPWriteTo(hudp2, actemp, strlen(actemp), "192.168.35.85", 10000);
	}

	Sleep(3 * 1000);
	
	MyListernerDelFD(lsn, UOS_UDPGetFd(hudp1));
	MyListernerDelFD(lsn, UOS_UDPGetFd(hudp2));

	{
		char actemp[] = "fd1 msg";
		snprintf(actemp, sizeof(actemp), "newnewfd1 %d msg", 1);
		UOS_UDPWriteTo(hudp1, actemp, sizeof(actemp), "192.168.35.85", 10001);
		Sleep(1000);
	}
	{
		char actemp[] = "fd2 msg";
		snprintf(actemp, sizeof(actemp), "newnewfd2 %d msg", 1);
		UOS_UDPWriteTo(hudp2, actemp, sizeof(actemp), "192.168.35.85", 10000);
		Sleep(1000);
	}

	Sleep(3 * 1000);

	UOS_UDPDestruct(hudp1);
	UOS_UDPDestruct(hudp2);
}
#endif

static int cb_listerner_input_sysmq(unsigned long context_data, int fd)
{
	char actemp[256] = {0};
	int ret = 0;
	static count = 0;

	ret = MySVMsgQRead((HMYSVMSGQ)context_data, actemp, sizeof(actemp), 0);
	LOG_WARN(("context_data:%d fd:%d [%d:%s] %d", context_data, fd, ret, actemp, ++count));
	return 0;	
}

#ifndef WIN32
static void test_sys_msg(HMYLISTERNER lsn)
{
	int i = 0;
	
	HMYSVMSGQ hsysvmq1 = MySVMsgQConstruct(NULL, 1000, 1, 0, "abc");	
	HMYSVMSGQ hsysvmq2 = MySVMsgQConstruct(NULL, 1000, 0, 0, "abc");	
	HMYSVMSGQ hsysvmq3 = MySVMsgQGet(NULL, MySVMsgQGetID(hsysvmq1), "abc");	
	LOG_WARN(("hsysvmq1:%x hsysvmq2:%x hsysvmq3:%x", hsysvmq1, hsysvmq2, hsysvmq3));

	{
		event_handle_t evt_handle = {cb_listerner_input_sysmq, NULL, NULL, (unsigned long)hsysvmq1};
		MyListernerAddFD(lsn, MySVMsgQGetSelectFd(hsysvmq1), E_FD_READ, &evt_handle);
	}
	
	for(i = 0; i < 10; i ++)
	{
		MySVMsgQWrite(hsysvmq2, "abc", strlen("abc"));
		MySVMsgQWrite(hsysvmq3, "xyz", strlen("xyz"));
		//sleep(1);
	}

	sleep(1);
	
	MyListernerDelFD(lsn, MySVMsgQGetSelectFd(hsysvmq1));
	
	MySVMsgQDestruct(hsysvmq1);
	MySVMsgQDestruct(hsysvmq2);
	MySVMsgQDestruct(hsysvmq3);
}
#endif

static void * event_pipe_thread_fun(void * param)
{
	LOG_INFO(("begin pipe event wait"));
	
	MyEventWait((HMYEVENT)param, NULL);
	
	LOG_INFO(("end pipe event wait"));
}

#ifndef WIN32
void test_event_pipe()
{
	HMYEVENT he = MyEventConstruct(NULL);
	HMYTHREAD hthr1 = MyThreadConstruct(event_pipe_thread_fun, (void *)he, 0, NULL);
	HMYTHREAD hthr2 = MyThreadConstruct(event_pipe_thread_fun, (void *)he, 0, NULL);
	HMYTHREAD hthr3 = MyThreadConstruct(event_pipe_thread_fun, (void *)he, 0, NULL);
	
	Sleep(1000);

	LOG_INFO(("begin broadcast"));
	MyEventBroadCast(he);
	LOG_INFO(("end broadcast"));

	MyThreadJoin(hthr1);
	MyThreadJoin(hthr2);
	MyThreadJoin(hthr3);
	
	LOG_INFO(("destroy obj"));
	MyThreadDestruct(hthr1);
	MyThreadDestruct(hthr2);
	MyThreadDestruct(hthr3);
	MyEventDestruct(he);
	MyMemPoolMemReport(1);
}
#endif















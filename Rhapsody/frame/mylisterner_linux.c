/*
*
*mylisterner.h 定时器线程 lin shao chuan
*
* 参照ACE的Select_Reactor实现以下功能:
*  监听设备句柄
*  监听消息队列(优先级与无优先级)
*  提供定时器接口
*
* 暂未实现的接口:
*	提供sysv消息队列监听
*	仿照ACE提供suspend resume fd类似的接口
*	删除监听fd的接口
*
*/


#include "mylisterner.h"

#include <assert.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>

#include "mypipe.h"
#include "mymsgque.h"
#include "myproritymsgque.h"
#include "mymutex.h"
#include "myhashmap.h"
#include "mythread.h"
#include "mylog.h"
#include "myutility.h"


typedef struct __mylisterner_t_
{
	//fd集合 
	HMYHANDLESET handles;
	HMYPIPE handles_notifier;

	//fd事件回调函数表
	HMYHASHMAP handles_dispatch_table;
	HMYMUTEX handles_protect;

	//普通消息队列
	HMYMSGQUE mq;
	HMYPIPE mq_notifier;

	//优先级消息队列
	HMY_PRO_MQ promq;
	HMYPIPE promq_notifier;

	//定时器
	HMYTIMERHEAP tmhp;
	HMYPIPE tmhp_notifier;
	
	//监听线程
	HMYTHREAD thr;

	HMYMEMPOOL hm;
	
	int bNotExit;
}mylisterner_t;

typedef struct __msg_tag_
{
	CB_LISTERNER_HANDLE_MSG handle;
	unsigned long context_data;
	void * msg;
}msg_tag;

typedef struct __notify_tag_
{
	int unused;
}notify_tag;


static size_t handle_table_hashfun(const void * key)
{
	return (size_t)key;
}

static int handle_table_equalfun(const void * key1, const void * key2)
{
	return (key1 == key2);
}


/*
*
* 处理带优先级的消息
*
*/
static __INLINE__ void listerner_process_promsg(mylisterner_t * l)
{
	msg_tag * msg = NULL;

	LOG_DEBUG(("listerner_process_promsg"));

	//读出所有的消息
	assert(l && l->promq);

	{
		/* 读出管道里的东西 */
		notify_tag t = {0};
		int ret = 0;
		do{
			ret = MyPipeRead(l->mq_notifier, &t, sizeof(t));
		}while(ret > 0);
	}

	//呼叫回调函数
	while(msg = (msg_tag *)MyProrityMsgQuePop(l->promq))
	{
		LOG_DEBUG(("msg:%x", msg));

		if(msg->handle)
			msg->handle(msg->context_data, msg->msg);

		MyMemPoolFree(l->hm, msg);
	}
}

/*
*
* 处理带超时消息
*
*/
static __INLINE__ void listerner_process_timeout(mylisterner_t * l)
{
	struct timeval now;

	LOG_DEBUG(("listerner_process_timeout"));

	assert(l && l->tmhp);
	
	gettimeofday(&now, NULL);	

	MyTimerHeapRunExpire(l->tmhp, (mytv_t *)&now);
}

/*
*
* 处理带普通消息
*
*/
static __INLINE__ void listerner_process_msg(mylisterner_t * l)
{
	msg_tag * msg = NULL;

	LOG_DEBUG(("listerner_process_msg"));

	//读出所有的消息
	assert(l && l->mq);

	{
		/* 读出管道里的东西 */
		notify_tag t = {0};
		int ret = 0;
		do{
			ret = MyPipeRead(l->mq_notifier, &t, sizeof(t));
		}while(ret > 0);
	}

	//呼叫回调函数
	while(msg = (msg_tag *)MyMsgQuePop(l->mq))
	{
		LOG_DEBUG(("msg:%x", msg));

		if(msg->handle)
			msg->handle(msg->context_data, msg->msg);

		MyMemPoolFree(l->hm, msg);
	}
}

/*
*
* 处理可读的io fd
*
*/
static __INLINE__ void listerner_dispatch_read(mylisterner_t * l, HMYVECTOR read_set)
{
	size_t i = 0;
	size_t loop = 0;

	LOG_DEBUG(("listerner process io read event"));
	assert(l && read_set);

	//轮循read_set,
	loop = MyVectorGetCount(read_set);
	for(i = 0; i < loop; i ++)
	{
		event_handle_t evt_handle = {0};
		event_handle_t * peh = NULL;
		HMYHASHMAP_ITER it = NULL;
		int fd = (int)MyVectorGetIndexData(read_set, i, NULL);

		//加锁
		if(0 != MyMutexLock(l->handles_protect))
			continue;

		//查表回调
		it = MyHashMapSearch(l->handles_dispatch_table, (void *)fd);
		if(NULL == it)
		{
			//解锁
			MyMutexUnLock(l->handles_protect);
			continue;
		}

		peh = (event_handle_t *)MyHashMapGetIterData(it);
		if(NULL == peh || NULL == peh->input)
		{
			//解锁
			MyMutexUnLock(l->handles_protect);
			continue;
		}

		memcpy(&evt_handle, peh, sizeof(evt_handle));

		//解锁
		MyMutexUnLock(l->handles_protect);

		evt_handle.input(evt_handle.context_data, fd);
	}
}

/*
*
* 处理可写的io fd
*
*/
static __INLINE__ void listerner_dispatch_write(mylisterner_t * l, HMYVECTOR write_set)
{
	size_t i = 0;
	size_t loop = 0;

	LOG_DEBUG(("listerner process io write event"));
	assert(l && write_set);

	//轮循read_set,
	loop = MyVectorGetCount(write_set);
	for(i = 0; i < loop; i ++)
	{
		event_handle_t evt_handle = {0};
		event_handle_t * peh = NULL;
		HMYHASHMAP_ITER it = NULL;
		int fd = (int)MyVectorGetIndexData(write_set, i, NULL);

		//加锁
		if(0 != MyMutexLock(l->handles_protect))
			continue;

		//查表回调
		it = MyHashMapSearch(l->handles_dispatch_table, (void *)fd);
		if(NULL == it)
		{
			//解锁
			MyMutexUnLock(l->handles_protect);
			continue;
		}

		peh = (event_handle_t *)MyHashMapGetIterData(it);
		if(NULL == peh || NULL == peh->output)
		{
			//解锁
			MyMutexUnLock(l->handles_protect);
			continue;
		}

		memcpy(&evt_handle, peh, sizeof(evt_handle));

		//解锁
		MyMutexUnLock(l->handles_protect);

		evt_handle.output(evt_handle.context_data, fd);
	}
}

/*
*
* 处理异常的io fd
*
*/
static __INLINE__ void listerner_dispatch_exception(mylisterner_t * l, HMYVECTOR exception_set)
{
	size_t i = 0;
	size_t loop = 0;

	LOG_DEBUG(("listerner process io exception event"));
	assert(l && exception_set);

	//轮循read_set,
	loop = MyVectorGetCount(exception_set);
	for(i = 0; i < loop; i ++)
	{
		event_handle_t evt_handle = {0};
		event_handle_t * peh = NULL;
		HMYHASHMAP_ITER it = NULL;
		int fd = (int)MyVectorGetIndexData(exception_set, i, NULL);

		//加锁
		if(0 != MyMutexLock(l->handles_protect))
			continue;

		//查表回调
		it = MyHashMapSearch(l->handles_dispatch_table, (void *)fd);
		if(NULL == it)
		{
			//解锁
			MyMutexUnLock(l->handles_protect);
			continue;
		}

		peh = (event_handle_t *)MyHashMapGetIterData(it);
		if(NULL == peh || NULL == peh->exception)
		{
			//解锁
			MyMutexUnLock(l->handles_protect);
			continue;
		}

		memcpy(&evt_handle, peh, sizeof(evt_handle));

		//解锁
		MyMutexUnLock(l->handles_protect);

		evt_handle.exception(evt_handle.context_data, fd);
	}
}

static __INLINE__ int listerner_del_fd(mylisterner_t * l, int fd)
{
	assert(l && l->handles_dispatch_table && l->handles);

	//加锁
	if(0 != MyMutexLock(l->handles_protect))
		return -1;

	MyHashMapDelKey(l->handles_dispatch_table, (void *)fd);

	//解锁
	MyMutexUnLock(l->handles_protect);

	//从fd集合中删除
	MyHandleSetDelFd(l->handles, fd);

	//产生fd添加通知
	{
		notify_tag t = {0};
		MyPipeWrite(l->handles_notifier, &t, sizeof(t));
	}
	
	return 0;
}

static __INLINE__ int listerner_add_fd(mylisterner_t * l, int fd, enum E_HANDLE_SET_MASK mask, event_handle_t * evt_handle)
{
	int ret = 0;

	assert(l && l->handles_dispatch_table && l->handles);


	//添加io fd回调函数表
	if(evt_handle)
	{
		//加锁
		if(0 != MyMutexLock(l->handles_protect))
			return -1;

		if(NULL == MyHashMapInsertUnique(l->handles_dispatch_table, (void *)fd, 0, evt_handle, sizeof(*evt_handle)))
		{
			//解锁
			MyMutexUnLock(l->handles_protect);
			return -1;
		}
		else/*解锁*/
			MyMutexUnLock(l->handles_protect);
	}

	//添加到fd集合
	if((ret = MyHandleSetFdSet(l->handles, fd, mask)) != 0)
		goto listerner_add_fd_end_;

	//产生fd添加通知
	{
		notify_tag t = {0};
		MyPipeWrite(l->handles_notifier, &t, sizeof(t));
	}

	return 0;

listerner_add_fd_end_:

	//加锁
	MyMutexLock(l->handles_protect);

	//从回调表中删除
	MyHashMapDelKey(l->handles_dispatch_table, (void *)fd);

	//解锁
	MyMutexUnLock(l->handles_protect);

	//从集合中删除
	MyHandleSetDelFd(l->handles, fd);

	return ret;
}

/*
*
* 处理带io消息
*
*/
static __INLINE__ void listerner_process_io(mylisterner_t * l)
{
	HMYVECTOR read_set = NULL;
	HMYVECTOR write_set = NULL;
	HMYVECTOR exception_set = NULL;

	LOG_DEBUG(("listerner_process_io"));

	assert(l);

	read_set = MyVectorConstruct(l->hm, 0, NULL, NULL);
	write_set = MyVectorConstruct(l->hm, 0, NULL, NULL);
	exception_set = MyVectorConstruct(l->hm, 0, NULL, NULL);

	//取出所有发生事件的fd
	MyHandleSetGetAllSignalFd(l->handles,
		read_set, write_set, exception_set);

	//查表,
	//呼叫相应的回调函数
	if(MyVectorGetCount(read_set))
		listerner_dispatch_read(l, read_set);
	if(MyVectorGetCount(write_set))
		listerner_dispatch_write(l, write_set);
	if(MyVectorGetCount(exception_set))
		listerner_dispatch_exception(l, exception_set);

	MyVectorDestruct(read_set);
	MyVectorDestruct(write_set);
	MyVectorDestruct(exception_set);
}

/*
*
* 监听线程函数
*
*/
static void * listerner_thread_fun(void * param)
{
	struct timeval tv_now = {0};
	struct timeval tv_earliest = {0};
	int ret = 0;

	mylisterner_t * l = (mylisterner_t *)param;
	assert(l);

	LOG_DEBUG(("listerner loop run"));

	while(l->bNotExit)
	{
		LOG_DEBUG(("listerner process begin\r\n"));

		//循环select fd集合timeout为定时器的最小超时
		if(0 == MyTimerHeapGetEarliestExpire(l->tmhp, (mytv_t *)&tv_earliest))
		{
			gettimeofday(&tv_now, NULL);
			if(timeval_smaller(tv_now, tv_earliest))
			{
				timeval_minus(tv_earliest, tv_now);
				ret = MyHandleSetSelect(l->handles, &tv_earliest);
			}
			else
			{
				ret = 0;
				memset(&tv_earliest, 0, sizeof(tv_earliest));
				LOG_INFO(("null loop"));
			}
		}
		else
			ret = MyHandleSetSelect(l->handles, NULL);

		LOG_DEBUG(("event fd count:%d", ret));

		//没有事件发生,只运行定时器回调
		if(0 == ret)
		{
			LOG_DEBUG(("process timer"));
			listerner_process_timeout(l);
			continue;
		}

		//优先处理优先级队列的消息
		if(E_FD_READ & MyHandleSetIsSet(l->handles, MyPipeGetReadFD(l->promq_notifier), E_FD_READ))
		{
			LOG_DEBUG(("process promsg"));
			listerner_process_promsg(l);
		}

		//处理普通消息队列
		if(E_FD_READ & MyHandleSetIsSet(l->handles, MyPipeGetReadFD(l->mq_notifier), E_FD_READ))
		{
			LOG_DEBUG(("process msg"));
			listerner_process_msg(l);
		}
		
		//删除添加定时器发生的事件
		if(E_FD_READ & MyHandleSetIsSet(l->handles, MyPipeGetReadFD(l->tmhp_notifier), E_FD_READ))
		{
			LOG_DEBUG(("add or del timer"));

			//从管道里读出消息
			notify_tag t = {0};
			MyPipeRead(l->tmhp_notifier, &t, sizeof(t));
		}

		//删除由于添加io fd产生的事件
		if(E_FD_READ & MyHandleSetIsSet(l->handles, MyPipeGetReadFD(l->handles_notifier), E_FD_READ))
		{
			LOG_DEBUG(("add or del fd"));

			//从管道里读出消息
			notify_tag t = {0};
			MyPipeRead(l->handles_notifier, &t, sizeof(t));
		}

		//根据fd hash表进行相应的调度
		listerner_process_io(l);
	}

	return NULL;
}

/*
*
* 监听线程函数
*
*/
static __INLINE__ void destroy(mylisterner_t * l)
{
	assert(l);
	
	l->bNotExit = 0;

	{
		notify_tag t = {0};
		MyPipeWrite(l->handles_notifier, &t, sizeof(t));
	}
	MyThreadJoin(l->thr);
	
	if(l->thr)
		MyThreadDestruct(l->thr);
	l->thr = NULL;
		
	if(l->handles)
		MyHandleSetDestruct(l->handles);
	l->handles = NULL;

	if(l->mq)
		MyMsgQueDestruct(l->mq);
	l->mq = NULL;

	if(l->mq_notifier)
		MyPipeDestruct(l->mq_notifier);
	l->mq_notifier = NULL;

	if(l->promq)
		MyProrityMsgQueDestruct(l->promq);
	l->promq = NULL;

	if(l->promq_notifier)
		MyPipeDestruct(l->promq_notifier);
	l->promq_notifier = NULL;

	if(l->tmhp)
		MyTimerHeapDestruct(l->tmhp);
	l->tmhp = NULL;

	if(l->tmhp_notifier)
		MyPipeDestruct(l->tmhp_notifier);
	l->tmhp_notifier = NULL;

	
	//加锁
	MyMutexLock(l->handles_protect);

	if(l->handles_dispatch_table)
		MyHashMapDestruct(l->handles_dispatch_table);
	l->handles_dispatch_table = NULL;

	//解锁
	MyMutexUnLock(l->handles_protect);

	if(l->handles_protect)
		MyMutexDestruct(l->handles_protect);
	l->handles_protect = NULL;

	if(l->handles_notifier)
		MyPipeDestruct(l->handles_notifier);
	l->handles_notifier = NULL;

	MyMemPoolFree(l->hm, l);
}


/*
*
* 构造监听线程
*
*/
HMYLISTERNER MyListernerConstruct(HMYMEMPOOL hm, size_t max_msg_count)
{
	mylisterner_t * l = (mylisterner_t *)MyMemPoolMalloc(hm, sizeof(*l));
	if(NULL == l)
		return NULL;

	LOG_DEBUG(("create listerner"));

	l->hm = hm;
	l->bNotExit = 1;

	//定时器
	l->tmhp = MyTimerHeapConstruct(hm);
	l->tmhp_notifier = MyPipeConstruct(hm);
	MyPipeNoBlock(l->tmhp_notifier);

	//io句柄表 
	l->handles = MyHandleSetConstruct(hm);
	l->handles_notifier = MyPipeConstruct(hm);
	MyPipeNoBlock(l->handles_notifier);
	l->handles_dispatch_table = MyHashMapConstruct(hm,
		handle_table_hashfun, handle_table_equalfun, 0, NULL, NULL);
	l->handles_protect = MyMutexConstruct(hm);

	//普通消息队列
	l->mq = MyMsgQueConstruct(hm, max_msg_count);
	l->mq_notifier = MyPipeConstruct(hm);
	MyPipeNoBlock(l->mq_notifier);


	//优先级消息队列
	l->promq = MyProrityMsgQueConstruct(hm, max_msg_count);
	l->promq_notifier = MyPipeConstruct(hm);
	MyPipeNoBlock(l->promq_notifier);

	listerner_add_fd(l, MyPipeGetReadFD(l->promq_notifier), E_FD_READ, NULL);
	listerner_add_fd(l, MyPipeGetReadFD(l->mq_notifier), E_FD_READ, NULL);
	listerner_add_fd(l, MyPipeGetReadFD(l->handles_notifier), E_FD_READ, NULL);
	listerner_add_fd(l, MyPipeGetReadFD(l->tmhp_notifier), E_FD_READ, NULL);
	
	//监听线程
	l->thr = MyThreadConstruct(listerner_thread_fun, l, 1, hm);
	
	if(NULL == l->handles || NULL == l->handles_notifier || NULL == l->handles_dispatch_table || NULL == l->handles_protect ||
		NULL == l->mq || NULL == l->mq_notifier ||
		NULL == l->promq || NULL == l->promq_notifier ||
		NULL == l->tmhp || NULL == l->tmhp_notifier ||
		NULL == l->thr)
	{
		LOG_WARN(("fail create listerner"));
		destroy(l);
		return NULL;
	}

	return (HMYLISTERNER)l;
}

/*
*
* 析构监听线程
*
*/
void MyListernerDestruct(HMYLISTERNER hlisterner)
{
	mylisterner_t * l = (mylisterner_t *)hlisterner;
	if(NULL == l)
		return;

	LOG_DEBUG(("destroy listerner"));

	destroy(l);
}

/*
*
* 运行监听线程
*
*/
void MyListernerRun(HMYLISTERNER hlisterner)
{
	mylisterner_t * l = (mylisterner_t *)hlisterner;
	if(NULL == l)
		return;

	assert(l->thr);
	MyThreadRun(l->thr);
}

/*
*
* 等待listern线程退出
*
*/
void MyListernerWait(HMYLISTERNER hlisterner)
{
	mylisterner_t * l = (mylisterner_t *)hlisterner;
	if(NULL == l)
		return;

	assert(l->thr);
	MyThreadJoin(l->thr);
}

/*
*
* 添加定时器
*
*/
HTIMERID MyListernerAddTimer(HMYLISTERNER hlisterner, mytimer_node_t * node)
{
	HTIMERID timer_id = NULL;
	mylisterner_t * l = (mylisterner_t *)hlisterner;
	if(NULL == l || NULL == node || NULL == l->tmhp)
		return NULL;

	LOG_DEBUG(("listerner add timer sec:%d usec:%d", node->first_expire.tv_sec, node->first_expire.tv_usec));

	gettimeofday((struct timeval *)&node->abs_expire, NULL);
	timeval_add(node->abs_expire, node->first_expire);

	//先添加,再唤醒
	timer_id = MyTimerHeapAdd(l->tmhp, node);

	if(MyTimeHeapGetEarliestKey(l->tmhp) == timer_id)
	{
		notify_tag t = {0};
		MyPipeWrite(l->tmhp_notifier, &t, sizeof(t));

		LOG_DEBUG(("write timer-add notify, timer id:%x", timer_id));
	}

	return timer_id;
}

/*
*
* 删除定时器
*
*/
int MyListernerDelTimer(HMYLISTERNER hlisterner, HTIMERID timer_id)
{
	mylisterner_t * l = (mylisterner_t *)hlisterner;
	if(NULL == l || NULL == l->tmhp)
		return -1;

	return MyTimerHeapDel(l->tmhp, timer_id);
}

/*
*
* 重置加定时器
*
*/
HTIMERID MyListernerResetTimer(HMYLISTERNER hlisterner, HTIMERID timer_id, mytimer_node_t * node)
{
	mylisterner_t * l = (mylisterner_t *)hlisterner;
	if(NULL == l || NULL == l->tmhp)
		return NULL;

	gettimeofday((struct timeval *)&node->abs_expire, NULL);
	timeval_add(node->abs_expire, node->first_expire);

	timer_id = MyTimerHeapReset(l->tmhp, timer_id, node);

	if(NULL == timer_id)
		return NULL;

	//如果发现 <新的超时> 比 <当前最短的超时> 还要短,表示需要唤醒定时器线程
	if(MyTimeHeapGetEarliestKey(l->tmhp) == timer_id)
	{
		notify_tag t = {0};
		MyPipeWrite(l->tmhp_notifier, &t, sizeof(t));
	}

	return timer_id;
}

/*
*
* 添加文件扫描符
*
*/
int MyListernerAddFD(HMYLISTERNER hlisterner, int fd, enum E_HANDLE_SET_MASK mask, event_handle_t * evt_handle)
{
	mylisterner_t * l = (mylisterner_t *)hlisterner;
	if(NULL == l || NULL == l->handles_dispatch_table || NULL == evt_handle || NULL == l->handles)
		return -1;

	LOG_DEBUG(("add fd:%d mask:%d", fd, mask));
	return listerner_add_fd(l, fd, mask, evt_handle);
}

/*
*
* 删除文件扫描符
*
*/
int MyListernerDelFD(HMYLISTERNER hlisterner, int fd)
{
	mylisterner_t * l = (mylisterner_t *)hlisterner;
	if(NULL == l || NULL == l->handles_dispatch_table || NULL == l->handles)
		return -1;

	LOG_DEBUG(("del fd:%d", fd));
	return listerner_del_fd(l, fd);
}


/*
*
* 添加一条消息 
*
*/
int MyListernerAddMsg(HMYLISTERNER hlisterner, 
							 const void * user_msg, 
							 unsigned long context_data,
							 CB_LISTERNER_HANDLE_MSG handle)
{
	msg_tag * msg = NULL;

	mylisterner_t * l = (mylisterner_t *)hlisterner;
	if(NULL == l || NULL == l->mq)
		return -1;

	//添加消息
	msg = (msg_tag *)MyMemPoolMalloc(l->hm, sizeof(*msg));
	if(NULL == msg)
		return -1;

	msg->context_data = context_data;
	msg->handle = handle;
	msg->msg = (void *)user_msg;

	MyMsgQuePush(l->mq, msg);

	//通知
	{
		notify_tag t = {0};
		MyPipeWrite(l->mq_notifier, &t, sizeof(t));
	}
	
	return 0;
}

/*
*
* 添加一条带优先级的消息
*
*/
int MyListernerAddProrityMsg(HMYLISTERNER hlisterner, 
									int prority, 
									const void * user_msg, 
									unsigned long context_data,
									CB_LISTERNER_HANDLE_MSG handle)
{
	msg_tag * msg = NULL;
	int ret = 0;

	mylisterner_t * l = (mylisterner_t *)hlisterner;
	if(NULL == l || NULL == l->promq)
		return -1;

	//添加消息
	msg = (msg_tag *)MyMemPoolMalloc(l->hm, sizeof(*msg));
	if(NULL == msg)
		return -1;

	msg->context_data = context_data;
	msg->handle = handle;
	msg->msg = (void *)user_msg;

	ret = MyProrityMsgQuePush(l->promq, prority, msg);

	//通知
	if(0 == ret)
	{
		notify_tag t = {0};
		MyPipeWrite(l->promq_notifier, &t, sizeof(t));
	}
	else
		LOG_WARN(("add prority msg fail %d", ret));	
	
	return ret;
}


/*
*
* 获取定时器个数
*
*/
int MyListernerPrint(HMYLISTERNER hlisterner, char * pctemp, size_t sz)
{
	mylisterner_t * l = (mylisterner_t *)hlisterner;

	//printf("listerner %x\r\n timer:%d msgq:%d\r\n", 
	//	l, MyTimerHeapGetCount(l->tmhp),
	//	MyMsgQueGetCount(l->mq));

	if(pctemp && sz)
	{
		snprintf(pctemp, sz, "listerner %x\r\n timer:%d msgq:%d\r\n", 
			l, MyTimerHeapGetCount(l->tmhp),
			MyMsgQueGetCount(l->mq));
	}

	return 	MyTimerHeapGetCount(l->tmhp);
}















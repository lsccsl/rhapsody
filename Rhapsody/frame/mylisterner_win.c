/**
* @file mylisterner_win.c windows�汾�ļ����� 2008-04-03 15:01
*
* @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
* @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.        
*
* Permission to use, copy, modify, distribute and sell this software
* and its documentation for any purpose is hereby granted without fee,
* provided that the above copyright notice appear in all copies and
* that both that copyright notice and this permission notice appear
* in supporting documentation.  lin shao chuan makes no
* representations about the suitability of this software for any
* purpose.  It is provided "as is" without express or implied warranty.
* see the GNU General Public License  for more detail.
*/

#include <winsock2.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>

#include "mylisterner.h"

#include "__event_win32.h"
#include "mymutex.h"
#include "myproritymsgque.h"
#include "mymsgque.h"
#include "mythread.h"
#include "gettimeofday.h"
#include "mylog.h"
#include "myhashmap.h"
#include "os_def.h"
#pragma warning(disable:4312)
#pragma warning(disable:4311)


typedef struct __mylisterner_t_
{
	/* �������,������� */
	HANDLE harray[MAXIMUM_WAIT_OBJECTS];
	size_t hcount;
	/* ����ص��� */
	HMYHASHMAP handles_dispatch_table;
	/* harray��handles_dispatch_table�ı����� */
	HMYMUTEX handles_protect;
	/* ���ɾ�����������֪ͨ�¼� */
	HMYEVENT handleset_notifier;

	/* ��ͨ��Ϣ����,�Լ���ͨ��Ϣ���е�֪ͨ�ź� */
	HMYMSGQUE mq;
	HMYEVENT mq_notifier;

	/* ���ȼ���Ϣ����,�Լ����ȼ���Ϣ���е�֪ͨ�ź� */
	HMY_PRO_MQ promq;
	HMYEVENT promq_notifier;

	/* ��ʱ��,�Լ���ʱ����֪ͨ�ź� */
	HMYTIMERHEAP tmhp;
	HMYEVENT tmhp_notifier;

	/* �����߳� */
	HMYTHREAD thr;

	/* �ڴ�ؾ�� */
	HMYMEMPOOL hm;
	
	/* �Ƿ��˳� */
	int bNotExit;
}mylisterner_t;

typedef struct __msg_tag_
{
	CB_LISTERNER_HANDLE_MSG handle;
	unsigned long context_data;
	void * msg;
}msg_tag;

typedef struct __evt_tag_
{
	/* ��¼�û��Ļص�����,�Լ����������� */
	event_handle_t e;

	/* socket��� */
	int fd;

	/* ��¼������¼����� */
	enum E_HANDLE_SET_MASK mask;
}evt_tag;


/**
 * @brief ��Ӽ������
 */
static int listerner_add_handle(mylisterner_t * l, HANDLE h, evt_tag * et)
{
	int ret = 0;

	assert(l && h != INVALID_HANDLE_VALUE);

	/* ���� */
	if(0 != MyMutexLock(l->handles_protect))
		return -1;

	if(l->hcount >= sizeof(l->harray)/sizeof(l->harray[0]))
	{
		ret = -1;
		goto listerner_add_handle_end_;
	}

	l->harray[l->hcount] = h;
	l->hcount += 1;

	if(NULL == et)
		goto listerner_add_handle_end_;

	MyHashMapInsertUnique(l->handles_dispatch_table, h, 0, et, sizeof(*et));


listerner_add_handle_end_:

	/* ���� */
	MyMutexUnLock(l->handles_protect);

	/* ����֪ͨ,���ѹ����߳� */
	MyEventSetSignaled(l->handleset_notifier);

	return ret;
}

/**
 * @brief ��ȡ�����������
 * @param h:�������,�������صľ������
 * @param h_sz:h�����ɵľ������
 */
static int listerner_get_handle(mylisterner_t * l, HANDLE * h, size_t h_sz)
{
	assert(l && h && h_sz >= sizeof(l->harray)/sizeof(l->harray[0]));

	/* ���� */
	if(0 != MyMutexLock(l->handles_protect))
		return -1;

	memset(h, 0, sizeof(h[0]) * h_sz);

	assert(l->hcount <= sizeof(l->harray)/sizeof(l->harray[0]));
	memcpy(h, l->harray, sizeof(l->harray[0]) * l->hcount);

	/* ���� */
	MyMutexUnLock(l->handles_protect);

	return 0;
}

/**
 * @brief ��ȡ�����������
 * @param h:�������,�������صľ������
 * @param h_sz:h�����ɵľ������
 */
static int listerner_del_handle(mylisterner_t * l, const HANDLE h)
{
	unsigned int i;
	int ret = 0;
	assert(l && h != INVALID_HANDLE_VALUE);

	/* ���� */
	if(0 != MyMutexLock(l->handles_protect))
		return -1;

	if(0 == l->hcount)
	{
		ret = 0;
		goto listerner_del_handle_end_;
	}

	for(i = 0; i < l->hcount; i ++)
	{
		if(h == l->harray[i])
			break;
	}

	if(i >= l->hcount)
	{
		ret = -1;
		goto listerner_del_handle_end_;
	}

	for(; i < l->hcount - 1; i ++)
		l->harray[i] = l->harray[i + 1];

	l->hcount -= 1;

	MyHashMapDelKey(l->handles_dispatch_table, h);

listerner_del_handle_end_:

	/* ���� */
	MyMutexUnLock(l->handles_protect);

	/* ����֪ͨ,���ѹ����߳� */
	MyEventSetSignaled(l->handleset_notifier);

	return ret;
}

/**
 * @brief ��ѯfd��ص���Ϣ
 */
static __INLINE__ int listerner_query_handle(mylisterner_t * l, HANDLE h, evt_tag * et)
{
	int ret = 0;
	evt_tag * e_tmp = NULL;
	HMYHASHMAP_ITER it = NULL;

	assert(l && et && INVALID_HANDLE_VALUE != h);

	/* ���� */
	if(0 != MyMutexLock(l->handles_protect))
		return -1;

	it = MyHashMapSearch(l->handles_dispatch_table, h);
	if(NULL == it)
	{
		LOG_WARN(("dispatch:%x, can't find handle info", h));
		ret = -1;
		goto listerner_query_handle_end_;
	}

	memcpy(et, MyHashMapGetIterData(it), sizeof(*et));

listerner_query_handle_end_:

	/* ���� */
	MyMutexUnLock(l->handles_protect);

	return ret;
}

/**
 * @brief ��ѯfd��ص���Ϣ
 */
static __INLINE__ int listerner_query_handle_by_fd(mylisterner_t * l, const int fd, evt_tag * et, HANDLE * ph)
{
	int ret = 0;
	evt_tag * e_tmp = NULL;
	HMYHASHMAP_ITER it = NULL;

	assert(l);

	/* ���� */
	if(0 != MyMutexLock(l->handles_protect))
		return -1;

	/* ������ϣ��,�ҵ���Ӧ��fd */
	it = MyHashMapBegin(l->handles_dispatch_table);
	for(; it != NULL; it = MyHashMapGetNext(l->handles_dispatch_table, it))
	{
		e_tmp = (evt_tag *)MyHashMapGetIterData(it);
		if(NULL == e_tmp)
			continue;

		if(fd == e_tmp->fd)
			break;
	}

	if(NULL == it)
	{
		ret = -1;
		goto listerner_query_handle_end_;
	}

	if(et)
		memcpy(et, e_tmp, sizeof(*et));

	if(ph)
		*ph = (HANDLE)MyHashMapGetIterKey(it);

listerner_query_handle_end_:

	/* ���� */
	MyMutexUnLock(l->handles_protect);

	return ret;
}

/**
 * @brief �������ʱ��Ϣ
 */
static __INLINE__ void listerner_process_promsg(mylisterner_t * l)
{
	msg_tag * msg = NULL;

	assert(l && l->promq);

	/* �������е���Ϣ */
	while(msg = (msg_tag *)MyProrityMsgQuePop(l->promq))
	{
		/* ���лص����� */
		if(msg->handle)
			msg->handle(msg->context_data, msg->msg);

		MyMemPoolFree(l->hm, msg);
	}
}

/**
 * @brief �������ʱ��Ϣ
 */
static __INLINE__ void listerner_process_msg(mylisterner_t * l)
{
	msg_tag * msg = NULL;

	assert(l && l->mq);

	/* �������е���Ϣ */
	while(msg = (msg_tag *)MyMsgQuePop(l->mq))
	{
		/* ���лص����� */
		if(msg->handle)
			msg->handle(msg->context_data, msg->msg);

		MyMemPoolFree(l->hm, msg);
	}
}

/**
 * @brief �������ʱ��Ϣ
 */
static __INLINE__ void listerner_process_timeout(mylisterner_t * l)
{
	struct timeval now;

	assert(l && l->tmhp);
	
	gettimeofday(&now, NULL);	

	MyTimerHeapRunExpire(l->tmhp, (mytv_t *)&now);
}

/**
 * @brief �������ص�����ľ���¼�
 */
static __INLINE__ void listerner_dispatch_handles(mylisterner_t * l, HANDLE h)
{
	evt_tag et = {0};
	WSANETWORKEVENTS events = {0};

	assert(l && INVALID_HANDLE_VALUE != h);

	/* ���,�ص� */
	if(0 != listerner_query_handle(l, h, &et))
		return;

	WSAEnumNetworkEvents(et.fd, h, &events);

	/*
	* FD_READ/FD_ACCEPT/FD_CONNECT ͳһ��input���������
	* ��Ϊ��linux��������ʱ,accept����input�����ﴥ����
	*/
	if((events.lNetworkEvents & FD_OOB_BIT) && (et.mask & E_FD_READ) && et.e.input)
		et.e.input(et.e.context_data, et.fd);
	if((events.lNetworkEvents & FD_READ) && (et.mask & E_FD_READ) && et.e.input)
		et.e.input(et.e.context_data, et.fd);
	if((events.lNetworkEvents & FD_ACCEPT) && (et.mask & E_FD_READ) && et.e.input)
		et.e.input(et.e.context_data, et.fd);
	if((events.lNetworkEvents & FD_CONNECT) && (et.mask & E_FD_READ) && et.e.input)
		et.e.input(et.e.context_data, et.fd);
	if((events.lNetworkEvents & FD_CLOSE) && (et.mask & E_FD_EXCEPTION) && et.e.input)
		et.e.exception(et.e.context_data, et.fd);

	if((events.lNetworkEvents & FD_WRITE) && (et.mask & E_FD_WRITE) && et.e.output)
		et.e.output(et.e.context_data, et.fd);
}

/**
 * @brief �����̺߳���
 */
static void * listerner_thread_fun(void * param)
{
	struct timeval tv_now = {0};
	struct timeval tv_earliest = {0};
	HANDLE htemp[MAXIMUM_WAIT_OBJECTS] = {0};
	HANDLE hsrc = NULL;
	mylisterner_t * l = (mylisterner_t *)param;
	int ret = 0;

	while(l->bNotExit)
	{
		LOG_DEBUG(("win listerner process begin\r\n"));

		listerner_get_handle(l, htemp, sizeof(htemp)/sizeof(htemp[0]));

		if(0 == MyTimerHeapGetEarliestExpire(l->tmhp, (mytv_t *)&tv_earliest))
		{
			gettimeofday(&tv_now, NULL);
			if(timeval_smaller(tv_now, tv_earliest))
			{
				timeval_minus(tv_earliest, tv_now);
				
				ret = WaitForMultipleObjects((unsigned long)l->hcount, htemp, 0,
					tv_earliest.tv_sec * 1000 + tv_earliest.tv_usec/1000);
			}
			else
			{
				ret = -1;
				memset(&tv_earliest, 0, sizeof(tv_earliest));
				LOG_INFO(("null loop"));
			}
		}
		else
			ret = WaitForMultipleObjects((unsigned long)l->hcount, htemp, 0, INFINITE);

		if(-1 == ret || ret >= sizeof(l->harray)/sizeof(l->harray[0]))
		{
			/* û�з����¼�,ֻ���г�ʱ�¼� */
			listerner_process_timeout(l);
			continue;
		}

		/* ȡ�������¼��ľ��,���ݴ����¼�Դ���д��� */
		hsrc = l->harray[ret];

		if(hsrc == (HANDLE)MyEventWin32GetHandle(l->promq_notifier))
		{
			/* ��������ȼ�����Ϣ���е���Ϣ */
			listerner_process_promsg(l);
		}
		else if(hsrc == (HANDLE)MyEventWin32GetHandle(l->mq_notifier))
		{
			/* ���������ȼ�����Ϣ���е���Ϣ */
			listerner_process_msg(l);
		}
		else if(hsrc == (HANDLE)MyEventWin32GetHandle(l->handleset_notifier))
		{
			LOG_DEBUG(("fdset event"));
		}
		else if(hsrc == (HANDLE)MyEventWin32GetHandle(l->tmhp_notifier))
		{
			LOG_DEBUG(("timer event"));
		}
		else
		{
			/* ���������ȱ���ľ����Ϣ */
			listerner_dispatch_handles(l, hsrc);
		}
	}

	return NULL;
}

/**
 * @brief ����ص���Ĺ�ϣ����
 */
static size_t handle_table_hashfun(const void * key)
{
	return (size_t)key;
}

/**
 * @brief ����ص���Ĺ�ϣֵ�ȽϺ���
 */
static int handle_table_equalfun(const void * key1, const void * key2)
{
	return (key1 == key2);
}


/**
 * @brief ��������߳�
 */
HMYLISTERNER MyListernerConstruct(HMYMEMPOOL hm, size_t max_msg_count)
{
	mylisterner_t * l = MyMemPoolMalloc(hm, sizeof(*l));
	assert(l);

	l->hm = hm;
	l->bNotExit = 1;

	/* ��ʱ�� */
	l->tmhp = MyTimerHeapConstruct(hm);
	l->tmhp_notifier = MyEventConstruct(hm);
	assert(l->tmhp_notifier && l->tmhp);

	/* �������ı����� */
	l->handles_protect = MyMutexConstruct(hm);
	assert(l->handles_protect);
	l->handleset_notifier = MyEventConstruct(hm);
	assert(l->handleset_notifier);
	l->handles_dispatch_table = MyHashMapConstruct(hm,
		handle_table_hashfun, handle_table_equalfun, 0, NULL, NULL);
	assert(l->handles_dispatch_table);
	l->hcount = 0;

	/* ������Ϣ����,�Լ���Ϣ���е�֪ͨ�ź� */
	l->mq = MyMsgQueConstruct(hm, max_msg_count);
	l->mq_notifier = MyEventConstruct(hm);
	assert(l->mq && l->mq_notifier);

	/* �������ȼ���Ϣ����,�Լ���Ϣ���е�֪ͨ�ź� */
	l->promq = MyProrityMsgQueConstruct(hm, max_msg_count);
	l->promq_notifier = MyEventConstruct(hm);
	assert(l->promq && l->promq_notifier);

	/* �����߳� */
	l->thr = MyThreadConstruct(listerner_thread_fun, l, 1, hm);
	assert(l->thr);

	listerner_add_handle(l, (HANDLE)MyEventWin32GetHandle(l->promq_notifier), NULL);
	listerner_add_handle(l, (HANDLE)MyEventWin32GetHandle(l->mq_notifier), NULL);
	listerner_add_handle(l, (HANDLE)MyEventWin32GetHandle(l->tmhp_notifier), NULL);
	listerner_add_handle(l, (HANDLE)MyEventWin32GetHandle(l->handleset_notifier), NULL);

	return l;
}

/**
 * @brief ���������߳�
 */
void MyListernerDestruct(HMYLISTERNER l)
{
	unsigned int i = 0;
	evt_tag et = {0};

	assert(l);
	
	l->bNotExit = 0;
	MyEventSetSignaled(l->handleset_notifier);
	MyThreadJoin(l->thr);

	/* ȡ�������о����select */
	for(i = 0; i < l->hcount; i ++)
	{
		listerner_query_handle(l, l->harray[i], &et);
		WSAEventSelect(et.fd, l->harray[i], 0);
	}
	
	if(l->thr)
		MyThreadDestruct(l->thr);
	l->thr = NULL;
		
	if(l->mq)
		MyMsgQueDestruct(l->mq);
	l->mq = NULL;

	if(l->mq_notifier)
		MyEventDestruct(l->mq_notifier);
	l->mq_notifier = NULL;

	if(l->promq)
		MyProrityMsgQueDestruct(l->promq);
	l->promq = NULL;

	if(l->promq_notifier)
		MyEventDestruct(l->promq_notifier);
	l->promq_notifier = NULL;

	if(l->tmhp)
		MyTimerHeapDestruct(l->tmhp);
	l->tmhp = NULL;

	if(l->tmhp_notifier)
		MyEventDestruct(l->tmhp_notifier);
	l->tmhp_notifier = NULL;
	
	//����
	MyMutexLock(l->handles_protect);

	if(l->handles_dispatch_table)
		MyHashMapDestruct(l->handles_dispatch_table);
	l->handles_dispatch_table = NULL;

	//����
	MyMutexUnLock(l->handles_protect);

	if(l->handles_protect)
		MyMutexDestruct(l->handles_protect);
	l->handles_protect = NULL;

	if(l->handleset_notifier)
		MyEventDestruct(l->handleset_notifier);
	l->handleset_notifier = NULL;

	MyMemPoolFree(l->hm, l);
}

/**
 * @brief ���м����߳�
 */
void MyListernerRun(HMYLISTERNER hlisterner)
{
	if(NULL == hlisterner)
		return;

	assert(hlisterner->thr);
	MyThreadRun(hlisterner->thr);
}

/**
 * @brief �ȴ�listern�߳��˳�
 */
void MyListernerWait(HMYLISTERNER hlisterner)
{
	if(NULL == hlisterner)
		return;

	assert(hlisterner->thr);
	MyThreadJoin(hlisterner->thr);
}

/**
 * @brief ��Ӷ�ʱ��
 */
HTIMERID MyListernerAddTimer(HMYLISTERNER l, mytimer_node_t * node)
{
	HTIMERID timer_id = NULL;
	if(NULL == l || NULL == node || NULL == l->tmhp)
		return NULL;

	LOG_DEBUG(("listerner add timer sec:%d usec:%d", node->first_expire.tv_sec, node->first_expire.tv_usec));

	gettimeofday((struct timeval *)&node->abs_expire, NULL);
	timeval_add(node->abs_expire, node->first_expire);

	/* �����,�ٻ��� */
	timer_id = MyTimerHeapAdd(l->tmhp, node);

	/* ������� <�µĳ�ʱ> �� <��ǰ��̵ĳ�ʱ> ��Ҫ��,��ʾ��Ҫ���Ѷ�ʱ���߳� */
	if(MyTimeHeapGetEarliestKey(l->tmhp) == timer_id)
	{
		MyEventSetSignaled(l->tmhp_notifier);

		LOG_DEBUG(("write timer-add notify, timer id:%x", timer_id));
	}

	return timer_id;
}

/**
 * @brief ɾ����ʱ��
 */
int MyListernerDelTimer(HMYLISTERNER l, HTIMERID timer_id)
{
	if(NULL == l || NULL == l->tmhp)
		return -1;

	return MyTimerHeapDel(l->tmhp, timer_id);
}

/**
 * @brief ���ö�ʱ��
 */
HTIMERID MyListernerResetTimer(HMYLISTERNER l, HTIMERID timer_id, mytimer_node_t * node)
{
	if(NULL == l || NULL == l->tmhp)
		return NULL;

	gettimeofday((struct timeval *)&node->abs_expire, NULL);
	timeval_add(node->abs_expire, node->first_expire);

	timer_id = MyTimerHeapReset(l->tmhp, timer_id, node);

	if(NULL == timer_id)
		return NULL;

	/* ������� <�µĳ�ʱ> �� <��ǰ��̵ĳ�ʱ> ��Ҫ��,��ʾ��Ҫ���Ѷ�ʱ���߳� */
	if(MyTimeHeapGetEarliestKey(l->tmhp) == timer_id)
	{
		MyEventSetSignaled(l->tmhp_notifier);

		LOG_DEBUG(("write timer-add reset notify, timer id:%x", timer_id));
	}

	return timer_id;
}

/**
 * @brief ����ļ�ɨ���
 */
int MyListernerAddFD(HMYLISTERNER hl, int fd, enum E_HANDLE_SET_MASK mask, event_handle_t * evt_handle)
{
	/*
	* ����windows����HANDLE��ʵ��ͳһ��̽����
	* ������socket��һ��handle����
	*/

	WSAEVENT he = NULL;
	evt_tag et;

	if(NULL == hl || NULL == evt_handle || (int)INVALID_FD == fd)
		return -1;

	memcpy(&et.e, evt_handle, sizeof(et.e));
	et.mask = mask;
	et.fd = fd;

	he = WSACreateEvent();
	if(INVALID_HANDLE_VALUE == he)
		return -1;

	if(0 != WSAEventSelect(fd, he, FD_OOB | FD_READ | FD_WRITE | FD_ACCEPT | FD_CONNECT | FD_CLOSE))
		goto MyListernerAddFD_err_;

	listerner_add_handle(hl, he, &et);

	return 0;

MyListernerAddFD_err_:

	WSACloseEvent(he);

	return -1;
}

/**
 * @brief ɾ���ļ�ɨ���
 */
int MyListernerDelFD(HMYLISTERNER hl, int fd)
{
	HANDLE h = INVALID_HANDLE_VALUE;

	if(NULL == hl || (int)INVALID_FD == fd)
		return -1;

	if(0 != listerner_query_handle_by_fd(hl, fd, NULL, &h))
		return -1;

	if(INVALID_HANDLE_VALUE == h)
		return -1;

	/* ȡ������ */
	if(0 != WSAEventSelect(fd, h, 0))
		return -1;

	/* �ӻص�����ɾ����Ӧ�ļ�¼ */
	listerner_del_handle(hl, h);

	return 0;
}

/**
 * @brief ���һ����Ϣ 
 */
int MyListernerAddMsg(HMYLISTERNER l,
					  const void * user_msg,
					  unsigned long context_data,
					  CB_LISTERNER_HANDLE_MSG handle)
{
	msg_tag * msg = NULL;

	if(NULL == l || NULL == l->mq)
		return -1;

	/* �����Ϣ */
	msg = (msg_tag *)MyMemPoolMalloc(l->hm, sizeof(*msg));
	if(NULL == msg)
		return -1;

	msg->context_data = context_data;
	msg->handle = handle;
	msg->msg = (void *)user_msg;

	MyMsgQuePush(l->mq, msg);

	/* ֪ͨ */
	MyEventSetSignaled(l->mq_notifier);
	
	return 0;
}

/**
 * @brief ���һ�������ȼ�����Ϣ
 */
int MyListernerAddProrityMsg(HMYLISTERNER l,
							 int prority,
							 const void * user_msg,
							 unsigned long context_data,
							 CB_LISTERNER_HANDLE_MSG handle)
{
	msg_tag * msg = NULL;
	int ret = 0;

	if(NULL == l || NULL == l->promq)
		return -1;

	/* �����Ϣ */
	msg = (msg_tag *)MyMemPoolMalloc(l->hm, sizeof(*msg));
	if(NULL == msg)
		return -1;

	msg->context_data = context_data;
	msg->handle = handle;
	msg->msg = (void *)user_msg;

	ret = MyProrityMsgQuePush(l->promq, prority, msg);

	/* ֪ͨ */
	if(0 == ret)
	{
		MyEventSetSignaled(l->promq_notifier);
	}
	else
		LOG_WARN(("add prority msg fail %d", ret));	
	
	return ret;
}

/*
*
* ��ȡ��ʱ������
*
*/
int MyListernerPrint(HMYLISTERNER hlisterner, char * pctemp, size_t sz)
{
	mylisterner_t * l = (mylisterner_t *)hlisterner;

	printf("listerner %x\r\n timer:%d msgq:%d\r\n", 
		(unsigned long)l, MyTimerHeapGetCount(l->tmhp),
		MyMsgQueGetCount(l->mq));

	return 	(int)MyTimerHeapGetCount(l->tmhp);
}




















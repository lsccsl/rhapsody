/*
*
*mymsgque.c 队列 lin shao chuan
*
*/
#include "mymsgque.h"

#include <assert.h>

#include "myutility.h"
#include "mylist.h"
#include "myevent.h"
#include "mymutex.h"


typedef struct __mymsgque_t_
{
	HMYMEMPOOL hm;

	//读写事件
	HMYEVENT write_evt;
	HMYEVENT read_evt;

	//共享保护锁
	HMYMUTEX mt_protect;

	HMYLIST list_q;

	//消息队列的最大消息数量与当前的消息数量
	size_t max_msg_count;
	size_t current_msg_count;
}mymsgque_t;


static __INLINE__ void mymsgque_destroy(mymsgque_t * q)
{
	if(NULL == q)
		return;

	//加锁
	MyMutexLock(q->mt_protect);

	if(q->list_q)
		MyListDestruct(q->list_q);
	q->list_q = NULL;

	if(q->read_evt)
		MyEventDestruct(q->read_evt);
	q->read_evt = NULL;

	if(q->write_evt)
		MyEventDestruct(q->write_evt);
	q->write_evt = NULL;

	//解锁
	MyMutexUnLock(q->mt_protect);

	if(q->mt_protect)
		MyMutexDestruct(q->mt_protect);
	q->mt_protect = NULL;

	MyMemPoolFree(q->hm, q);
}

static __INLINE__ void mymsgque_inter_push(mymsgque_t * q, const void * data)
{
	assert(q && q->list_q);

	MyListAddTail(q->list_q, data);
	
	q->current_msg_count ++;
}

static __INLINE__ void * mymsgque_inter_pop(mymsgque_t * q)
{
	assert(q && q->list_q);

	if(MyListIsEmpty(q->list_q))
		return NULL;

	q->current_msg_count --;
	return MyListPopHead(q->list_q);
}


/*
*
*创建消息队列
*
*/
HMYMSGQUE MyMsgQueConstruct(HMYMEMPOOL hm, size_t max_msg_count)
{
#define DEFAULT_MAX_MSG_COUNT 1024
	mymsgque_t * q = (mymsgque_t *)MyMemPoolMalloc(hm, sizeof(*q));
	if(NULL == q)
		return NULL;

	q->mt_protect = MyMutexConstruct(hm);
	q->read_evt = MyEventConstruct(hm);
	q->write_evt = MyEventConstruct(hm);
	q->list_q = MyListConstruct(hm);
	q->max_msg_count = max_msg_count;
	if(0 == q->max_msg_count)
		q->max_msg_count = DEFAULT_MAX_MSG_COUNT;
	q->current_msg_count = 0;
	q->hm = hm;

	if(NULL == q->list_q || NULL == q->mt_protect || NULL == q->read_evt || NULL == q->write_evt)
		goto MyMsgQueConstruct_err_;

	return (HMYMSGQUE)q;

MyMsgQueConstruct_err_:

	mymsgque_destroy(q);

	return NULL;
}

/*
*
*销毁消息队列
*
*/
void MyMsgQueDestruct(HMYMSGQUE hmq)
{
	mymsgque_t * q = (mymsgque_t *)hmq;
	if(NULL == q)
		return;

	mymsgque_destroy(q);
}

/*
*
*添加消息至队尾,如果队列满了,则会阻塞,产生读通知
*
*/
void MyMsgQuePush_block(HMYMSGQUE hmq, const void * data)
{
	mymsgque_t * q = (mymsgque_t *)hmq;
	if(NULL == q)
		return;

	while(1)
	{
		if(0 != MyMutexLock(q->mt_protect))
			return;

		//防止消息队列已经被析构,
		if(NULL == q->list_q)
		{
			MyMutexUnLock(q->mt_protect);
			return;
		}
		else if(q->current_msg_count < q->max_msg_count)
		{
			MyMutexUnLock(q->mt_protect);
			break;
		}
		else
		{
			MyMutexUnLock(q->mt_protect);
			MyEventWait(q->write_evt, NULL);
		}
	}

	if(0 != MyMutexLock(q->mt_protect))
		return;
	
	if(NULL == q->list_q)
		goto MyMsgQuePush_block_end_;

	mymsgque_inter_push(q, data);

	assert(q->read_evt);
	//if(1 == q->current_msg_count)
		MyEventBroadCast(q->read_evt);

MyMsgQuePush_block_end_:

	MyMutexUnLock(q->mt_protect);
}

/*
*
*添加消息至队尾,不产生读通知
*
*/
void MyMsgQuePush(HMYMSGQUE hmq, const void * data)
{
	mymsgque_t * q = (mymsgque_t *)hmq;
	if(NULL == q)
		return;

	if(0 != MyMutexLock(q->mt_protect))
		return;
	
	if(NULL == q->list_q)
		goto MyMsgQuePush_block_end_;

	mymsgque_inter_push(q, data);

MyMsgQuePush_block_end_:

	MyMutexUnLock(q->mt_protect);
}

/*
*
*取队头消息,如果队列为空,则会阻塞,产生写通知
*
*/
void * MyMsgQuePop_block(HMYMSGQUE hmq)
{
	void * ret_data = NULL;
	mymsgque_t * q = (mymsgque_t *)hmq;
	if(NULL == q)
		return NULL;

	while(1)
	{
		if(0 != MyMutexLock(q->mt_protect))
			return NULL;

		//防止消息队列已经被析构,
		if(NULL == q->list_q)
		{
			MyMutexUnLock(q->mt_protect);
			return NULL;
		}
		else if(!MyListIsEmpty(q->list_q))
		{
			MyMutexUnLock(q->mt_protect);
			break;
		}
		else
		{
			MyMutexUnLock(q->mt_protect);
			MyEventWait(q->read_evt, NULL);
		}
	}

	if(0 != MyMutexLock(q->mt_protect))
		return NULL;

	if(NULL == q->list_q)
		goto MyMsgQuePop_block_end_;

	ret_data = mymsgque_inter_pop(q);

	assert(q->write_evt);
	//if(q->current_msg_count == q->max_msg_count - 1)
		MyEventBroadCast(q->write_evt);

MyMsgQuePop_block_end_:

	MyMutexUnLock(q->mt_protect);

	return ret_data;
}

/*
*
*取队头消息,不产生写通知
*
*/
void * MyMsgQuePop(HMYMSGQUE hmq)
{
	void * ret_data = NULL;
	mymsgque_t * q = (mymsgque_t *)hmq;
	if(NULL == q)
		return NULL;

	if(0 != MyMutexLock(q->mt_protect))
		return NULL;

	if(NULL == q->list_q)
		goto MyMsgQuePop_block_end_;

	ret_data = mymsgque_inter_pop(q);

MyMsgQuePop_block_end_:

	MyMutexUnLock(q->mt_protect);

	return ret_data;
}

/*
*
*取得消息的数量
*
*/
size_t MyMsgQueGetCount(HMYMSGQUE hmq)
{
	size_t ret = 0;
	mymsgque_t * q = (mymsgque_t *)hmq;
	if(NULL == q)
		return 0;

	assert(q->list_q && q->mt_protect && q->read_evt && q->write_evt);

	if(0 != MyMutexLock(q->mt_protect))
		return 0;

	if(q)
		ret = q->current_msg_count;

	MyMutexUnLock(q->mt_protect);

	return ret;
}


















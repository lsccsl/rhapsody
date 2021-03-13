/*
*
*myproritymsgque.c 优先级队列 lin shao chuan
*
*/
#include "myproritymsgque.h"

#include <assert.h>

#include "myutility.h"
#include "myvector.h"
#include "mymutex.h"
#include "myevent.h"


typedef struct __mypromq_t_
{
	HMYMEMPOOL hm;
	HMYMUTEX protecter;

	HMYEVENT write_evt;
	HMYEVENT read_evt;
	HMYVECTOR hv;

	size_t max_msg_count;
}mypromq_t;

typedef struct __myproritymsg_tag_
{
	int prority;
	void * userdata;
}myproritymsg_tag;


/*
*
*1表示 data1 > data2
*0表示 !(data1 > data2)
*
*/
static __INLINE__ int myprority_compare(const void * data1, const void * data2, const void * context)
{
	myproritymsg_tag * t1 = (myproritymsg_tag *)data1;
	myproritymsg_tag * t2 = (myproritymsg_tag *)data2;

	assert(t1 && t2);

	return t1->prority - t2->prority;
}

static __INLINE__ void myproritymq_destroy(mypromq_t * q)
{
	if(NULL == q)
		return;
	
	MyMutexLock(q->protecter);

	if(q->hv)
		MyVectorDestruct(q->hv);
	q->hv = NULL;

	if(q->read_evt)
		MyEventDestruct(q->read_evt);
	q->read_evt = NULL;

	if(q->write_evt)
		MyEventDestruct(q->write_evt);
	q->write_evt = NULL;

	MyMutexUnLock(q->protecter);

	if(q->protecter)
		MyMutexDestruct(q->protecter);

	MyMemPoolFree(q->hm, q);
}

static __INLINE__ int myproritymq_inter_push(mypromq_t * q, int prority, const void * data)
{
	assert(q && q->hv);
	
	{
		myproritymsg_tag t = {prority, (void *)data};
		return MyVectorHeapPush(q->hv, &t, sizeof(t));
	}
}

static __INLINE__ void * myproritymq_inter_pop(mypromq_t * q)
{
	void * data = NULL;
	size_t q_msg_count = 0;
	assert(q && q->hv);

	q_msg_count = MyVectorGetCount(q->hv);

	if(0 == q_msg_count)
		return NULL;

	MyVectorHeapPop(q->hv);
	{
		myproritymsg_tag * t = 
			(myproritymsg_tag *)MyVectorGetIndexData(q->hv, q_msg_count - 1, NULL);

		data = t->userdata;
	}
	MyVectorDel(q->hv, q_msg_count - 1);

	return data;
}


/*
*
*创建优先级队列
*
*/
HMY_PRO_MQ MyProrityMsgQueConstruct(HMYMEMPOOL hm, size_t max_msg_count)
{
	mypromq_t * q = (mypromq_t *)MyMemPoolMalloc(hm, sizeof(*q));
	if(NULL == q)
		return NULL;

	q->hm = hm;
	q->max_msg_count = max_msg_count;
	if(!q->max_msg_count)
		q->max_msg_count = 1024;

	q->protecter = MyMutexConstruct(hm);
	q->read_evt = MyEventConstruct(hm);
	q->write_evt = MyEventConstruct(hm);
	q->hv = MyVectorConstruct(hm, 0, NULL, myprority_compare);

	if(NULL == q->hv || NULL == q->protecter || NULL == q->read_evt || NULL == q->write_evt)
		goto MyProrityMsgQueConstruct_err_;

	return (HMY_PRO_MQ)q;

MyProrityMsgQueConstruct_err_:

	myproritymq_destroy(q);

	return NULL;
}

/*
*
*销毁优先级队列
*
*/
void MyProrityMsgQueDestruct(HMY_PRO_MQ hpmq)
{
	mypromq_t * q = (mypromq_t *)hpmq;
	if(NULL == q)
		return;

	myproritymq_destroy(q);
}

/*
*
*添加一条消息,如果队列满,则会阻塞,产生读通知
*
*/
int MyProrityMsgQuePush_block(HMY_PRO_MQ hpmq, int prority, const void * data)
{
	int ret = 0;
	mypromq_t * q = (mypromq_t *)hpmq;
	if(NULL == q)
		return -1;

	assert(q->protecter);

	while(1)
	{
		if(0 != MyMutexLock(q->protecter))
			return -1;

		if(NULL == q->hv)
		{
			MyMutexUnLock(q->protecter);
			return -1;
		}
		else if(MyVectorGetCount(q->hv) < q->max_msg_count)
		{
			MyMutexUnLock(q->protecter);
			break;
		}
		else
		{
			MyMutexUnLock(q->protecter);
			MyEventWait(q->write_evt, NULL);
		}
	}

	if(0 != MyMutexLock(q->protecter))
		return -1;

	if(NULL == q->hv)
		goto MyProrityMsgQuePush_end_;

	ret = myproritymq_inter_push(q, prority, data);

	assert(q->read_evt);
	if(MyVectorGetCount(q->hv) == 1)
		MyEventSetSignaled(q->read_evt);

MyProrityMsgQuePush_end_:
	MyMutexUnLock(q->protecter);

	return ret;
}

/*
*
*添加一条消息,如果队列满,函数返回,不产生读通知
*
*/
int MyProrityMsgQuePush(HMY_PRO_MQ hpmq, int prority, const void * data)
{
	int ret = 0;
	mypromq_t * q = (mypromq_t *)hpmq;
	if(NULL == q)
		return -1;

	assert(q->protecter);

	if(0 != MyMutexLock(q->protecter))
		return -1;

	if(NULL == q->hv)
		goto MyProrityMsgQuePush_end_;

	if(MyVectorGetCount(q->hv) < q->max_msg_count)
		ret = myproritymq_inter_push(q, prority, data);
	else
		ret = -1;

MyProrityMsgQuePush_end_:
	MyMutexUnLock(q->protecter);

	return ret;
}

/*
*
*取出一条优先级最高的消息,如果队列为空,则会阻塞,产生写通知
*
*/
void * MyProrityMsgQuePop_block(HMY_PRO_MQ hpmq)
{
	void * data = NULL;
	mypromq_t * q = (mypromq_t *)hpmq;
	if(NULL == q)
		return NULL;

	assert(q->protecter);

	while(1)
	{
		if(0 != MyMutexLock(q->protecter))
			return NULL;

		if(NULL == q->hv)
		{
			MyMutexUnLock(q->protecter);
			return NULL;
		}
		else if(MyVectorGetCount(q->hv))
		{
			MyMutexUnLock(q->protecter);
			break;
		}
		else
		{
			MyMutexUnLock(q->protecter);
			MyEventWait(q->read_evt, NULL);
		}
	}

	if(0 != MyMutexLock(q->protecter))
		return NULL;

	if(NULL == q->hv)
		goto MyProrityMsgQuePop_end_;

	data = myproritymq_inter_pop(q);

	assert(q->write_evt);
	if(MyVectorGetCount(q->hv) == q->max_msg_count - 1)
		MyEventSetSignaled(q->write_evt);

MyProrityMsgQuePop_end_:

	MyMutexUnLock(q->protecter);

	return data;
}

/*
*
*取出一条优先级最高的消息,如果队列为空,返回null,不产生写通知
*
*/
void * MyProrityMsgQuePop(HMY_PRO_MQ hpmq)
{
	void * data = NULL;
	mypromq_t * q = (mypromq_t *)hpmq;
	if(NULL == q)
		return NULL;

	assert(q->protecter);

	if(0 != MyMutexLock(q->protecter))
		return NULL;

	if(NULL == q->hv)
		goto MyProrityMsgQuePop_end_;

	data = myproritymq_inter_pop(q);

MyProrityMsgQuePop_end_:

	MyMutexUnLock(q->protecter);

	return data;
}

/*
*
*取出一条消息
*
*/
size_t MyProrityMsgQueGetCount(HMY_PRO_MQ hpmq)
{
	size_t ret = 0;
	mypromq_t * q = (mypromq_t *)hpmq;
	if(NULL == q)
		return 0;

	if(0 != MyMutexLock(q->protecter))
		return 0;

	if(NULL == q->hv)
		goto MyProrityMsgQueGetCount_end_;

	assert(q->hv && q->read_evt && q->write_evt);

	ret = MyVectorGetCount(q->hv);

MyProrityMsgQueGetCount_end_:

	MyMutexUnLock(q->protecter);

	return ret;
}












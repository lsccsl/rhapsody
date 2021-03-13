/*
*
*mytimerheap.c 定时堆 lin shao chuan
*
*/
#include "mytimerheap.h"

#include <assert.h>
#include <string.h>

#include "myutility.h"
#include "myheap.h"
#include "mymutex.h"
#include "mylog.h"


typedef struct __mytimerheap_t_
{
	HMYMEMPOOL hm;

	HMYHEAP hp;
	HMYMUTEX protecter;
}mytimerheap_t;


/*
*
*1表示 data1 > data2
*0表示 !(data1 > data2)
*
*/
static __INLINE__ int timer_heap_compare(const void * data1, const void * data2, const void * context)
{
	mytimer_node_t * node1 = (mytimer_node_t *)data1;
	mytimer_node_t * node2 = (mytimer_node_t *)data2;

	assert(node1 && node2);

	//超时时间越近,权值越大
	if(node1->abs_expire.tv_sec < node2->abs_expire.tv_sec)
		return 1;
	else if(node1->abs_expire.tv_sec == node2->abs_expire.tv_sec)
	{
		if(node1->abs_expire.tv_usec <= node2->abs_expire.tv_usec)
			return 1;
		else
			return -1;
	}
	else
		return -1;
}


static __INLINE__ void mytimerheap_inter_destroy(mytimerheap_t * th)
{
	if(NULL == th)
		return;

	MyMutexLock(th->protecter);

	if(th->hp)
		MyHeapDestruct(th->hp);
	th->hp = NULL;

	MyMutexUnLock(th->protecter);

	MyMutexDestruct(th->protecter);
	th->protecter = NULL;

	MyMemPoolFree(th->hm, th);
}


/*
*
* 构造定时堆
*
*/
HMYTIMERHEAP MyTimerHeapConstruct(HMYMEMPOOL hm)
{
	mytimerheap_t * th = (mytimerheap_t *)MyMemPoolMalloc(hm, sizeof(*th));
	if(NULL == th)
		return NULL;

	th->hm = hm;

	th->hp = MyHeapConstruct(hm, 0, NULL, timer_heap_compare);
	th->protecter = MyMutexConstruct(th->hm);

	if(NULL == th->hp || NULL == th->protecter)
		goto MyTimerHeapConstruct_err_;

	return (HMYTIMERHEAP)th;

MyTimerHeapConstruct_err_:

	mytimerheap_inter_destroy(th);

	return NULL;
}

/*
*
* 析构定时堆
*
*/
void MyTimerHeapDestruct(HMYTIMERHEAP hth)
{
	mytimerheap_t * th = (mytimerheap_t *)hth;
	if(NULL == th)
		return;

	mytimerheap_inter_destroy(th);
}

/*
*
* 添加定时器
*
*@param 
	mytimer_node_t * node:定时器节点
*@retval 定时器的id NULL:失败
*
*/
HTIMERID MyTimerHeapAdd(HMYTIMERHEAP hth, mytimer_node_t * node)
{
	HTIMERID key = NULL;
	mytimerheap_t * th = (mytimerheap_t *)hth;
	if(NULL == th || NULL == node || NULL == node->timeout_cb)
		return NULL;

	if(0 != MyMutexLock(th->protecter))
		return NULL;

	if(NULL == th->hp)
		goto MyTimerHeapAdd_end_;

	key = (HTIMERID)MyHeapPush(th->hp, node, sizeof(*node));

MyTimerHeapAdd_end_:

	MyMutexUnLock(th->protecter);

	return key;
}

/*
*
* 删除定时器
*
*/
int MyTimerHeapDel(HMYTIMERHEAP hth, HTIMERID timerid)
{
	int ret = 0;
	mytimerheap_t * th = (mytimerheap_t *)hth;
	if(NULL == th)
		return -1;

	if(0 != MyMutexLock(th->protecter))
		return -1;

	if(NULL == th->hp)
		goto MyTimerHeapDel_end_;

	ret = MyHeapDel(th->hp, (HMYHEAP_KEY)timerid);

MyTimerHeapDel_end_:

	MyMutexUnLock(th->protecter);

	return ret;
}

/*
*
* 重置定时器
*
*@param first_expire:第一次超时(绝对时间) period:定时触发的间隔
*@retval 0:成功 -1:失败
*
*/
HTIMERID MyTimerHeapReset(HMYTIMERHEAP hth, HTIMERID timerid, mytimer_node_t * node)
{
	mytimerheap_t * th = (mytimerheap_t *)hth;
	if(NULL == th)
		return NULL;

	assert(node && node->timeout_cb);

	if(0 != MyMutexLock(th->protecter))
		return NULL;

	if(NULL == th->hp)
		goto MyTimerHeapReset_end_;

	timerid = (HTIMERID)MyHeapUpdate(th->hp, (HMYHEAP_KEY)timerid, node, sizeof(*node));

MyTimerHeapReset_end_:

	MyMutexUnLock(th->protecter);

	return timerid;
}

/*
*
* 获取头节点,即绝对超时最小(最早超时)的键值
*
*/
HTIMERID MyTimeHeapGetEarliestKey(HMYTIMERHEAP hth)
{
	HTIMERID timerid = NULL;
	mytimerheap_t * th = (mytimerheap_t *)hth;
	if(NULL == th)
		return NULL;

	if(0 != MyMutexLock(th->protecter))
		return NULL;

	if(th->hp)
		timerid = (HTIMERID)MyHeapFrontKey(th->hp);

	MyMutexUnLock(th->protecter);

	return timerid;
}

/*
*
* 获取最小超时(绝对时间)
*
*@retval 0:成功 -1:失败
*
*/
int MyTimerHeapGetEarliestExpire(HMYTIMERHEAP hth, mytv_t * expire)
{
	int ret = -1;
	mytimerheap_t * th = (mytimerheap_t *)hth;
	if(NULL == th)
		return -1;

	assert(expire);

	if(0 != MyMutexLock(th->protecter))
		return -1;

	if(NULL == th->hp)
		goto MyTimerHeapGetEarliesExpire_end_;

	{
		size_t data_size = 0;

		mytimer_node_t * n = (mytimer_node_t *)MyHeapFront(th->hp, &data_size);

		if(n)
		{
			assert(sizeof(*n) == data_size);
			*expire = n->abs_expire;
			ret = 0;
		}
		else
			ret = -1;
	}

MyTimerHeapGetEarliesExpire_end_:

	MyMutexUnLock(th->protecter);

	return ret;
}

/*
*
* 触发所有超时事件
*
*@retval 超时个数
*
*/
unsigned MyTimerHeapRunExpire(HMYTIMERHEAP hth, mytv_t * tv_now)
{
	unsigned ret = 0;

	mytimerheap_t * th = (mytimerheap_t *)hth;
	if(NULL == th)
		return 0;

	assert(tv_now);

	if(NULL == th->hp)
		return 0;

	if(0 != MyMutexLock(th->protecter))
		return 0;

	for(;;)
	{
		size_t data_size = 0;
		unsigned long context_data = 0;
		unsigned long timer_user_data = 0;
		MY_TIMER_HEAP_TIMEOUT_CB timeout_cb = NULL;
		mytimer_node_t n_min = {0};
		mytimer_node_t * pn =	NULL;

		//依次取出堆中的节点
		HMYHEAP_KEY key =  MyHeapFrontKey(th->hp);
		pn = (mytimer_node_t *)MyHeapFront(th->hp, &data_size);
		//堆中没有节点,结束
		if(NULL == pn)
			break;

		assert(sizeof(n_min) == data_size);
		memcpy(&n_min, pn, data_size);

		context_data = n_min.context_data;
		timer_user_data = n_min.timer_user_data;
		timeout_cb = n_min.timeout_cb;

		//最近的超时未到,说明之后的节点均未超时
		if(timeval_smaller(*tv_now, n_min.abs_expire))
			break;

		//是否是周期性触发
		if(n_min.period.tv_sec || n_min.period.tv_usec)
		{
			//防止已经落后了好几个周期的情况出现 --- 优化此代码,在某些回调函数执行需要很长,而定时间隔又很短的场合,这里要花费大量的时间计算
			do
			{
				timeval_add(n_min.abs_expire, n_min.period);
			}while(timeval_smaller(n_min.abs_expire, *tv_now));
		
			MyHeapUpdate(th->hp, key, &n_min, sizeof(n_min));
		}
		else
		{
			MyHeapPop(th->hp);
		}

		/*
		* 呼叫回调函数,呼叫前解锁
		*/
		MyMutexUnLock(th->protecter);
		if(timeout_cb)
			timeout_cb(context_data, timer_user_data, key);
		if(0 != MyMutexLock(th->protecter))
			break;

		ret ++;
	}

	MyMutexUnLock(th->protecter);

	return ret;
}

/*
*
* 获取定时器的个数
*
*/
size_t MyTimerHeapGetCount(HMYTIMERHEAP hth)
{
	size_t ret = 0;
	mytimerheap_t * th = (mytimerheap_t *)hth;
	if(NULL == th)
		return 0;

	if(0 != MyMutexLock(th->protecter))
		return 0;

	if(th->hp)
		ret = MyHeapGetCount(th->hp);

	MyMutexUnLock(th->protecter);

	return ret;
}

/*
*
* 输出timer heap里的信息
*
*/
void MyTimerHeapPrint(HMYTIMERHEAP hth)
{
	mytimerheap_t * th = (mytimerheap_t *)hth;
	mytimer_node_t * n = NULL;
	size_t i = 0;

	if(0 != MyMutexLock(th->protecter))
		return;

	for(; i < MyVectorGetCount((HMYVECTOR)th->hp); i ++)
	{
		n = MyVectorGetIndexData((HMYVECTOR)th->hp, i, NULL);
		LOG_DEBUG(("context_data:%d timer_user_data:%d timeout_cb:%x period:%d-%d first:%d-%d abs:%d-%d",
			n->context_data,
			n->timer_user_data,
			n->timeout_cb,
			n->period.tv_sec, n->period.tv_usec,
			n->first_expire.tv_sec, n->first_expire.tv_usec,
			n->abs_expire.tv_sec, n->abs_expire.tv_usec));
	}

	MyMutexUnLock(th->protecter);
}





















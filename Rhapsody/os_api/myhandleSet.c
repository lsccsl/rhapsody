/*
*
*myhandleSet.c fd集合 lin shao chuan
*
*/


#include "myhandleSet.h"

#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>

#include "myrbtree.h"
#include "mymutex.h"


typedef struct __myhandleSet_t_
{
	fd_set read_mask;
	fd_set read_mask_signaled;
	int read_size;

	fd_set write_mask;
	fd_set write_mask_signaled;
	int write_size;

	fd_set exception_mask;
	fd_set exception_mask_signaled;
	int exception_size;

	HMYRBTREE hfdtable;	

	HMYMEMPOOL hm;

	HMYMUTEX protecter;
}myhandleSet_t;


/*
*
*1 表示 key1 比 key2 大
*0 表示 key1 比 key2 小 
*
*/
static __INLINE__ int handle_rbtree_compare(const void * key1, const void * key2)
{
	return key1 > key2;
}

static __INLINE__ void destroy(myhandleSet_t * hset)
{
	assert(hset);

	MyMutexLock(hset->protecter);

	if(hset->hfdtable)
		MyRBTreeDestruct(hset->hfdtable);

	MyMutexUnLock(hset->protecter);

	MyMutexDestruct(hset->protecter);

	MyMemPoolFree(hset->hm, hset);
}


/*
*
*创建
*
*/
HMYHANDLESET MyHandleSetConstruct(HMYMEMPOOL hm)
{
	myhandleSet_t * hset = (myhandleSet_t *)MyMemPoolMalloc(hm, sizeof(*hset));
	if(NULL == hset)
		return NULL;

	hset->hm = hm;
	hset->hfdtable = MyRBTreeConstruct(hm, handle_rbtree_compare);
	hset->protecter = MyMutexConstruct(hm);
	if(NULL == hset->hfdtable || NULL == hset->protecter)
	{
		destroy(hset);
		return NULL;
	}
	
	FD_ZERO(&hset->read_mask);
	FD_ZERO(&hset->read_mask_signaled);
	hset->read_size = 0;

	FD_ZERO(&hset->write_mask);
	FD_ZERO(&hset->write_mask_signaled);
	hset->write_size = 0;

	FD_ZERO(&hset->exception_mask);
	FD_ZERO(&hset->exception_mask_signaled);
	hset->exception_size = 0;
	
	return (HMYHANDLESET)hset;
}

/*
*
*销毁
*
*/
void MyHandleSetDestruct(HMYHANDLESET hs)
{
	myhandleSet_t * hset = (myhandleSet_t *)hs;
	if(NULL == hset)
		return;

	destroy(hset);
}

/*
*
*添加一个fd
*@retval:
	0:成功
	-1:添加全部失败
	-2:添加读失败
	-3:添加写失败
	-4:添加异常失败
*
*/
int MyHandleSetFdSet(HMYHANDLESET hs, int fd, unsigned long mask)
{
	int ret = 0;
	myhandleSet_t * hset = (myhandleSet_t *)hs;
	if(NULL == hset)
		return -1;

	assert(hset->hfdtable);

	if(0 != MyMutexLock(hset->protecter))
		return -1;

	if(mask & E_FD_READ)
	{
		if(hset->read_size >= FD_SETSIZE)
		{
			ret = -2;
			goto MyHandleSetFdSet_end_;
		}

		FD_SET(fd, &hset->read_mask);
		hset->read_size = (fd >= hset->read_size)?(fd+1):hset->read_size;
	}
	else if(mask & E_FD_WRITE)
	{
		if(hset->write_size >= FD_SETSIZE)
		{
			ret = -3;
			goto MyHandleSetFdSet_end_;
		}

		FD_SET(fd, &hset->write_mask);
		hset->write_size = (fd >= hset->write_size)?(fd+1):hset->write_size;
	}
	else if(mask & E_FD_EXCEPTION)
	{
		if(hset->exception_size >= FD_SETSIZE)
		{
			ret = -4;
			goto MyHandleSetFdSet_end_;
		}

		FD_SET(fd, &hset->exception_mask);
		hset->exception_size = (fd >= hset->exception_size)?(fd+1):hset->exception_size;
	}

	MyRBTreeInsertUnique(hset->hfdtable, (void *)fd, (void *)mask);

MyHandleSetFdSet_end_:

	MyMutexUnLock(hset->protecter);
	return ret;
}

/*
*
*删除加一个fd
*
*/
void MyHandleSetDelFd(HMYHANDLESET hs, int fd)
{
	unsigned long mask = 0;
	HMYRBTREE_ITER it = NULL;
	myhandleSet_t * hset = (myhandleSet_t *)hs;
	if(NULL == hset)
		return;

	if(0 != MyMutexLock(hset->protecter))
		return;

	//找出fd的信息
	it = MyRBTreeSearch(hset->hfdtable, (void *)fd);
	mask = (unsigned long)MyRBTreeGetIterData(it);
	
	//从表中删除
	MyRBTreeDelIter(hset->hfdtable, it, NULL, NULL);
	
	//根据需要从相应的集合里删除
	if(mask & E_FD_READ)
	{
		FD_CLR(fd, &hset->read_mask);
		hset->read_size --;
		if(hset->read_size < 0)
			hset->read_size = 0;
	}
	else if(mask & E_FD_WRITE)
	{
		FD_CLR(fd, &hset->write_mask);
		hset->write_size --;
		if(hset->write_size < 0)
			hset->write_size = 0;
	}
	else if(mask & E_FD_EXCEPTION)
	{
		FD_CLR(fd, &hset->exception_mask);
		hset->exception_size --;
		if(hset->exception_size < 0)
			hset->exception_size = 0;
	}

	MyMutexUnLock(hset->protecter);
}

/*
*
*select
*@param
	timeout:超时，null表示无限等待
*@retval
	0:表示超时 非-1则表示发生事件的句柄数 -1:表示失败
*
*/
int MyHandleSetSelect(HMYHANDLESET hs, struct timeval * timeout)
{	
	int nfds = 0;

	myhandleSet_t * hset = (myhandleSet_t *)hs;
	if(NULL == hset)
		return -1;

	if(0 != MyMutexLock(hset->protecter))
		return -1;

	FD_ZERO(&hset->read_mask_signaled);
	FD_ZERO(&hset->write_mask_signaled);
	FD_ZERO(&hset->exception_mask_signaled);

	hset->read_mask_signaled = hset->read_mask;
	hset->write_mask_signaled = hset->write_mask;
	hset->exception_mask_signaled = hset->exception_mask;

	nfds = (hset->read_size > hset->write_size)?(hset->read_size > hset->exception_size ? hset->read_size:hset->exception_size):(hset->write_size > hset->exception_size ? hset->write_size:hset->exception_size);

	MyMutexUnLock(hset->protecter);

	return select(nfds,
		&hset->read_mask_signaled,
		&hset->write_mask_signaled,
		&hset->exception_mask_signaled,
		timeout);
}

/*
*
*取出所有发生事件的fd,并清空当前发生事件的fd集合
*@param
	hvRead:表示可读的fd集合
	hvWrite:表示可写的fd集合
	hvException:表示出错的fd集合
*
*/	
void MyHandleSetGetAllSignalFd(HMYHANDLESET hs,
	HMYVECTOR read_set,
	HMYVECTOR write_set,
	HMYVECTOR exception_set)
{
	/**
	*
	*ACE实现了直接从fd_set里取出发生事件的fd,
	*而不是循环的使用FD_ISSET找出发生事件的fd.
	*
	*注:<sys/select.h> <bits/select.h> 里fd_set操作的实现根据gcc
	*版本不同,实现也不同.而且有其它宏来决定fd_set相关操作的具体实现
	*
	*ACE的关于这一块的代码也暂时没有看懂.
	*
	*todo: help me!
	*
	**/

	HMYRBTREE_ITER it = NULL;
	myhandleSet_t * hset = (myhandleSet_t *)hs;
	if(NULL == hset)
		return;

	if(0 != MyMutexLock(hset->protecter))
		return;

	for(it = MyRBTreeBegin(hset->hfdtable); 
		it != NULL; 
		it = MyRBTreeGetNext(it))
	{
		int fd = (int)MyRBTreeGetIterKey(it);

		if(FD_ISSET(fd, &hset->read_mask_signaled) && read_set)
			MyVectorAdd(read_set, (void *)fd, 0);
		if(FD_ISSET(fd, &hset->write_mask_signaled) && write_set)
			MyVectorAdd(write_set, (void *)fd, 0);
		if(FD_ISSET(fd, &hset->exception_mask_signaled) && exception_set)
			MyVectorAdd(exception_set, (void *)fd, 0);
	}

	FD_ZERO(&hset->read_mask_signaled);
	FD_ZERO(&hset->write_mask_signaled);
	FD_ZERO(&hset->exception_mask_signaled);

	MyMutexUnLock(hset->protecter);
}

/*
*
*判断fd是否发生mask中指定的事件,并从相应的集合中删除fd
*
*/	
unsigned long MyHandleSetIsSet(HMYHANDLESET hs, int fd, unsigned long mask)
{
	unsigned long mask_ret = 0;
	myhandleSet_t * hset = (myhandleSet_t *)hs;
	if(NULL == hset)
		return 0;

	if(0 != MyMutexLock(hset->protecter))
		return 0;

	if((mask & E_FD_READ) && FD_ISSET(fd, &hset->read_mask_signaled))
	{
		mask_ret |= E_FD_READ;
		FD_CLR(fd, &hset->read_mask_signaled);
	}

	if((mask & E_FD_WRITE) && FD_ISSET(fd, &hset->write_mask_signaled))
	{
		mask_ret |= E_FD_WRITE;
		FD_CLR(fd, &hset->write_mask_signaled);
	}

	if((mask & E_FD_EXCEPTION) && FD_ISSET(fd, &hset->exception_mask_signaled))
	{
		mask_ret |= E_FD_EXCEPTION;
		FD_CLR(fd, &hset->exception_mask_signaled);
	}

	MyMutexUnLock(hset->protecter);

	return mask_ret;
}



















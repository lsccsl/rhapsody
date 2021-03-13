/**
 *
 * @file mynamepipe.c 命名管道 2007-8-23 23:41
 *
 * @author diablo 
 *
 */
#include "mynamepipe.h"

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>

#include "os_def.h"

#include "mybuffer.h"
#include "mylog.h"


typedef struct __mynamepipe_t_
{
	/*
	* 存放管道名字的缓冲区
	*/
	HMYBUFFER hbuf_name;
	
	/*
	* 记录是否是当前对象创建了这个管道,用于对象析构时判断是否unlink
	*/
	int bcreate;
	
	/*
	* 管道的fd
	*/
	int fd;
	
	/*
	* 内存池标识
	*/
	HMYMEMPOOL hm;
}mynamepipe_t;


/**
 * @brief 销毁命名管道
 */
static __INLINE__ void name_pipe_destroy(mynamepipe_t * p)
{
	assert(p);
	
	close(p->fd);
	if(p->bcreate && p->hbuf_name)
	{
		LOG_INFO(("unlink name pipe %s", MyBufferGet(p->hbuf_name, NULL)));
		unlink((char *)MyBufferGet(p->hbuf_name, NULL));
	}

	MyBufferDestruct(p->hbuf_name);
	
	MyMemPoolFree(p->hm, p);
}


/**
 * @brief 创建命名管道
 */
HMYNAMEPIPE MyNamePipeConstruct(HMYMEMPOOL hm, const char * name, int bcreate)
{
	mynamepipe_t * p = (mynamepipe_t *)MyMemPoolMalloc(hm, sizeof(*p));
	if(NULL == p)
		return NULL;
	
	p->hm = hm;
	p->bcreate = bcreate;
	
	if(NULL == name)
		goto MyNamePipeConstruct_end_;

	p->hbuf_name = MyBufferConstruct(hm, 0);
	if(NULL == p->hbuf_name)
		goto MyNamePipeConstruct_end_;

	MyBufferSet(p->hbuf_name, name, strlen(name));
	MyBufferAppend(p->hbuf_name, "\0", 1);

	/*
	* 创建管道
	*/
	if(bcreate)
	{
		unlink(name);
		if(mkfifo(name, 0666) != 0)
			goto MyNamePipeConstruct_end_;
	}

	/*
	* 打开管道句柄
	*/
	p->fd = open(name,IPC_CREAT|O_RDWR| O_NONBLOCK);
	if(-1 == p->fd)
		goto MyNamePipeConstruct_end_;
		
	return (HMYNAMEPIPE)p;

MyNamePipeConstruct_end_:
	
	name_pipe_destroy(p);
	
	return NULL;
}
 
/**
 * @brief 销毁命名管道
 */
void MyNamePipeDestruct(HMYNAMEPIPE hnp)
{
	mynamepipe_t * p = (mynamepipe_t *)hnp;
	if(NULL == p)
		return;

	name_pipe_destroy(p);
}

/**
 * @brief 获取管道的fd
 */
int MyNamePipeGetFd(HMYNAMEPIPE hnp)
{
	mynamepipe_t * p = (mynamepipe_t *)hnp;
	if(NULL == p)
		return INVALID_FD;
		
	return p->fd;
}
 
/**
 * @brief 写
 */
int MyNamePipeWrite(HMYNAMEPIPE hnp, const void * data, size_t data_len)
{
	mynamepipe_t * p = (mynamepipe_t *)hnp;
	if(NULL == p)
		return -1;
		
	return write(p->fd, data, data_len);
}
 
/**
 * @brief 读
 */
int MyNamePipeRead(HMYNAMEPIPE hnp, void * data, size_t data_len)
{
	mynamepipe_t * p = (mynamepipe_t *)hnp;
	if(NULL == p)
		return -1;

	return read(p->fd, data, data_len);
}








/**
 *
 * @file mynamepipe.c �����ܵ� 2007-8-23 23:41
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
	* ��Źܵ����ֵĻ�����
	*/
	HMYBUFFER hbuf_name;
	
	/*
	* ��¼�Ƿ��ǵ�ǰ���󴴽�������ܵ�,���ڶ�������ʱ�ж��Ƿ�unlink
	*/
	int bcreate;
	
	/*
	* �ܵ���fd
	*/
	int fd;
	
	/*
	* �ڴ�ر�ʶ
	*/
	HMYMEMPOOL hm;
}mynamepipe_t;


/**
 * @brief ���������ܵ�
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
 * @brief ���������ܵ�
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
	* �����ܵ�
	*/
	if(bcreate)
	{
		unlink(name);
		if(mkfifo(name, 0666) != 0)
			goto MyNamePipeConstruct_end_;
	}

	/*
	* �򿪹ܵ����
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
 * @brief ���������ܵ�
 */
void MyNamePipeDestruct(HMYNAMEPIPE hnp)
{
	mynamepipe_t * p = (mynamepipe_t *)hnp;
	if(NULL == p)
		return;

	name_pipe_destroy(p);
}

/**
 * @brief ��ȡ�ܵ���fd
 */
int MyNamePipeGetFd(HMYNAMEPIPE hnp)
{
	mynamepipe_t * p = (mynamepipe_t *)hnp;
	if(NULL == p)
		return INVALID_FD;
		
	return p->fd;
}
 
/**
 * @brief д
 */
int MyNamePipeWrite(HMYNAMEPIPE hnp, const void * data, size_t data_len)
{
	mynamepipe_t * p = (mynamepipe_t *)hnp;
	if(NULL == p)
		return -1;
		
	return write(p->fd, data, data_len);
}
 
/**
 * @brief ��
 */
int MyNamePipeRead(HMYNAMEPIPE hnp, void * data, size_t data_len)
{
	mynamepipe_t * p = (mynamepipe_t *)hnp;
	if(NULL == p)
		return -1;

	return read(p->fd, data, data_len);
}








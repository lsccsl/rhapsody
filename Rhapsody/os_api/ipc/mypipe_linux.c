/*
*
*mypipe.c ��д�ܵ� lin shao chuan
*
*/

#include "mypipe.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "os_def.h"
#include "mylog.h"


typedef struct __mypipe_t_
{
	int fd[2];
	HMYMEMPOOL hm;
}mypipe_t;


/*
*
*�����ܵ�
*
*/
HMYPIPE MyPipeConstruct(HMYMEMPOOL hm)
{
	int ret = 0;
	mypipe_t * p = (mypipe_t *)MyMemPoolMalloc(hm, sizeof(*p));	
	if(NULL == p)
		return NULL;

	p->hm = hm;
	ret = pipe(p->fd);

	if(0 != ret)
	{
		LOG_WARN(("fail create pipe"));

		MyMemPoolFree(hm, p);
		return NULL;
	}
	
	return (HMYPIPE)p;
}

/*
*
*���ٹܵ�
*
*/
void MyPipeDestruct(HMYPIPE hp)
{
	mypipe_t * p = (mypipe_t *)hp;	
	if(NULL == p)
		return;

	close(p->fd[0]);
	close(p->fd[1]);
	
	MyMemPoolFree(p->hm, p);
}

/*
*
*���ܵ�
*
*/
int MyPipeRead(HMYPIPE hp, void * data, size_t data_len)
{
	mypipe_t * p = (mypipe_t *)hp;	
	if(NULL == p)
		return 0;

	return read(p->fd[0], data, data_len);
}

/*
*
*д�ܵ�
*
*/
int MyPipeWrite(HMYPIPE hp, void * data, size_t data_len)
{
	mypipe_t * p = (mypipe_t *)hp;	
	if(NULL == p)
		return 0;

	return write(p->fd[1], data, data_len);
}

/*
*
*��ȡ��fd
*
*/
int MyPipeGetReadFD(HMYPIPE hp)
{
	mypipe_t * p = (mypipe_t *)hp;	
	if(NULL == p)
		return INVALID_FD;

	return p->fd[0];
}

/*
*
*��ȡдfd
*
*/
int MyPipeGetWriteFD(HMYPIPE hp)
{
	mypipe_t * p = (mypipe_t *)hp;	
	if(NULL == p)
		return INVALID_FD;

	return p->fd[1];
}

/**
 * @brief ��ɷ�����
 */
void MyPipeNoBlock(HMYPIPE hp)
{
	mypipe_t * p = (mypipe_t *)hp;

	LOG_DEBUG(("MyPipeNoBlock"));

	if(NULL == p)
		return;

	{
		int val = fcntl(p->fd[0], F_GETFL);
		if (val < 0)
			return;

		val = val | O_NONBLOCK;
		fcntl(p->fd[0], F_SETFL, val);

		LOG_DEBUG(("MyPipeNoBlock end"));
	}

	{
		int val = fcntl(p->fd[1], F_GETFL);
		if (val < 0)
			return;

		val = val | O_NONBLOCK;
		fcntl(p->fd[1], F_SETFL, val);
	}
}

/**
 * @brief �ܵ���
 */
int MyPipeSelectRead(HMYPIPE hp, void * data, size_t data_len, int time_out)
{
	LOG_DEBUG(("MyPipeSelectRead"));
	return 0;
}

/**
 * @brief �ܵ�д
 */
int MyPipeSelectWrite(HMYPIPE hp, void * data, size_t data_len, int time_out)
{
	LOG_DEBUG(("MyPipeSelectRead"));
	return 0;
}



















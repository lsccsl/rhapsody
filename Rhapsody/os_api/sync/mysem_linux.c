/*
*
*mysem_linux.c 信号量 lin shao chuan
*
*/
#include "mysem.h"

#include <bits/pthreadtypes.h>
#include <semaphore.h>
#include <errno.h>

#include "mylog.h"


typedef struct __mysem_t_
{
	sem_t sem;
	
	HMYMEMPOOL hm;
}mysem_t;


/*
*
*创建信号量
*
*/
HMYSEM MySemRealConstruct(HMYMEMPOOL hm, int value, const char * pcname)
{
	int ret = 0;
	mysem_t * ms = (mysem_t *)MyMemPoolMalloc(hm, sizeof(*ms));
	if(NULL == ms)
		return NULL;

	ret = sem_init(&ms->sem, 0, value);
	ms->hm = hm;

	if(0 != ret)
	{
		LOG_WARN(("fail create semaphore"));

		MyMemPoolFree(hm, ms);
		return NULL;
	}

	return (HMYSEM)ms;
}

/*
*
*销毁信号量
*
*/
void MySemDestruct(HMYSEM hsem)
{
	mysem_t * ms = (mysem_t *)hsem;
	if(NULL == ms)
		return;

	while(EAGAIN == sem_trywait(&ms->sem))
		sem_post(&ms->sem);

	sem_destroy(&ms->sem);
	
	MyMemPoolFree(ms->hm, ms);
}

/*
*
*增加信号量
*
*/
void MySemInCrease(HMYSEM hsem)
{
	mysem_t * ms = (mysem_t *)hsem;
	if(NULL == ms)
		return;

	sem_post(&ms->sem);
}

/*
*
*减少信号量
*
*/
void MySemDeCrease(HMYSEM hsem)
{
	mysem_t * ms = (mysem_t *)hsem;
	if(NULL == ms)
		return;

	sem_wait(&ms->sem);
}















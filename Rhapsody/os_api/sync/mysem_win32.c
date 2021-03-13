/*
*
*mysem_win32.c 信号量 lin shao chuan
*
*/
#include "mysem.h"
#include <assert.h>
#include <windows.h>
#include "mylog.h"


typedef struct __mysem_t_
{
	HANDLE hsem;
	
	HMYMEMPOOL hm;
}mysem_t;


/*
*
*创建信号量
*
*/
HMYSEM MySemRealConstruct(HMYMEMPOOL hm, int value, const char * pcname)
{
	mysem_t * ms = MyMemPoolMalloc(hm, sizeof(*ms));
	if(NULL == ms)
		return NULL;

	if(pcname)
	{
		ms->hsem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, pcname);
		if(NULL == ms->hsem)
			ms->hsem = CreateSemaphore(NULL, value, 0x7fffffff, pcname);
	}
	else
		ms->hsem = CreateSemaphore(NULL, value, 0x7fffffff, NULL);
	ms->hm = hm;

	if(NULL == ms->hsem)
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

	assert(ms->hsem);

	/*while(WAIT_TIMEOUT == WaitForSingleObject(ms->hsem, 0))
		ReleaseSemaphore(ms->hsem, 1, NULL);*/

	CloseHandle(ms->hsem);
	ms->hsem = NULL;

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

	assert(ms->hsem);

	ReleaseSemaphore(ms->hsem, 1, NULL);
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

	assert(ms->hsem);

	WaitForSingleObject(ms->hsem, INFINITE);
}















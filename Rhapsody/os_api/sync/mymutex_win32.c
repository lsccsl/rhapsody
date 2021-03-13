/*
*
*mymutex_win32.c ������ lin shao chuan
*
*/


#include "mymutex.h"
#include <assert.h>
#include <windows.h>
#include "mylog.h"


typedef struct __mymutex_t_
{
	HANDLE hmtx;
	HMYMEMPOOL hm;

	/* �������߳�,�Լ����̶߳��������ü��� */
	volatile DWORD thr_owner;
	volatile int ref_count;
}mymutex_t;


/*
*
*����������
*
*/
HMYMUTEX MyMutexRealConstruct(HMYMEMPOOL hm, const char * pcname)
{
	mymutex_t * mt = MyMemPoolMalloc(hm, sizeof(*mt));
	if(NULL == mt)
		return NULL;

	mt->hm = hm;
	mt->thr_owner = 0;
	mt->ref_count = 0;

	if(pcname)
	{
		mt->hmtx = OpenMutex(MUTEX_ALL_ACCESS, FALSE, pcname);
		if(NULL == mt->hmtx)
			mt->hmtx = CreateMutex(NULL, FALSE, NULL);
	}
	else
		mt->hmtx = CreateMutex(NULL, FALSE, NULL);
	if(NULL == mt->hmtx)
	{
		LOG_WARN(("fail create mutex"));

		MyMemPoolFree(hm, mt);
		return NULL;
	}

	return (HMYMUTEX)mt;
}

/*
*
*���ٻ�����
*
*/
void MyMutexDestruct(HMYMUTEX hmx)
{
	mymutex_t * mt = (mymutex_t *)hmx;
	if(NULL == mt)
		return;

	assert(mt->hmtx);

	if(WAIT_TIMEOUT == WaitForSingleObject(mt->hmtx, 0))
		ReleaseMutex(mt->hmtx);

	CloseHandle(mt->hmtx);
	mt->hmtx = NULL;

	MyMemPoolFree(mt->hm, mt);
}

/*
*
*���� 0:�ɹ�, -1:ʧ��
*
*/
int MyMutexLock(HMYMUTEX hmx)
{
	DWORD ret = 0;
	mymutex_t * mt = (mymutex_t *)hmx;
	if(NULL == mt || NULL == mt->hmtx)
		return -1;

	//LOG_DEBUG(("lock mutex:%x, thread:%d", mt, GetCurrentThreadId()));

	if(0 == mt->thr_owner || GetCurrentThreadId() != mt->thr_owner)
	{
		ret = WaitForSingleObject(mt->hmtx, INFINITE);
		if(WAIT_OBJECT_0 == ret)
		{
			assert(0 == mt->ref_count);
			assert(0 == mt->thr_owner);

			mt->thr_owner = GetCurrentThreadId();
			mt->ref_count ++;

			//LOG_DEBUG(("mutex %x has not been taken, take it thr_owner:%d - %d, ref_count:%d", 
			//	mt, mt->thr_owner, GetCurrentThreadId(), mt->ref_count));

			return 0;
		}
		else if(WAIT_ABANDONED == ret)
		{
			/* ռ�������߳����ͷ���֮ǰ�˳��� */
			LOG_WARN(("unlock mutex got %x %d", mt, mt->thr_owner));

			mt->ref_count = 0;
			mt->thr_owner = 0;;
		}

		return -1;
	}
	else
	{
		assert(0 != mt->ref_count);
		mt->ref_count ++;

		//LOG_DEBUG(("mutex %x has been taken by : %d, current taker : %d, don't take it ref_count:%d", 
		//	mt, mt->thr_owner, GetCurrentThreadId(), mt->ref_count));

		return 0;
	}

	return -1;
}

int MyMutexTryLock(HMYMUTEX hmx)
{
	DWORD ret = 0;
	mymutex_t * mt = (mymutex_t *)hmx;
	if(NULL == mt || NULL == mt->hmtx)
		return -1;

	ret = WaitForSingleObject(mt->hmtx, 0);
	if(WAIT_OBJECT_0 == ret)
		return 0;

	return -1;
}

/*
*
*����
*
*/
int MyMutexUnLock(HMYMUTEX hmx)
{
	mymutex_t * mt = (mymutex_t *)hmx;
	if(NULL == mt || NULL == mt->hmtx)
		return -1;

	//LOG_DEBUG(("unlock mutex:%x, thread:%d", mt, GetCurrentThreadId()));

	if(0 == mt->thr_owner)
	{
		LOG_WARN(("mutex %x has not been taken", mt));
		return 0;
	}
	else if(GetCurrentThreadId() != mt->thr_owner)
	{
		LOG_WARN(("mutex %x thr_owner : %d, current thread : %d, unlocking can't be done", 
			mt,  mt->thr_owner, GetCurrentThreadId()));
		return -1;
	}
	else
	{
		mt->ref_count --;

		//LOG_DEBUG(("mutex %x thr_owner : %d, current thread : %d, unlocking can be done, ref_count:%d", 
		//	mt,  mt->thr_owner, GetCurrentThreadId(), mt->ref_count));

		if(0 == mt->ref_count)
		{
			//LOG_DEBUG(("mutex %x thr_owner : %d, current thread : %d, unlocking can be done, ref_count:%d", 
			//	mt,  mt->thr_owner, GetCurrentThreadId(), mt->ref_count));

			mt->thr_owner = 0;
			return (ReleaseMutex(mt->hmtx)) ? (0): (-1);
		}

		return 0;
	}
}

#ifdef WIN32
/*
*
*��ȡ���
*
*/
HANDLE MyMutexGetHandle(HMYMUTEX hmx)
{
	mymutex_t * mt = (mymutex_t *)hmx;
	if(NULL == mt)
		return NULL;

	return mt->hmtx;
}
#endif



















/*
*
* mymempool.c �ڴ�� 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/
#include "mymempool.h"


#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


/**
 * ���̰߳�ȫ(�ڴ�й©���)
 */
#ifndef WIN32
	#include <pthread.h>
#else
	#include <windows.h>
	#define pthread_mutex_t CRITICAL_SECTION
	#define pthread_mutex_init(cs, x) do{ InitializeCriticalSection(cs); }while(0)
	#define pthread_mutex_lock(cs) do{ EnterCriticalSection(cs); }while(0)
	#define pthread_mutex_unlock(cs) do{ LeaveCriticalSection(cs); }while(0)
#endif

/**
 * ��¼�ڴ���亯��
 */
#ifdef MEM_LEAK_CHECK
	static void add_to_meminfo_list(const char * file, const int line, const void * ptr, size_t size);
	static void del_from_meminfo_list(const void * ptr);
	static int meminfo_report(int, int);
#endif


typedef struct __mymempool_t
{
	mymalloc malloc_fun;
	myfree free_fun;

	mempool_view view_fun;
	
	mempool_destroy destroy_fun;	
	
	/*
	* �û��Զ��������������
	*/
	void * context_data;
}mymempool_t;


/*
*
*�����ڴ��
*
*/
HMYMEMPOOL MyMemPoolConstruct(mymalloc malloc_helper, myfree free_helper, mempool_destroy destroy, mempool_view view, void * context_data)
{
	mymempool_t * mp = NULL;
	assert(malloc_helper && free_helper);

	mp = (mymempool_t *)malloc(sizeof(*mp));
	assert(mp);

	mp->destroy_fun = destroy;
	mp->context_data = context_data;
	mp->malloc_fun = malloc_helper;
	mp->free_fun = free_helper;
	mp->view_fun = view;

	return (HMYMEMPOOL)mp;
}

/*
*
*�����ڴ��
*
*/
void MyMemePoolDestruct(HMYMEMPOOL hm)
{
	mymempool_t * mp = (mymempool_t *)hm;
	assert(mp);

	if(mp->destroy_fun)
		mp->destroy_fun(mp->context_data);

	free(mp);
}

/*
*
*�۲��ڴ��
*
*/
void MyMemPoolView(HMYMEMPOOL hm, void * info, size_t info_size)
{
	mymempool_t * mp = (mymempool_t *)hm;
	assert(mp);
	
	if(mp->view_fun)
		mp->view_fun(mp->context_data, info, info_size);
}

/*
*
*�����ڴ�
*
*/
#ifdef MEM_LEAK_CHECK 
	void * MemPoolMalloc(HMYMEMPOOL hm,size_t size, char * file, int line)
#else
	void * MyMemPoolMalloc(HMYMEMPOOL hm,size_t size)
#endif
{
	mymempool_t * mp = (mymempool_t *)hm;
	void * ptr = NULL;

	if(mp)
		ptr = mp->malloc_fun(size, mp->context_data);
	else
		ptr = malloc(size);

#ifdef MEM_LEAK_CHECK
	//��ӵ��ڴ�����
	add_to_meminfo_list(file, line, ptr, size);
#endif

	return ptr;
}

/*
*
*�ͷ��ڴ�
*
*/
void MyMemPoolFree(HMYMEMPOOL hm, void * ptr)
{
	mymempool_t * mp = (mymempool_t *)hm;

#ifdef MEM_LEAK_CHECK
	//���ڴ�������ɾ��
	del_from_meminfo_list(ptr);
#endif

	if(mp)
		mp->free_fun(ptr, mp->context_data);
	else
		free(ptr);
}


#ifdef MEM_LEAK_CHECK

/*
*
*��ȡ���˼����ڴ�������
*
*/
int MyMemPoolGetBlkCount()
{
	return meminfo_report(0, 0);
}

/*
*
*�ڴ���䱨��
*
*/
void MyMemPoolMemReport(int needassert)
{
	meminfo_report(needassert, 1);
}

/**
 * ����ȫ�ֵ�������ָ��
 */
static void * get_meminfo_lock()
{
	static int b_init = 0;
	static pthread_mutex_t mem_lock;

	if(!b_init)
	{
		pthread_mutex_init(&mem_lock, NULL);
		b_init = 1;
	}

	return &mem_lock;
}

/**
 * �ڴ��¼�����
 */
static void meminfo_lock()
{
	pthread_mutex_lock((pthread_mutex_t *)get_meminfo_lock());
}

/**
 * �ڴ��¼�����
 */
static void meminfo_unlock()
{
	pthread_mutex_unlock((pthread_mutex_t *)get_meminfo_lock());
}

/**
 * �ڴ�����¼����Ŀ��Ϣ
 */
static struct __mymeminfo
{
	char file[64];
	int line;
	size_t size;
	void * ptr;

	struct __mymeminfo * next;
	struct __mymeminfo * prev;
} * head_meminfo_list = NULL;

/**
 * ���ڴ�����¼�������һ����¼
 */
static void add_to_meminfo_list(const char * file, const int line, const void * ptr, size_t size)
{
	struct __mymeminfo * info = (struct __mymeminfo *)malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));
	
	//����
	meminfo_lock();

	strncpy(info->file, file, sizeof(info->file));
	info->line = line;
	info->ptr = (void *)ptr;
	info->size = size;

	info->next = head_meminfo_list;
	if(head_meminfo_list)
		head_meminfo_list->prev = info;

	head_meminfo_list = info;

	//����
	meminfo_unlock();
}

/**
 * ���ڴ�����¼����ɾ��һ����¼
 */
static void del_from_meminfo_list(const void * ptr)
{
	struct __mymeminfo * info = NULL;
	
	//����
	meminfo_lock();

	info = head_meminfo_list;

	while(info)
	{
		if(ptr != info->ptr)
		{
			info = info->next;
			continue;
		}

		if(info->prev)
			info->prev->next = info->next;

		if(info->next)
			info->next->prev = info->prev;

		if(info == head_meminfo_list)
			head_meminfo_list = info->next;

		free(info);

		break;
	}

	//����
	meminfo_unlock();
}

/**
 * �ڴ���䱨��,���ڼ���Ƿ�������ڴ�й©
 */
static int meminfo_report(int needassert, int bprintf)
{
	struct __mymeminfo * info = NULL;
	int leak_count = 0;

	printf("\n======== mem leak report begin ========\n");

	//����
	meminfo_lock();
	
	info = head_meminfo_list;

	while(info)
	{
		if(bprintf)
		{
			#ifdef _MBCSV6
			printf("[%s:%d] %3d - %x \n", info->file, info->line, info->size, info->ptr);
			#else
			#ifdef WIN32
			printf("[%s:%d] %3d - %x \n", info->file, info->line, info->size, (long long)info->ptr);
			#else
			printf("[%s:%d] %3d - %x \n", info->file, info->line, info->size, info->ptr);
			#endif
			#endif
		}

		info = info->next;

		leak_count ++;
	}

	//����
	meminfo_unlock();

	printf("mem leak %d\n", leak_count);

	printf("======== mem leak report end ==========\n\n");

	if(needassert)
		assert(0 == leak_count);

	return leak_count;
}
#else

void MyMemPoolMemReport(int needassert){}
int MyMemPoolGetBlkCount(){return 0;}


#endif














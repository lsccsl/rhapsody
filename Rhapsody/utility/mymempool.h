/*
*
* mymempool.h �ڴ�� 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
* 
*/
#ifndef __MYMEMPOOL_H__
#define __MYMEMPOOL_H__


#include "myconfig.h"
#include <stdlib.h>

/**
 * @brief �����ڴ�Ļص�����
 */
typedef void *(*mymalloc)(size_t size, void * context_data);

/**
 * @brief �ͷ��ڴ�Ļص�����
 */
typedef void(*myfree)(void *, void * context_data);

/**
 * @brief �۲��ڴ�ػص�
 */
typedef void(*mempool_view)(void * context_data, void * info, size_t info_size);

/**
 * @brief �ڴ������ʱ�Ļص�����
 */
typedef void(*mempool_destroy)(void * context_data);

typedef struct __handle_mymempool
{int unused;} * HMYMEMPOOL;


/*
*
*�����ڴ��
*
*/
extern HMYMEMPOOL MyMemPoolConstruct(mymalloc malloc_helper, myfree free_helper, mempool_destroy destroy, mempool_view view, void * context_data);

/*
*
*�����ڴ��
*
*/
extern void MyMemePoolDestruct(HMYMEMPOOL hm);

/*
*
*�۲��ڴ��
* @param info:���ص��ڴ���Ϣ������
* @param info_size:info�Ĵ�С
*
*/
extern void MyMemPoolView(HMYMEMPOOL hm, void * info, size_t info_size);

/*
*
*�����ڴ�
*
*/
#ifdef MEM_LEAK_CHECK
	extern void * MemPoolMalloc(HMYMEMPOOL hm,size_t size, char * file, int line);
	#define MyMemPoolMalloc(h, s) MemPoolMalloc(h, s, __FILE__, __LINE__);
#else
	extern void * MyMemPoolMalloc(HMYMEMPOOL hm,size_t size);
#endif

/*
*
*�ͷ��ڴ�
*
*/
extern void MyMemPoolFree(HMYMEMPOOL hm, void * ptr);

/*
*
*�ڴ��Ƿ�й©����
*
*/
extern void MyMemPoolMemReport(int needassert);

/*
*
*��ȡ�����˶����ڴ�
*
*/
extern int MyMemPoolGetBlkCount();

#endif










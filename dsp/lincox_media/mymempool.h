/*
*
* mymempool.h 内存池 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
* 
*/
#ifndef __MYMEMPOOL_H__
#define __MYMEMPOOL_H__


#include "myconfig.h"
#include <stdlib.h>

/**
 * @brief 分配内存的回调函数
 */
typedef void *(*mymalloc)(size_t size, void * context_data);

/**
 * @brief 释放内存的回调函数
 */
typedef void(*myfree)(void *, void * context_data);

/**
 * @brief 观察内存池回调
 */
typedef void(*mempool_view)(void * context_data, void * info, size_t info_size);

/**
 * @brief 内存池销毁时的回调函数
 */
typedef void(*mempool_destroy)(void * context_data);

typedef struct __handle_mymempool
{int unused;} * HMYMEMPOOL;


/*
*
*构造内存池
*
*/
extern HMYMEMPOOL MyMemPoolConstruct(mymalloc malloc_helper, myfree free_helper, mempool_destroy destroy, mempool_view view, void * context_data);

/*
*
*销毁内存池
*
*/
extern void MyMemePoolDestruct(HMYMEMPOOL hm);

/*
*
*观察内存池
* @param info:返回的内存信息缓冲区
* @param info_size:info的大小
*
*/
extern void MyMemPoolView(HMYMEMPOOL hm, void * info, size_t info_size);

/*
*
*分配内存
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
*释放内存
*
*/
extern void MyMemPoolFree(HMYMEMPOOL hm, void * ptr);

/*
*
*内存是否泄漏报告
*
*/
extern void MyMemPoolMemReport(int needassert);

/*
*
*获取分配了多少内存
*
*/
extern int MyMemPoolGetBlkCount();

#endif










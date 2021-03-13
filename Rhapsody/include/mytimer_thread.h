/*
*
*mytimer_thread.h 定时器线程 lin shao chuan
*
*/
#ifndef __MYTIMER_THREAD_H__
#define __MYTIMER_THREAD_H__

#include "mymempool.h"
#include "mytimerheap.h"


typedef struct __mytimer_thread_handle_
{int unused;}*HMYTIMER_THREAD;


/*
*
*定时器线程构造
*
*/
extern HMYTIMER_THREAD MyTimerThreadConstruct(HMYMEMPOOL hm);

/*
*
*定时器线程析构
*
*/
extern void MyTimerThreadDestruct(HMYTIMER_THREAD htm_thr);

/*
*
*运行定时器线程
*
*/
extern void MyTimerThreadRun(HMYTIMER_THREAD htm_thr);

/*
*
*添加定时器
*
*/
extern HTIMERID MyTimerThreadAddTimer(HMYTIMER_THREAD htm_thr, mytimer_node_t * node);

/*
*
*删除定时器
*
*/
extern int MyTimerThreadDelTimer(HMYTIMER_THREAD htm_thr, HTIMERID timer_id);

/*
*
*重置定时器
*
*/
extern HTIMERID MyTimerThreadResetTimer(HMYTIMER_THREAD htm_thr, HTIMERID timer_id, mytimer_node_t * node);

/*
*
*获取定时器的个数
*
*/
extern int MyTimerThreadGetTimerCount(HMYTIMER_THREAD htm_thr);

/*
*
*获取定时器的个数
*
*/
extern void MyTimerThreadPrint(HMYTIMER_THREAD htm_thr);

#endif




















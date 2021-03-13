/*
*
*mythread.h 封装线程接口 lin shao chuan
*
*/
#ifndef __MYTHREAD_H__
#define __MYTHREAD_H__

#include "mymempool.h"

typedef void *(*MY_THREAD_FUN)(void *);


typedef struct __mythread_handle_
{int unused;}*HMYTHREAD;


/*
*
*创建线程
*
*/
extern HMYTHREAD MyThreadConstruct(MY_THREAD_FUN, void * data, int bsuspend, HMYMEMPOOL hm);

/*
*
*销毁线程
*
*/
extern void MyThreadDestruct(HMYTHREAD ht);

/*
*
*运行线程
*
*/
extern void MyThreadRun(HMYTHREAD ht);

/*
*
*停止线程运行
*
*/
extern void MyThreadSuspend(HMYTHREAD ht);

/*
*
*等待线程退出
*
*/
extern void MyThreadJoin(HMYTHREAD ht);

/*
*
* 判断呼叫此函数的线程是否就是ht本身
* 1:表示是, 0:表示否
*
*/
extern int MyThreadInMyContext(HMYTHREAD ht);


#endif




















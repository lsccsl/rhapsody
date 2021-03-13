/*
*
*mythread.h ��װ�߳̽ӿ� lin shao chuan
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
*�����߳�
*
*/
extern HMYTHREAD MyThreadConstruct(MY_THREAD_FUN, void * data, int bsuspend, HMYMEMPOOL hm);

/*
*
*�����߳�
*
*/
extern void MyThreadDestruct(HMYTHREAD ht);

/*
*
*�����߳�
*
*/
extern void MyThreadRun(HMYTHREAD ht);

/*
*
*ֹͣ�߳�����
*
*/
extern void MyThreadSuspend(HMYTHREAD ht);

/*
*
*�ȴ��߳��˳�
*
*/
extern void MyThreadJoin(HMYTHREAD ht);

/*
*
* �жϺ��д˺������߳��Ƿ����ht����
* 1:��ʾ��, 0:��ʾ��
*
*/
extern int MyThreadInMyContext(HMYTHREAD ht);


#endif




















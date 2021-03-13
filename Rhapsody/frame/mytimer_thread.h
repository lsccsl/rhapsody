/*
*
*mytimer_thread.h ��ʱ���߳� lin shao chuan
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
*��ʱ���̹߳���
*
*/
extern HMYTIMER_THREAD MyTimerThreadConstruct(HMYMEMPOOL hm);

/*
*
*��ʱ���߳�����
*
*/
extern void MyTimerThreadDestruct(HMYTIMER_THREAD htm_thr);

/*
*
*���ж�ʱ���߳�
*
*/
extern void MyTimerThreadRun(HMYTIMER_THREAD htm_thr);

/*
*
*��Ӷ�ʱ��
*
*/
extern HTIMERID MyTimerThreadAddTimer(HMYTIMER_THREAD htm_thr, mytimer_node_t * node);

/*
*
*ɾ����ʱ��
*
*/
extern int MyTimerThreadDelTimer(HMYTIMER_THREAD htm_thr, HTIMERID timer_id);

/*
*
*���ö�ʱ��
*
*/
extern HTIMERID MyTimerThreadResetTimer(HMYTIMER_THREAD htm_thr, HTIMERID timer_id, mytimer_node_t * node);

/*
*
*��ȡ��ʱ���ĸ���
*
*/
extern int MyTimerThreadGetTimerCount(HMYTIMER_THREAD htm_thr);

/*
*
*��ȡ��ʱ���ĸ���
*
*/
extern void MyTimerThreadPrint(HMYTIMER_THREAD htm_thr);

#endif




















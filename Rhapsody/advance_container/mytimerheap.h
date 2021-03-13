/*
* mytimerheap.h ��ʱ�� lin shao chuan
*
* changelog:
* <lsccsl@tom.com 2008 09:17>
*			ȥ����timeval����ṹ�������,��ֹ����<winsock2.h>֮���ͷ�ļ�.
*			�ڳ�ʱ�ص�������,���䶨ʱ��id����
*/

#ifndef __MYTIMERHEAP_H__
#define __MYTIMERHEAP_H__


#include "mymempool.h"
#include "__tv_def.h"

typedef struct __mytimerheap_handle_
{int unused;}*HMYTIMERHEAP;

typedef void * HTIMERID;

/*
*��ʱ�����Ļص�����
*@param 
	context_data:�����ص�ʱ���û�������
	timer_user_data:��ʱ���ڵ���ص��û�����
	timerid:��ʱ����id
*/
typedef int (*MY_TIMER_HEAP_TIMEOUT_CB)(unsigned long context_data, 
										unsigned long timer_user_data,
										HTIMERID timerid);

typedef struct __mytimer_node_t_
{
	unsigned long context_data;				/*�����ص�ʱ���û�������*/
	unsigned long timer_user_data;			/*��ʱ���ڵ���ص��û�����*/
	MY_TIMER_HEAP_TIMEOUT_CB timeout_cb;	/*��ʱ���ص�����*/

	mytv_t period;							/* ��ʱ��ʱ���*/
	mytv_t first_expire;					/* <��һ��> ��Գ�ʱʱ��*/

	mytv_t abs_expire;						/* <��һ��> ���Գ�ʱʱ�� ������û����� */
}mytimer_node_t;


/*
*
* ���춨ʱ��
*
*/
extern HMYTIMERHEAP MyTimerHeapConstruct(HMYMEMPOOL hm);

/*
*
* ������ʱ��
*
*/
extern void MyTimerHeapDestruct(HMYTIMERHEAP hth);

/*
*
* ��Ӷ�ʱ��
*
*@param 
	mytimer_node_t * node:��ʱ���ڵ�
*@retval ��ʱ����id NULL:ʧ��
*
*/
extern HTIMERID MyTimerHeapAdd(HMYTIMERHEAP hth, mytimer_node_t * node);

/*
*
* ɾ����ʱ��
*
*/
extern int MyTimerHeapDel(HMYTIMERHEAP hth, HTIMERID timerid);

/*
*
* ���ö�ʱ��
*
*@param 
	mytimer_node_t * node:��ʱ���ڵ�
*@retval ��ʱ����id NULL:ʧ��
*
*/
extern HTIMERID MyTimerHeapReset(HMYTIMERHEAP hth, HTIMERID timerid, mytimer_node_t * node);

/*
*
* ��ȡͷ�ڵ�,�����Գ�ʱ��С(���糬ʱ)�ļ�ֵ
*
*/
extern HTIMERID MyTimeHeapGetEarliestKey(HMYTIMERHEAP hth);

/*
*
* ��ȡ��С��ʱ(����ʱ��)
*
*@retval 0:�ɹ� -1:ʧ��
*
*/
extern int MyTimerHeapGetEarliestExpire(HMYTIMERHEAP hth, mytv_t * expire);

/*
*
* �������г�ʱ�¼�
*
*@retval ��ʱ����
*
*/
extern unsigned MyTimerHeapRunExpire(HMYTIMERHEAP hth, mytv_t * tv_now);

/*
*
* ��ȡ��ʱ���ĸ���
*
*/
extern size_t MyTimerHeapGetCount(HMYTIMERHEAP hth);

/*
*
* ���timer heap�����Ϣ
*
*/
extern void MyTimerHeapPrint(HMYTIMERHEAP hth);


#endif




























/*
*
*myevent.h ����(cond)/�¼�(event) lin shao chuan
*
*/
#ifndef __MY_EVENT_H__
#define __MY_EVENT_H__

#include "mymempool.h"
#include "__tv_def.h"


typedef struct __myevent_handle_
{int unused;}*HMYEVENT;


/*
*
*�����¼�/����
*
*/
extern HMYEVENT MyEventRealConstruct(HMYMEMPOOL hm, int bNotAutoReset, const char * pcname);
#define MyEventConstruct(hm) MyEventRealConstruct((hm), 0, NULL)

/*
*
*�����¼�/����
*
*/
extern void MyEventDestruct(HMYEVENT he);

/*
*
*���¼����ó�signaled״̬
*
*/
extern void MyEventSetSignaled(HMYEVENT he);

/*
*
*�㲥�¼�����
*
*/
extern void MyEventBroadCast(HMYEVENT he);

/*
*
*���¼����óɷ�signaled״̬
*
*/
extern void MyEventSetNoSignaled(HMYEVENT he);

/*
*
* �ȴ��¼�����, 
*
*@param millsecond:��λ΢��, -1��ʾ���޵ȴ�,ֱ���¼�����
*@return 0:�¼����� -1:��ʱ
*
*/
extern int MyEventWait(HMYEVENT he, mytv_t * tv);


#endif













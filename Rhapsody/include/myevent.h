/*
*
*myevent.h 条件(cond)/事件(event) lin shao chuan
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
*创建事件/条件
*
*/
extern HMYEVENT MyEventRealConstruct(HMYMEMPOOL hm, int bNotAutoReset, const char * pcname);
#define MyEventConstruct(hm) MyEventRealConstruct((hm), 0, NULL)

/*
*
*销毁事件/条件
*
*/
extern void MyEventDestruct(HMYEVENT he);

/*
*
*把事件设置成signaled状态
*
*/
extern void MyEventSetSignaled(HMYEVENT he);

/*
*
*广播事件发生
*
*/
extern void MyEventBroadCast(HMYEVENT he);

/*
*
*把事件设置成非signaled状态
*
*/
extern void MyEventSetNoSignaled(HMYEVENT he);

/*
*
* 等待事件发生, 
*
*@param millsecond:单位微秒, -1表示无限等待,直至事件发生
*@return 0:事件发生 -1:超时
*
*/
extern int MyEventWait(HMYEVENT he, mytv_t * tv);


#endif













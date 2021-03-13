/*
*
*myproritymsgque.h 优先级队列 lin shao chuan
*
*/
#ifndef __MYPRORITYMSGQUE_H__
#define __MYPRORITYMSGQUE_H__


#include "mymempool.h"


typedef struct __myproritymsgque_handle_
{int unused;}*HMY_PRO_MQ;


/*
*
*创建优先级队列
*
*/
extern HMY_PRO_MQ MyProrityMsgQueConstruct(HMYMEMPOOL hm, size_t max_msg_count);

/*
*
*销毁优先级队列
*
*/
extern void MyProrityMsgQueDestruct(HMY_PRO_MQ hpmq);

/*
*
*添加一条消息,如果队列满,则会阻塞,产生读通知
*
*/
extern int MyProrityMsgQuePush_block(HMY_PRO_MQ hpmq, int prority, const void * data);

/*
*
*添加一条消息,如果队列满,函数返回,不产生读通知
*
*/
extern int MyProrityMsgQuePush(HMY_PRO_MQ hpmq, int prority, const void * data);

/*
*
*取出一条优先级最高的消息,如果队列为空,则会阻塞,产生写通知
*
*/
extern void * MyProrityMsgQuePop_block(HMY_PRO_MQ hpmq);

/*
*
*取出一条优先级最高的消息,如果队列为空,返回null,不产生写通知
*
*/
extern void * MyProrityMsgQuePop(HMY_PRO_MQ hpmq);

/*
*
*取出一条消息
*
*/
extern size_t MyProrityMsgQueGetCount(HMY_PRO_MQ hpmq);


#endif



















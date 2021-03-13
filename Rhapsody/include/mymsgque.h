/*
*
*mymsgque.h 队列 lin shao chuan
*
*/
#ifndef __MYMSGQUE_H__
#define __MYMSGQUE_H__


#include "mymempool.h"


typedef struct __mymsgque_handle_
{int unused;}*HMYMSGQUE;


/*
*
*创建消息队列
*
*/
extern HMYMSGQUE MyMsgQueConstruct(HMYMEMPOOL hm, size_t max_msg_count);

/*
*
*销毁消息队列
*
*/
extern void MyMsgQueDestruct(HMYMSGQUE hmq);

/*
*
*添加消息至队尾,如果队列满了,则会阻塞,产生读通知
*
*/
extern void MyMsgQuePush_block(HMYMSGQUE hmq, const void * data);

/*
*
*添加消息至队尾,不产生读通知
*
*/
extern void MyMsgQuePush(HMYMSGQUE hmq, const void * data);

/*
*
*取队头消息,如果队列为空,则会阻塞,产生写通知
*
*/
extern void * MyMsgQuePop_block(HMYMSGQUE hmq);

/*
*
*取队头消息,不产生写通知
*
*/
extern void * MyMsgQuePop(HMYMSGQUE hmq);

/*
*
*取得消息的数量
*
*/
extern size_t MyMsgQueGetCount(HMYMSGQUE hmq);


#endif






















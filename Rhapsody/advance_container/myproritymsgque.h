/*
*
*myproritymsgque.h ���ȼ����� lin shao chuan
*
*/
#ifndef __MYPRORITYMSGQUE_H__
#define __MYPRORITYMSGQUE_H__


#include "mymempool.h"


typedef struct __myproritymsgque_handle_
{int unused;}*HMY_PRO_MQ;


/*
*
*�������ȼ�����
*
*/
extern HMY_PRO_MQ MyProrityMsgQueConstruct(HMYMEMPOOL hm, size_t max_msg_count);

/*
*
*�������ȼ�����
*
*/
extern void MyProrityMsgQueDestruct(HMY_PRO_MQ hpmq);

/*
*
*���һ����Ϣ,���������,�������,������֪ͨ
*
*/
extern int MyProrityMsgQuePush_block(HMY_PRO_MQ hpmq, int prority, const void * data);

/*
*
*���һ����Ϣ,���������,��������,��������֪ͨ
*
*/
extern int MyProrityMsgQuePush(HMY_PRO_MQ hpmq, int prority, const void * data);

/*
*
*ȡ��һ�����ȼ���ߵ���Ϣ,�������Ϊ��,�������,����д֪ͨ
*
*/
extern void * MyProrityMsgQuePop_block(HMY_PRO_MQ hpmq);

/*
*
*ȡ��һ�����ȼ���ߵ���Ϣ,�������Ϊ��,����null,������д֪ͨ
*
*/
extern void * MyProrityMsgQuePop(HMY_PRO_MQ hpmq);

/*
*
*ȡ��һ����Ϣ
*
*/
extern size_t MyProrityMsgQueGetCount(HMY_PRO_MQ hpmq);


#endif



















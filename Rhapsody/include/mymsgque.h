/*
*
*mymsgque.h ���� lin shao chuan
*
*/
#ifndef __MYMSGQUE_H__
#define __MYMSGQUE_H__


#include "mymempool.h"


typedef struct __mymsgque_handle_
{int unused;}*HMYMSGQUE;


/*
*
*������Ϣ����
*
*/
extern HMYMSGQUE MyMsgQueConstruct(HMYMEMPOOL hm, size_t max_msg_count);

/*
*
*������Ϣ����
*
*/
extern void MyMsgQueDestruct(HMYMSGQUE hmq);

/*
*
*�����Ϣ����β,�����������,�������,������֪ͨ
*
*/
extern void MyMsgQuePush_block(HMYMSGQUE hmq, const void * data);

/*
*
*�����Ϣ����β,��������֪ͨ
*
*/
extern void MyMsgQuePush(HMYMSGQUE hmq, const void * data);

/*
*
*ȡ��ͷ��Ϣ,�������Ϊ��,�������,����д֪ͨ
*
*/
extern void * MyMsgQuePop_block(HMYMSGQUE hmq);

/*
*
*ȡ��ͷ��Ϣ,������д֪ͨ
*
*/
extern void * MyMsgQuePop(HMYMSGQUE hmq);

/*
*
*ȡ����Ϣ������
*
*/
extern size_t MyMsgQueGetCount(HMYMSGQUE hmq);


#endif






















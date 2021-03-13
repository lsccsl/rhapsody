/*
*
* mylist.h ���� 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/

#ifndef __MYLIST_H__
#define __MYLIST_H__


#include "mymempool.h"


typedef struct __handle_mylist
{
	int unused;
}* HMYLIST;

typedef struct __handle_mylist_iter
{
	int unused;
}* HMYLIST_ITER;


/*
*
*��������
*
*/
extern HMYLIST MyListConstruct(HMYMEMPOOL hm);

/*
*
*��������
*
*/
extern int MyListDestruct(HMYLIST hlist);

/*
*
*���һ�ڵ㵽������β
*
*/
extern HMYLIST_ITER MyListAddTail(HMYLIST hlist, const void * userdata);

/*
*
*���һ�ڵ�������ͷ
*
*/
extern HMYLIST_ITER MyListAddHead(HMYLIST hlist, const void * userdata); 

/*
*
*ɾ��һ�ڵ㣬�����û�����
*
*/
extern HMYLIST_ITER MyListErase(HMYLIST hlist, HMYLIST_ITER iter);

/*
*
*ɾ�����нڵ�
*
*/
extern void MyListEraseAll(HMYLIST hlist);

/*
*
*��ȡͷ���
*
*/
extern HMYLIST_ITER MyListGetHead(HMYLIST hlist);

/*
*
*��ȡβ���
*
*/
extern HMYLIST_ITER MyListGetTail(HMYLIST hlist);

/*
*
*��ȡ��һ�ڵ�
*
*/
extern HMYLIST_ITER MyListGetNext(HMYLIST hlist, HMYLIST_ITER iter);

/*
*
*��ȡ��һ�ڵ�
*
*/
extern HMYLIST_ITER MyListGetPrev(HMYLIST hlist, HMYLIST_ITER iter);

/*
*
*��ȡ�ڵ���û�����
*
*/
extern void * MyListGetIterData(HMYLIST_ITER iter);

/*
*
*�����Ƿ�Ϊ��
*
*/
extern int MyListIsEmpty(HMYLIST hlist);

/*
*
*����ͷ�ڵ�
*
*/
extern void * MyListPopHead(HMYLIST hlist);

/*
*
*����β�ڵ�
*
*/
extern void * MyListPopTail(HMYLIST hlist);

/*
*
*ȡ��Ԫ�ظ���
*
*/
extern int MyListGetCount(HMYLIST hlist);

#endif























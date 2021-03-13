/*
*
* mylistex.h ���� 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/
#ifndef __MYLISTEX_H__
#define __MYLISTEX_H__


#include "myobj.h"
#include "mylist.h"


typedef struct __mylistex_handle_
{int unused;}*HMYLIST_EX;


typedef struct __mylistex_iter_
{int unused;}*HMYLIST_EX_ITER;


/*
*
*mylistex�Ĺ���
*
*/
extern HMYLIST_EX MyListExConstruct(HMYMEMPOOL hm, myobj_ops * data_ops);

/*
*
*mylistex�Ĺ���
*
*/
extern void MyListExDestruct(HMYLIST_EX hlist_ex);

/*
*
*���һ�ڵ㵽������β
*
*/
extern HMYLIST_EX_ITER MyListExAddTail(HMYLIST_EX hlist, const void * userdata, size_t data_size);

/*
*
*���һ�ڵ�������ͷ
*
*/
extern HMYLIST_EX_ITER MyListExAddHead(HMYLIST_EX hlist, const void * userdata, size_t data_size); 

/*
*
*ɾ��һ�ڵ㣬�����û�����
*
*/
extern HMYLIST_EX_ITER MyListExErase(HMYLIST_EX hlist, HMYLIST_ITER iter);

/*
*
*ɾ�����нڵ�
*
*/
extern void MyListExEraseAll(HMYLIST_EX hlist);

/*
*
*����ͷ�ڵ� 
*
*/
extern int MyListExPopHead(HMYLIST_EX hlist, void * data, size_t data_size);

/*
*
*����β�ڵ� 
*
*/
extern int MyListExPopTail(HMYLIST_EX hlist, void * data, size_t data_size);


#endif






















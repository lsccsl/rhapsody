/*
*
* mylist.h 链表 
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
*构造链表
*
*/
extern HMYLIST MyListConstruct(HMYMEMPOOL hm);

/*
*
*销毁链表
*
*/
extern int MyListDestruct(HMYLIST hlist);

/*
*
*添加一节点到至链表尾
*
*/
extern HMYLIST_ITER MyListAddTail(HMYLIST hlist, const void * userdata);

/*
*
*添加一节点至链表头
*
*/
extern HMYLIST_ITER MyListAddHead(HMYLIST hlist, const void * userdata); 

/*
*
*删除一节点，返回用户数据
*
*/
extern HMYLIST_ITER MyListErase(HMYLIST hlist, HMYLIST_ITER iter);

/*
*
*删除所有节点
*
*/
extern void MyListEraseAll(HMYLIST hlist);

/*
*
*获取头结点
*
*/
extern HMYLIST_ITER MyListGetHead(HMYLIST hlist);

/*
*
*获取尾结点
*
*/
extern HMYLIST_ITER MyListGetTail(HMYLIST hlist);

/*
*
*获取下一节点
*
*/
extern HMYLIST_ITER MyListGetNext(HMYLIST hlist, HMYLIST_ITER iter);

/*
*
*获取上一节点
*
*/
extern HMYLIST_ITER MyListGetPrev(HMYLIST hlist, HMYLIST_ITER iter);

/*
*
*获取节点的用户数据
*
*/
extern void * MyListGetIterData(HMYLIST_ITER iter);

/*
*
*链表是否为空
*
*/
extern int MyListIsEmpty(HMYLIST hlist);

/*
*
*弹出头节点
*
*/
extern void * MyListPopHead(HMYLIST hlist);

/*
*
*弹出尾节点
*
*/
extern void * MyListPopTail(HMYLIST hlist);

/*
*
*取出元素个数
*
*/
extern int MyListGetCount(HMYLIST hlist);

#endif























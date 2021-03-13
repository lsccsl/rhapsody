/*
*
* mylistex.h 链表 
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
*mylistex的构造
*
*/
extern HMYLIST_EX MyListExConstruct(HMYMEMPOOL hm, myobj_ops * data_ops);

/*
*
*mylistex的构造
*
*/
extern void MyListExDestruct(HMYLIST_EX hlist_ex);

/*
*
*添加一节点到至链表尾
*
*/
extern HMYLIST_EX_ITER MyListExAddTail(HMYLIST_EX hlist, const void * userdata, size_t data_size);

/*
*
*添加一节点至链表头
*
*/
extern HMYLIST_EX_ITER MyListExAddHead(HMYLIST_EX hlist, const void * userdata, size_t data_size); 

/*
*
*删除一节点，返回用户数据
*
*/
extern HMYLIST_EX_ITER MyListExErase(HMYLIST_EX hlist, HMYLIST_ITER iter);

/*
*
*删除所有节点
*
*/
extern void MyListExEraseAll(HMYLIST_EX hlist);

/*
*
*弹出头节点 
*
*/
extern int MyListExPopHead(HMYLIST_EX hlist, void * data, size_t data_size);

/*
*
*弹出尾节点 
*
*/
extern int MyListExPopTail(HMYLIST_EX hlist, void * data, size_t data_size);


#endif






















/*
*
* mydeque.h 双向增长队列 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/
#ifndef __MYDEQUE_H__
#define __MYDEQUE_H__


#include "myobj.h"
#include "mymempool.h"


typedef struct __mydeque_handle_
{int unused;}*HMYDEQUE;

typedef struct __mydeque_iter_
{int unused;}*HMYDEQUE_ITER;


/*
*
*构造
*
*/
extern HMYDEQUE MyDequeConstruct(HMYMEMPOOL hm, const myobj_ops * data_ops, const size_t buffer_size, const size_t map_size);

/*
*
*析构
*
*/
extern void MyDequeDestruct(HMYDEQUE hdq);

/*
*
*从头部添加
*
*/
extern int MyDequeAddHead(HMYDEQUE hdq, const void * data, const size_t data_size);

/*
*
*从尾部添加
*
*/
extern int MyDequeAddTail(HMYDEQUE hdq, const void * data, const size_t data_size);

/*
*
*从头部删除
*
*/
extern int MyDequeDelHead(HMYDEQUE hdq);

/*
*
*从尾部删除
*
*/
extern int MyDequeDelTail(HMYDEQUE hdq);

/*
*
*获取头部
*
*/
extern int MyDequeGetHead(HMYDEQUE hdq, void ** data, size_t * data_size);

/*
*
*获取尾部
*
*/
extern int MyDequeGetTail(HMYDEQUE hdq, void ** data, size_t * data_size);

/*
*
*获取尾部
*
*/
extern size_t MyDequeGetCount(HMYDEQUE hdq);

/*
*
*获取尾部
*
*/
extern void MyDequePrint(HMYDEQUE hdq);


#endif

















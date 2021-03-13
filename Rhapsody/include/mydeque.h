/*
*
* mydeque.h ˫���������� 
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
*����
*
*/
extern HMYDEQUE MyDequeConstruct(HMYMEMPOOL hm, const myobj_ops * data_ops, const size_t buffer_size, const size_t map_size);

/*
*
*����
*
*/
extern void MyDequeDestruct(HMYDEQUE hdq);

/*
*
*��ͷ�����
*
*/
extern int MyDequeAddHead(HMYDEQUE hdq, const void * data, const size_t data_size);

/*
*
*��β�����
*
*/
extern int MyDequeAddTail(HMYDEQUE hdq, const void * data, const size_t data_size);

/*
*
*��ͷ��ɾ��
*
*/
extern int MyDequeDelHead(HMYDEQUE hdq);

/*
*
*��β��ɾ��
*
*/
extern int MyDequeDelTail(HMYDEQUE hdq);

/*
*
*��ȡͷ��
*
*/
extern int MyDequeGetHead(HMYDEQUE hdq, void ** data, size_t * data_size);

/*
*
*��ȡβ��
*
*/
extern int MyDequeGetTail(HMYDEQUE hdq, void ** data, size_t * data_size);

/*
*
*��ȡβ��
*
*/
extern size_t MyDequeGetCount(HMYDEQUE hdq);

/*
*
*��ȡβ��
*
*/
extern void MyDequePrint(HMYDEQUE hdq);


#endif

















/*
*
* myheap.h �� 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/
#ifndef __MYHEAP_H__
#define __MYHEAP_H__


#include "myvector.h"


typedef struct __myheap_t_ * HMYHEAP;

typedef struct __myheap_key_handle_
{int unused;}* HMYHEAP_KEY;


/*
*
* ����heap
*
*/
extern HMYHEAP MyHeapConstruct(HMYMEMPOOL hm, int size, myobj_ops * data_ops, ALG_COMPARE compare);

/*
*
* ����heap
*
*/
extern void MyHeapDestruct(HMYHEAP hp);

/*
*
* ���һ���ڵ�
*
*/
extern HMYHEAP_KEY MyHeapPush(HMYHEAP hp, const void * data, const size_t data_size);

/*
*
* �Ӷ��е���һ��Ԫ��
*
*/
extern int MyHeapPop(HMYHEAP hp);

/*
*
* ȡ����Ԫ��
*
*/
extern void * MyHeapFront(HMYHEAP hp, size_t * data_size);

/*
*
* ȡ����Ԫ�ص�key
*
*/
extern HMYHEAP_KEY MyHeapFrontKey(HMYHEAP hp);

/*
*
* ɾ��һ��Ԫ��
*
*/
extern int MyHeapDel(HMYHEAP hp, HMYHEAP_KEY key);

/*
*
* ����һ��Ԫ��
*
*/
extern HMYHEAP_KEY MyHeapUpdate(HMYHEAP hp, HMYHEAP_KEY key, const void * data, const size_t data_size);

/*
*
* ����һ��Ԫ��
*
*/
extern void * MyHeapSearch(HMYHEAP hp, HMYHEAP_KEY key, size_t * data_size);

/*
*
* ɾ�����е�Ԫ��
*
*/
extern void MyHeapClear(HMYHEAP hp);


#define MyHeapExamin(hp) MyVectorHeapExamin((HMYVECTOR)(hp))

#define MyHeapExaminSortOK(hp) MyVectorHeapExaminSortOK((HMYVECTOR)(hp))

#define MyHeapPrint(hp) MyVectorPrint((HMYVECTOR)(hp))

#define MyHeapGetCount(hp) MyVectorGetCount((HMYVECTOR)(hp))

#define MyHeapSort(hp) MyVectorHeapSort((HMYVECTOR)(hp))


#endif
























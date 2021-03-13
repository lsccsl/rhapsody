/*
*
* myheap.h 堆 
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
* 构造heap
*
*/
extern HMYHEAP MyHeapConstruct(HMYMEMPOOL hm, int size, myobj_ops * data_ops, ALG_COMPARE compare);

/*
*
* 析构heap
*
*/
extern void MyHeapDestruct(HMYHEAP hp);

/*
*
* 添加一个节点
*
*/
extern HMYHEAP_KEY MyHeapPush(HMYHEAP hp, const void * data, const size_t data_size);

/*
*
* 从堆中弹出一个元素
*
*/
extern int MyHeapPop(HMYHEAP hp);

/*
*
* 取队首元素
*
*/
extern void * MyHeapFront(HMYHEAP hp, size_t * data_size);

/*
*
* 取队首元素的key
*
*/
extern HMYHEAP_KEY MyHeapFrontKey(HMYHEAP hp);

/*
*
* 删除一个元素
*
*/
extern int MyHeapDel(HMYHEAP hp, HMYHEAP_KEY key);

/*
*
* 更新一个元素
*
*/
extern HMYHEAP_KEY MyHeapUpdate(HMYHEAP hp, HMYHEAP_KEY key, const void * data, const size_t data_size);

/*
*
* 查找一个元素
*
*/
extern void * MyHeapSearch(HMYHEAP hp, HMYHEAP_KEY key, size_t * data_size);

/*
*
* 删除所有的元素
*
*/
extern void MyHeapClear(HMYHEAP hp);


#define MyHeapExamin(hp) MyVectorHeapExamin((HMYVECTOR)(hp))

#define MyHeapExaminSortOK(hp) MyVectorHeapExaminSortOK((HMYVECTOR)(hp))

#define MyHeapPrint(hp) MyVectorPrint((HMYVECTOR)(hp))

#define MyHeapGetCount(hp) MyVectorGetCount((HMYVECTOR)(hp))

#define MyHeapSort(hp) MyVectorHeapSort((HMYVECTOR)(hp))


#endif
























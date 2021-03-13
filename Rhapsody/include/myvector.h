/*
*
* myvector.h vector 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/

#ifndef __MYVECTOR_H__
#define __MYVECTOR_H__


#include "mymempool.h"
#include "myobj.h"
#include "MyfunctionDef.h"


typedef struct __myvector_element_ * HMYVECTOR_ITER;

typedef struct __myvector_t_ * HMYVECTOR;


/*
*
*构造vector
*
*/
extern HMYVECTOR MyVectorConstruct(HMYMEMPOOL hm, size_t size, myobj_ops * data_ops, ALG_COMPARE compare);

/*
*
*析构vector
*
*/
extern void MyVectorDestruct(HMYVECTOR hv);

/*
*
*添加节点(在未尾添加)
*
*/
extern HMYVECTOR_ITER MyVectorAdd(HMYVECTOR hv, const void * data, const size_t data_size);

/*
*
*删除指定的节点
*
*/
extern int MyVectorDel(HMYVECTOR hv, size_t index);

/*
*
*删除指定的节点
*
*/
extern void MyVectorClear(HMYVECTOR hv);

/*
*
*获取指定的节点
*
*/
extern HMYVECTOR_ITER MyVectorGetIndex(HMYVECTOR hv, size_t index);

/*
*
*获取指定的节点数据
*
*/
extern void * MyVectorGetIndexData(HMYVECTOR hv, size_t index, size_t * data_size);

/*
*
*获取迭代器的数据
*
*/
extern void * MyVectorGetIterData(HMYVECTOR_ITER it);

/*
*
*获取迭代器的数据
*
*/
extern size_t MyVectorGetIterDataSize(HMYVECTOR_ITER it);

/*
*
*获取头结点
*
*/
extern HMYVECTOR_ITER MyVectorGetHead(HMYVECTOR hv);

/*
*
*获取尾结点
*
*/
extern HMYVECTOR_ITER MyVectorGetTail(HMYVECTOR hv);

/*
*
*获取尾结点
*
*/
extern int MyVectorGetTailData(HMYVECTOR hv, void ** data, size_t * data_size);

/*
*
*获取节点个数
*
*/
extern size_t MyVectorGetCount(HMYVECTOR hv);

/*
*
*获取vector的容量
*
*/
extern size_t MyVectorGetSize(HMYVECTOR hv);

/*
*
*重新设置vector的大小
*
*/
extern int MyVectorResize(HMYVECTOR hv, int vector_size);

/*
*
*交换两个vector元素,下标需在数组范围内
*
*/
extern int MyVectorSwitch(HMYVECTOR hv, const size_t index1, const size_t index2);

/*
*
*利用vector生成一个堆
*
*/
extern int MyVectorHeapMake(HMYVECTOR hv);

/*
*
*堆排序
*
*/
extern int MyVectorHeapSort(HMYVECTOR hv);

/*
*
*往堆中压入一个元素
*
*/
extern int MyVectorHeapPush(HMYVECTOR hv, const void * data, const size_t data_size);

/*
*
*从堆中弹出一个元素
*
*/
extern int MyVectorHeapPop(HMYVECTOR hv);


/*
*
*检查一个堆是否合法 1:表示合法
*
*/
extern void MyVectorHeapExamin(HMYVECTOR hv);

/*
*
*检查一个数组是否正确地被排序
*
*/
extern void MyVectorHeapExaminSortOK(HMYVECTOR hv);

/*
*
*打印出vector
*
*/
extern void MyVectorPrint(HMYVECTOR hv);


#endif
















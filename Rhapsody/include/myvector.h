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
*����vector
*
*/
extern HMYVECTOR MyVectorConstruct(HMYMEMPOOL hm, size_t size, myobj_ops * data_ops, ALG_COMPARE compare);

/*
*
*����vector
*
*/
extern void MyVectorDestruct(HMYVECTOR hv);

/*
*
*��ӽڵ�(��δβ���)
*
*/
extern HMYVECTOR_ITER MyVectorAdd(HMYVECTOR hv, const void * data, const size_t data_size);

/*
*
*ɾ��ָ���Ľڵ�
*
*/
extern int MyVectorDel(HMYVECTOR hv, size_t index);

/*
*
*ɾ��ָ���Ľڵ�
*
*/
extern void MyVectorClear(HMYVECTOR hv);

/*
*
*��ȡָ���Ľڵ�
*
*/
extern HMYVECTOR_ITER MyVectorGetIndex(HMYVECTOR hv, size_t index);

/*
*
*��ȡָ���Ľڵ�����
*
*/
extern void * MyVectorGetIndexData(HMYVECTOR hv, size_t index, size_t * data_size);

/*
*
*��ȡ������������
*
*/
extern void * MyVectorGetIterData(HMYVECTOR_ITER it);

/*
*
*��ȡ������������
*
*/
extern size_t MyVectorGetIterDataSize(HMYVECTOR_ITER it);

/*
*
*��ȡͷ���
*
*/
extern HMYVECTOR_ITER MyVectorGetHead(HMYVECTOR hv);

/*
*
*��ȡβ���
*
*/
extern HMYVECTOR_ITER MyVectorGetTail(HMYVECTOR hv);

/*
*
*��ȡβ���
*
*/
extern int MyVectorGetTailData(HMYVECTOR hv, void ** data, size_t * data_size);

/*
*
*��ȡ�ڵ����
*
*/
extern size_t MyVectorGetCount(HMYVECTOR hv);

/*
*
*��ȡvector������
*
*/
extern size_t MyVectorGetSize(HMYVECTOR hv);

/*
*
*��������vector�Ĵ�С
*
*/
extern int MyVectorResize(HMYVECTOR hv, int vector_size);

/*
*
*��������vectorԪ��,�±��������鷶Χ��
*
*/
extern int MyVectorSwitch(HMYVECTOR hv, const size_t index1, const size_t index2);

/*
*
*����vector����һ����
*
*/
extern int MyVectorHeapMake(HMYVECTOR hv);

/*
*
*������
*
*/
extern int MyVectorHeapSort(HMYVECTOR hv);

/*
*
*������ѹ��һ��Ԫ��
*
*/
extern int MyVectorHeapPush(HMYVECTOR hv, const void * data, const size_t data_size);

/*
*
*�Ӷ��е���һ��Ԫ��
*
*/
extern int MyVectorHeapPop(HMYVECTOR hv);


/*
*
*���һ�����Ƿ�Ϸ� 1:��ʾ�Ϸ�
*
*/
extern void MyVectorHeapExamin(HMYVECTOR hv);

/*
*
*���һ�������Ƿ���ȷ�ر�����
*
*/
extern void MyVectorHeapExaminSortOK(HMYVECTOR hv);

/*
*
*��ӡ��vector
*
*/
extern void MyVectorPrint(HMYVECTOR hv);


#endif
















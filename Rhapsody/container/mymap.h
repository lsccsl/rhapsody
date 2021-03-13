/*
*
* mymap.h ӳ�� 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/


#ifndef __MYMAP_H__
#define __MYMAP_H__


#include "myobj.h"
#include "myrbtree.h"


typedef struct __mymap_handle_
{ int unused;}*HMYMAP;

typedef struct __mymap_iter_handle_
{ int unused;}*HMYMAP_ITER;


//todo ...
#define MyMapSearch(__hm_, __k_) (HMYMAP_ITER)MyRBTreeSearch((HMYRBTREE)__hm_, __k_)

#define MyMapGetIterKey(__it_) MyRBTreeGetIterKey((HMYRBTREE_ITER) __it_)

#define MyMapGetNext(__it_) (HMYMAP_ITER)MyRBTreeGetNext((HMYRBTREE_ITER) __it_)

#define MyMapGetPrev(__it_) (HMYMAP_ITER)MyRBTreeGetPrev((HMYRBTREE_ITER) __it_)

#define MyMapBegin(__hm_) (HMYMAP)MyRBTreeBegin((HMYRBTREE) __hm_)

#define MyMapEnd(__hm_) (HMYMAP)MyRBTreeEnd((HMYRBTREE) __hm_)

/*����������*/
#define MyMapLayer(__hm_, bmax) MyRBTreeLayer((HMYRBTREE)__hm_, bmax)

/*��������Ƿ�Ϸ�*/
#define MyMapExamin(__hm_) MyRBTreeExamin((HMYRBTREE)__hm_)

/*��ȡ����*/
#define MyMapGetRealCount(__hm_) MyRBTreeGetRealCount((HMYRBTREE)__hm_)


#ifdef __cplusplus
extern "C"
{
#endif


/*
*
*ӳ��Ĺ���
*
*/
extern HMYMAP MyMapRealConstruct(HMYMEMPOOL hm, myrbtree_compare compare, 
	myobj_ops * key_op,
	myobj_ops * data_op);
#define MyMapConstruct(__hm_, __compare_) MyMapRealConstruct(__hm_, __compare_, NULL, NULL/*, NULL, NULL, NULL, NULL*/)

/*
*
*ӳ�������
*
*/
extern void MyMapDestruct(HMYMAP hmap);

/*
*
*����ֵ - δ���
*
*/
extern int MyMapSetIterData(HMYMAP_ITER it, const void * data, size_t data_size);

/*
*
*���һ�ڵ㲻�ظ�
*
*/
extern HMYMAP_ITER MyMapInsertUnique(HMYMAP hmap, const void * key, const size_t key_size, const void * data, const size_t data_size);

/*
*
*��ȡ��������ֵ
*
*/
extern void * MyMapGetIterData(const HMYMAP_ITER it, size_t * data_size);

/*
*
*���ݼ�ɾ��
*
*/
extern int MyMapDelKey(HMYMAP hmap, const void * key);

/*
*
*���ݵ�����ɾ��
*
*/
extern void MyMapDelIter(HMYMAP hmap, HMYMAP_ITER it);

/*
*
*ɾ�����нڵ�
*
*/
extern void MyMapClear(HMYMAP hmap);


#ifdef __cplusplus
}
#endif


#endif














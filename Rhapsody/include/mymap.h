/*
*
* mymap.h 映射 
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

/*计算最大层数*/
#define MyMapLayer(__hm_, bmax) MyRBTreeLayer((HMYRBTREE)__hm_, bmax)

/*检查红黑树是否合法*/
#define MyMapExamin(__hm_) MyRBTreeExamin((HMYRBTREE)__hm_)

/*获取个数*/
#define MyMapGetRealCount(__hm_) MyRBTreeGetRealCount((HMYRBTREE)__hm_)


#ifdef __cplusplus
extern "C"
{
#endif


/*
*
*映射的构造
*
*/
extern HMYMAP MyMapRealConstruct(HMYMEMPOOL hm, myrbtree_compare compare, 
	myobj_ops * key_op,
	myobj_ops * data_op);
#define MyMapConstruct(__hm_, __compare_) MyMapRealConstruct(__hm_, __compare_, NULL, NULL/*, NULL, NULL, NULL, NULL*/)

/*
*
*映射的析构
*
*/
extern void MyMapDestruct(HMYMAP hmap);

/*
*
*设置值 - 未完成
*
*/
extern int MyMapSetIterData(HMYMAP_ITER it, const void * data, size_t data_size);

/*
*
*添加一节点不重复
*
*/
extern HMYMAP_ITER MyMapInsertUnique(HMYMAP hmap, const void * key, const size_t key_size, const void * data, const size_t data_size);

/*
*
*获取迭代器的值
*
*/
extern void * MyMapGetIterData(const HMYMAP_ITER it, size_t * data_size);

/*
*
*根据键删除
*
*/
extern int MyMapDelKey(HMYMAP hmap, const void * key);

/*
*
*根据迭代器删除
*
*/
extern void MyMapDelIter(HMYMAP hmap, HMYMAP_ITER it);

/*
*
*删除所有节点
*
*/
extern void MyMapClear(HMYMAP hmap);


#ifdef __cplusplus
}
#endif


#endif














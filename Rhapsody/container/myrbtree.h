/*
*
* myrbtree.h 红黑树
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/
#ifndef __MYRBTREE_H__
#define __MYRBTREE_H__


#include "mymempool.h"


/*
*
*1 表示 key1 比 key2 大
*0 表示 key1 比 key2 小 
*
*/
typedef int (*myrbtree_compare)(const void * key1, const void * key2);

typedef struct __handle_myrbtree
{
	int unused;
}handle_myrbtree;
typedef handle_myrbtree * HMYRBTREE;

typedef struct __handle_myrbtree_iter
{
	int unused;
}handle_myrbtree_iter;
typedef handle_myrbtree_iter * HMYRBTREE_ITER;

/*
*
*创建rb树
*
*/
extern HMYRBTREE MyRBTreeConstruct(HMYMEMPOOL hm, myrbtree_compare compare);

/*
*
*销毁rb树
*
*/
extern void MyRBTreeDestruct(HMYRBTREE htree);

/*
*
*删除所有节点
*
*/
extern void MyRBTreeClear(HMYRBTREE htree);

/*
*
*往rb树中插入一个节点
*
*/
extern HMYRBTREE_ITER MyRBTreeInsertEqual(HMYRBTREE htree, const void * key, const void * userdata);

/*
*
*往rb树中插入一个节点
*
*/
extern HMYRBTREE_ITER MyRBTreeInsertUnique(HMYRBTREE htree, const void * key, const void * userdata);

/*
*
*从rb树中删除一个节点
*
*/
extern void MyRBTreeDelIter(HMYRBTREE htree, HMYRBTREE_ITER iter, void ** key, void ** data);

/*
*
*根据键值删除一个节点
*成功删除返回0, 否则返回-1
*
*/
extern int MyRBTreeDelKey(HMYRBTREE htree, const void * key, void ** key_ret, void ** data_ret);

/*
*
*获取节点的用户数据
*
*/
extern void * MyRBTreeGetIterData(const HMYRBTREE_ITER iter);

/*
*
*获取节点的键
*
*/
extern const void * MyRBTreeGetIterKey(const HMYRBTREE_ITER iter);

/*
*
*查找节点
*
*/
extern HMYRBTREE_ITER MyRBTreeSearch(const HMYRBTREE htree, const void * key);

/*
*
*计算最大层数
*
*/
extern int MyRBTreeLayer(const HMYRBTREE htree, int bmax);

/*
*
*"获取第一个节点"
*
*/
extern HMYRBTREE_ITER MyRBTreeBegin(const HMYRBTREE htree);

/*
*
*"获取最后一个节点"
*
*/
extern HMYRBTREE_ITER MyRBTreeEnd(const HMYRBTREE htree);

/*
*
*获取"下"一个节点
*
*/
extern HMYRBTREE_ITER MyRBTreeGetNext(const HMYRBTREE_ITER it);

/*
*
*获取"上"一个节点
*
*/
extern HMYRBTREE_ITER MyRBTreeGetPrev(const HMYRBTREE_ITER it);

/*
*
*检查红黑树是否合法
*
*/
extern int MyRBTreeExamin(const HMYRBTREE htree);

/*
*
*获取个数
*
*/
extern int MyRBTreeGetRealCount(const HMYRBTREE htree);


#endif


















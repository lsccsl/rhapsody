/*
*
* myrbtree.h �����
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/
#ifndef __MYRBTREE_H__
#define __MYRBTREE_H__


#include "mymempool.h"


/*
*
*1 ��ʾ key1 �� key2 ��
*0 ��ʾ key1 �� key2 С 
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
*����rb��
*
*/
extern HMYRBTREE MyRBTreeConstruct(HMYMEMPOOL hm, myrbtree_compare compare);

/*
*
*����rb��
*
*/
extern void MyRBTreeDestruct(HMYRBTREE htree);

/*
*
*ɾ�����нڵ�
*
*/
extern void MyRBTreeClear(HMYRBTREE htree);

/*
*
*��rb���в���һ���ڵ�
*
*/
extern HMYRBTREE_ITER MyRBTreeInsertEqual(HMYRBTREE htree, const void * key, const void * userdata);

/*
*
*��rb���в���һ���ڵ�
*
*/
extern HMYRBTREE_ITER MyRBTreeInsertUnique(HMYRBTREE htree, const void * key, const void * userdata);

/*
*
*��rb����ɾ��һ���ڵ�
*
*/
extern void MyRBTreeDelIter(HMYRBTREE htree, HMYRBTREE_ITER iter, void ** key, void ** data);

/*
*
*���ݼ�ֵɾ��һ���ڵ�
*�ɹ�ɾ������0, ���򷵻�-1
*
*/
extern int MyRBTreeDelKey(HMYRBTREE htree, const void * key, void ** key_ret, void ** data_ret);

/*
*
*��ȡ�ڵ���û�����
*
*/
extern void * MyRBTreeGetIterData(const HMYRBTREE_ITER iter);

/*
*
*��ȡ�ڵ�ļ�
*
*/
extern const void * MyRBTreeGetIterKey(const HMYRBTREE_ITER iter);

/*
*
*���ҽڵ�
*
*/
extern HMYRBTREE_ITER MyRBTreeSearch(const HMYRBTREE htree, const void * key);

/*
*
*����������
*
*/
extern int MyRBTreeLayer(const HMYRBTREE htree, int bmax);

/*
*
*"��ȡ��һ���ڵ�"
*
*/
extern HMYRBTREE_ITER MyRBTreeBegin(const HMYRBTREE htree);

/*
*
*"��ȡ���һ���ڵ�"
*
*/
extern HMYRBTREE_ITER MyRBTreeEnd(const HMYRBTREE htree);

/*
*
*��ȡ"��"һ���ڵ�
*
*/
extern HMYRBTREE_ITER MyRBTreeGetNext(const HMYRBTREE_ITER it);

/*
*
*��ȡ"��"һ���ڵ�
*
*/
extern HMYRBTREE_ITER MyRBTreeGetPrev(const HMYRBTREE_ITER it);

/*
*
*��������Ƿ�Ϸ�
*
*/
extern int MyRBTreeExamin(const HMYRBTREE htree);

/*
*
*��ȡ����
*
*/
extern int MyRBTreeGetRealCount(const HMYRBTREE htree);


#endif


















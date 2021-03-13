/**
 *
 * @file myAVLTree.h avlƽ����
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __MYAVLTREE_H__
#define __MYAVLTREE_H__


#include "mymempool.h"
#include "MyfunctionDef.h"


/**
 * @brief �������
 */
typedef struct __myavltree_handle__
{int unused;}*HMYAVL_TREE;

/**
 * @brief �������
 */
typedef struct __myavltree_iter__
{int unused;}*HMYAVL_TREE_ITER;


/**
 *
 * @brief ����avl��
 *
 */
extern HMYAVL_TREE MyAVLTreeConstruct(HMYMEMPOOL hm, ALG_COMPARE compare);

/**
 *
 * @brief ����avl��
 *
 */
extern void MyAVLTreeDestruct(HMYAVL_TREE htree);

/**
 *
 * @brief ���һ����¼
 *
 */
extern int MyAVLTreeInsert(HMYAVL_TREE htree, const void * key, const void * data);

/**
 *
 * @brief ���ݹؼ���ɾ��һ����¼
 *
 */
extern int MyAVLTreeDel(HMYAVL_TREE htree, const void * key, void ** pkey, void ** pdata);

/**
 *
 * @brief ���Ҽ�¼
 *
 */
extern HMYAVL_TREE_ITER MyAVLTreeSearch(HMYAVL_TREE htree, const void * key);

/**
 *
 * @brief ��ȡ��������ֵ��
 *
 */
extern void * MyAVLTreeGetIterData(HMYAVL_TREE_ITER it);

/**
 *
 * @brief ��ȡ�������Ĺؼ�����
 *
 */
extern const void * MyAVLTreeGetIterKey(HMYAVL_TREE_ITER it);

/**
 *
 * @brief avl�Ƿ���Ϲ���
 *
 */
extern void MyAVLTreeExamin(HMYAVL_TREE htree);

/**
 *
 * @brief ����avl���·��
 *
 */
extern int MyAVLTreeMaxPath(HMYAVL_TREE htree);

/**
 *
 * @brief ����avl���·��
 *
 */
extern int MyAVLTreeMinPath(HMYAVL_TREE htree);

/**
 *
 * @brief ����avl���·��
 *
 */
extern int MyAVLTreeGetCount(HMYAVL_TREE htree);


#endif

















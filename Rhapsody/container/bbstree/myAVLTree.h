/**
 *
 * @file myAVLTree.h avl平衡树
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __MYAVLTREE_H__
#define __MYAVLTREE_H__


#include "mymempool.h"
#include "MyfunctionDef.h"


/**
 * @brief 句柄声明
 */
typedef struct __myavltree_handle__
{int unused;}*HMYAVL_TREE;

/**
 * @brief 句柄声明
 */
typedef struct __myavltree_iter__
{int unused;}*HMYAVL_TREE_ITER;


/**
 *
 * @brief 构造avl树
 *
 */
extern HMYAVL_TREE MyAVLTreeConstruct(HMYMEMPOOL hm, ALG_COMPARE compare);

/**
 *
 * @brief 销毁avl树
 *
 */
extern void MyAVLTreeDestruct(HMYAVL_TREE htree);

/**
 *
 * @brief 添加一条记录
 *
 */
extern int MyAVLTreeInsert(HMYAVL_TREE htree, const void * key, const void * data);

/**
 *
 * @brief 根据关键字删除一条记录
 *
 */
extern int MyAVLTreeDel(HMYAVL_TREE htree, const void * key, void ** pkey, void ** pdata);

/**
 *
 * @brief 查找记录
 *
 */
extern HMYAVL_TREE_ITER MyAVLTreeSearch(HMYAVL_TREE htree, const void * key);

/**
 *
 * @brief 获取迭代器的值域
 *
 */
extern void * MyAVLTreeGetIterData(HMYAVL_TREE_ITER it);

/**
 *
 * @brief 获取迭代器的关键字域
 *
 */
extern const void * MyAVLTreeGetIterKey(HMYAVL_TREE_ITER it);

/**
 *
 * @brief avl是否符合规则
 *
 */
extern void MyAVLTreeExamin(HMYAVL_TREE htree);

/**
 *
 * @brief 计算avl最大路径
 *
 */
extern int MyAVLTreeMaxPath(HMYAVL_TREE htree);

/**
 *
 * @brief 计算avl最大路径
 *
 */
extern int MyAVLTreeMinPath(HMYAVL_TREE htree);

/**
 *
 * @brief 计算avl最大路径
 *
 */
extern int MyAVLTreeGetCount(HMYAVL_TREE htree);


#endif

















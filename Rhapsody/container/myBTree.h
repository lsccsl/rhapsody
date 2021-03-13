/**
 *
 * @file myBTree.c B树
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __MYBTREE_H__
#define __MYBTREE_H__


#include "mymempool.h"
#include "MyfunctionDef.h"


struct __mybtree_t_;
typedef struct __mybtree_t_ * HMYBTREE;


/**
 * @brief B树的创建
 * @param mini_sub:最小子节点的数目
 */
extern HMYBTREE MyBTreeConstruct(HMYMEMPOOL hm, ALG_COMPARE compare, size_t min_sub);

/**
 * @brief B树的销毁
 */
extern void MyBTreeDestruct(HMYBTREE hbtree);

/**
 * @brief 往B树中添加一个节点
 * @param index_info:要添加的索引信息
 * @return 0:成功 其它:失败
 */
extern int MyBTreeAdd(HMYBTREE hbtree, void * index_info);

/**
 * @brief 从B树中删除一个节点
 * @param index_info:要删除的索引信息
 * @param index_info_out:返回存储在B树里的索引信息指针给用户
 * @return 0:成功 其它:失败
 */
extern int MyBTreeDel(HMYBTREE hbtree, void * index_info, void ** index_info_out);

/**
 * @brief 在B树中查找
 * @param index_info:要查找的索引信息
 * @param index_info_out:返回存储在B树里的索引信息指针给用户
 * @return 0:成功 其它:失败
 */
extern int MyBTreeSearch(HMYBTREE hbtree, const void * index_info, void ** index_info_out);

/**
 * @brief 获取B树中节点的个数
 */
extern size_t MyBTreeGetCount(HMYBTREE hbtree);

/**
 * @brief 递归计算B树中的节点个数
 */
extern size_t MyBTreeCalCount(HMYBTREE hbtree);

/**
 * @brief 检查一棵B树是否合法
 */
extern int MyBTreeExamin(HMYBTREE hbtree, int look_printf);


#endif









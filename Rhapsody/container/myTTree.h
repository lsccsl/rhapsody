/**
 *
 * @file myTTree.h T树
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __MYTTREE_H__
#define __MYTTREE_H__


#include "mymempool.h"
#include "MyfunctionDef.h"


struct __myttree_t_;
typedef struct __myttree_t_ * HMYTTREE;


/**
 *
 * @brief T树构造
 *
 * @param hm:内存池
 * @param compare:比较回调函数
 * @param key_op:描述关键字的构造析构与拷贝
 * @param data_op:描述数据的构造析构与拷贝
 * @param underflow:T树节点关键字的下限
 * @param overflow:T树节点关键字的上限
 *
 */
extern HMYTTREE MyTTreeConstruct(HMYMEMPOOL hm, ALG_COMPARE compare, size_t underflow, size_t overflow);

/**
 * @brief T树析构
 */
extern void MyTTreeDestruct(HMYTTREE httree);

/**
 * @brief 添加记录
 *
 * @param key:关键字
 * @param index_info:索引信息
 * @param index_info_size:索引信息的大小
 */
extern int MyTTreeAdd(HMYTTREE httree, const void * index_info);

/**
 * @brief 删除记录
 *
 * @param key:要删除的关键字
 */
extern int MyTTreeDel(HMYTTREE httree, const void * index_info, void ** index_info_out);

/**
 * @brief 查找记录
 *
 * @param key:要查找的关键字
 */
extern int MyTTreeSearch(HMYTTREE httree, const void * index_info, void ** index_info_out);

/**
 * 检查T树的合法性
 */
extern void MyTTreeExamin(HMYTTREE httree, int bprint);

/**
 * 获取T树中的节点个数
 */
extern size_t MyTTreeGetCount(HMYTTREE httree);


#endif
















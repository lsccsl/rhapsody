/**
 *
 * @file __bstree.h 记录二叉搜索树的常用操作与通用结构定义
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __BSTREE_H__
#define __BSTREE_H__


#include "MyfunctionDef.h"


/**
 * @brief 二叉搜索树节点定义
 */
typedef struct __bstree_node_t_
{
	/*
	* 父节点 左右孩子节点指针
	*/
	struct __bstree_node_t_ * parent;
	struct __bstree_node_t_ * left;
	struct __bstree_node_t_ * right;

	/*
	* 记录关键字信息
	*/
	void * key;
}bstree_node_t;


/**
 *
 * @brief 左旋 以node为轴进行如下图所示的旋转
 *
 *   A                node
 *    \              /
 *     \    ---->   /
 *      node       A
 *
 * @param root:根节点(根据旋转后的情况,函数会自动判断是否改变root)
 * @param node:作为轴的节点
 *
 */
extern void __rotate_left(bstree_node_t ** root, bstree_node_t * node);

/**
 *
 * @brief 右旋 以node为轴进行如下图所示的旋转
 *
 *     A        node
 *    /          \
 *   /    --->    \
 *  node           A
 *
 * @param root:根节点(根据旋转后的情况,函数会自动判断是否改变root)
 * @param node:作为轴的节点
 *
 */
extern void __rotate_right(bstree_node_t ** root, bstree_node_t * node);

/**
 * @brief 查找某个节点
 * @param root:根节点
 * @param key:关键字
 * @param compare:比较回调函数
 */
extern bstree_node_t * __bstree_searchex(bstree_node_t * root, const void * key, ALG_COMPARE compare, bstree_node_t ** parent, const void * context);

/**
 * @brief 查找某个节点
 * @param root:根节点
 * @param key:关键字
 * @param compare:比较回调函数
 */
extern bstree_node_t * __bstree_search(bstree_node_t * root, const void * key, ALG_COMPARE compare, const void * context);

/**
 * @brief 计算一棵树的最大(最小)路径
 * @param root:根节点
 * @param key:bmax 1:计算最大路径 0:计算最小路径
 */
extern int __bstree_get_path(bstree_node_t * root, int bmax);

/**
 * @brief 计算一棵树所包含的节点
 */
extern int __bstree_get_count(bstree_node_t * root);


/**
 * @brief 描述在__bstree_erase函数中如何释放每个节点
 * @param context:用户的上下文数据
 * @param root:根节点
 */
typedef void (*__bstree_destroy_node_cb)(void * context, bstree_node_t * root);

/**
 * @brief 描述如何删除一个棵树中的所有节点
 */
extern void __bstree_erase(bstree_node_t * root, void * context, __bstree_destroy_node_cb cb);


#endif


















/**
 *
 * @file __avl_tree.h avl平衡树
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef ____AVL_TREE_H__
#define ____AVL_TREE_H__


#include "__bstree.h"


/**
 *
 * @brief avl树节点定义
 *
 */
typedef struct ____avltree_node_t_
{
	bstree_node_t base;

	/*
	* 当前树的高度,做为判断平衡与否的依据
	*/
	size_t height;
}__avltree_node_t;


/**
 * @brief 获取左边节点的高度
 */
#define __avltree_left_height(node) (assert(node), ((node)->base.left) ? ((__avltree_node_t *)((node)->base.left))->height : 0)

/**
 * @brief 右边节点的高度
 */
#define __avltree_right_height(node) (assert(node), (node)->base.right ? ((__avltree_node_t *)((node)->base.right))->height : 0)

/**
 * @brief 创建节点
 */
extern size_t __avltree_cal_height(__avltree_node_t * node);

/**
 * @brief 从node开始回溯,平衡受影响的路径,如果满足平衡条件且父节点没有长高,停止回溯
 */
extern void __avltree_balance(__avltree_node_t ** root, __avltree_node_t * node);

/**
 * @brief 删除指定节点node
 */
extern void __avltree_del(__avltree_node_t ** root, __avltree_node_t * node);


#endif




















/**
 *
 * @file __avl_tree.h avlƽ����
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef ____AVL_TREE_H__
#define ____AVL_TREE_H__


#include "__bstree.h"


/**
 *
 * @brief avl���ڵ㶨��
 *
 */
typedef struct ____avltree_node_t_
{
	bstree_node_t base;

	/*
	* ��ǰ���ĸ߶�,��Ϊ�ж�ƽ����������
	*/
	size_t height;
}__avltree_node_t;


/**
 * @brief ��ȡ��߽ڵ�ĸ߶�
 */
#define __avltree_left_height(node) (assert(node), ((node)->base.left) ? ((__avltree_node_t *)((node)->base.left))->height : 0)

/**
 * @brief �ұ߽ڵ�ĸ߶�
 */
#define __avltree_right_height(node) (assert(node), (node)->base.right ? ((__avltree_node_t *)((node)->base.right))->height : 0)

/**
 * @brief �����ڵ�
 */
extern size_t __avltree_cal_height(__avltree_node_t * node);

/**
 * @brief ��node��ʼ����,ƽ����Ӱ���·��,�������ƽ�������Ҹ��ڵ�û�г���,ֹͣ����
 */
extern void __avltree_balance(__avltree_node_t ** root, __avltree_node_t * node);

/**
 * @brief ɾ��ָ���ڵ�node
 */
extern void __avltree_del(__avltree_node_t ** root, __avltree_node_t * node);


#endif




















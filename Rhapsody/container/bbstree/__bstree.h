/**
 *
 * @file __bstree.h ��¼�����������ĳ��ò�����ͨ�ýṹ����
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __BSTREE_H__
#define __BSTREE_H__


#include "MyfunctionDef.h"


/**
 * @brief �����������ڵ㶨��
 */
typedef struct __bstree_node_t_
{
	/*
	* ���ڵ� ���Һ��ӽڵ�ָ��
	*/
	struct __bstree_node_t_ * parent;
	struct __bstree_node_t_ * left;
	struct __bstree_node_t_ * right;

	/*
	* ��¼�ؼ�����Ϣ
	*/
	void * key;
}bstree_node_t;


/**
 *
 * @brief ���� ��nodeΪ���������ͼ��ʾ����ת
 *
 *   A                node
 *    \              /
 *     \    ---->   /
 *      node       A
 *
 * @param root:���ڵ�(������ת������,�������Զ��ж��Ƿ�ı�root)
 * @param node:��Ϊ��Ľڵ�
 *
 */
extern void __rotate_left(bstree_node_t ** root, bstree_node_t * node);

/**
 *
 * @brief ���� ��nodeΪ���������ͼ��ʾ����ת
 *
 *     A        node
 *    /          \
 *   /    --->    \
 *  node           A
 *
 * @param root:���ڵ�(������ת������,�������Զ��ж��Ƿ�ı�root)
 * @param node:��Ϊ��Ľڵ�
 *
 */
extern void __rotate_right(bstree_node_t ** root, bstree_node_t * node);

/**
 * @brief ����ĳ���ڵ�
 * @param root:���ڵ�
 * @param key:�ؼ���
 * @param compare:�Ƚϻص�����
 */
extern bstree_node_t * __bstree_searchex(bstree_node_t * root, const void * key, ALG_COMPARE compare, bstree_node_t ** parent, const void * context);

/**
 * @brief ����ĳ���ڵ�
 * @param root:���ڵ�
 * @param key:�ؼ���
 * @param compare:�Ƚϻص�����
 */
extern bstree_node_t * __bstree_search(bstree_node_t * root, const void * key, ALG_COMPARE compare, const void * context);

/**
 * @brief ����һ���������(��С)·��
 * @param root:���ڵ�
 * @param key:bmax 1:�������·�� 0:������С·��
 */
extern int __bstree_get_path(bstree_node_t * root, int bmax);

/**
 * @brief ����һ�����������Ľڵ�
 */
extern int __bstree_get_count(bstree_node_t * root);


/**
 * @brief ������__bstree_erase����������ͷ�ÿ���ڵ�
 * @param context:�û�������������
 * @param root:���ڵ�
 */
typedef void (*__bstree_destroy_node_cb)(void * context, bstree_node_t * root);

/**
 * @brief �������ɾ��һ�������е����нڵ�
 */
extern void __bstree_erase(bstree_node_t * root, void * context, __bstree_destroy_node_cb cb);


#endif


















/**
 *
 * @file __bstree.c 2007-8-17 14:46 ��¼�����������ĳ��ò�����ͨ�ýṹ����
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "__bstree.h"
#include <stdlib.h>
#include <assert.h>


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
void __rotate_left(bstree_node_t ** root, bstree_node_t * node)
{
	bstree_node_t * A_node = NULL;

	assert(root && node && node->parent);

	A_node = node->parent;

	node->parent = A_node->parent;
	if(A_node->parent)
	{
		if(A_node == A_node->parent->left)
			A_node->parent->left = node;
		else
			A_node->parent->right = node;
	}
	A_node->parent = node;

	A_node->right = node->left;
	if(node->left)
		node->left->parent = A_node;

	node->left = A_node;

	if(A_node == *root)
		*root = node;

	assert(NULL == *root || NULL == (*root)->parent);
}

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
void __rotate_right(bstree_node_t ** root, bstree_node_t * node)
{
	bstree_node_t * A_node = NULL;

	assert(root && node && node->parent);

	A_node = node->parent;
	node->parent = A_node->parent;
	if(A_node->parent)
	{
		if(A_node == A_node->parent->left)
			A_node->parent->left = node;
		else
			A_node->parent->right = node;
	}
	A_node->parent = node;

	A_node->left = node->right;
	if(node->right)
		node->right->parent = A_node;

	node->right = A_node;

	if(A_node == *root)
		*root = node;

	assert(NULL == (*root) || NULL == (*root)->parent);
}

/**
 * @brief ����ĳ���ڵ�
 * @param root:���ڵ�
 * @param key:�ؼ���
 * @param compare:�Ƚϻص�����
 */
bstree_node_t * __bstree_searchex(bstree_node_t * root, const void * key, ALG_COMPARE compare, bstree_node_t ** parent, const void * context)
{
	bstree_node_t * y = NULL;/* ��¼���һ��������key�Ľڵ� */
	bstree_node_t * x = root;

	if(NULL == root)
		return NULL;

	assert(parent && compare);

	*parent = root;

	while(x)
	{
		int ret = compare(x->key, key, context);

		*parent = x;

		if(ret > 0)
			x = x->left;
		else if(ret < 0)
			x = x->right;
		else
			return x;
	}

	return NULL;

	/*while(x)
	{
		*parent = x;

		if(compare(x->key, key) < 0)
			y = x, x = x->right;
		else
			x = x->left;
	}

	return (NULL == y || compare(key, y->key) > 0)?NULL:y;*/
}

/**
 * @brief ����ĳ���ڵ�
 * @param root:���ڵ�
 * @param key:�ؼ���
 * @param compare:�Ƚϻص�����
 */
bstree_node_t * __bstree_search(bstree_node_t * root, const void * key, ALG_COMPARE compare, const void * context)
{
	bstree_node_t * y = NULL;/* ��¼���һ��������key�Ľڵ� */
	bstree_node_t * x = root;

	assert(compare);

	if(NULL == root)
		return NULL;

	while(x)
	{
		int ret = compare(x->key, key, context);

		if(ret > 0)
			x = x->left;
		else if(ret < 0)
			x = x->right;
		else
			return x;
	}

	return NULL;

	/*while(x)
	{
		if(compare(x->key, key) < 0)
			y = x, x = x->right;
		else
			x = x->left;
	}

	return (NULL == y || compare(key, y->key) > 0)?NULL:y;*/
}

/**
 * @brief ����һ���������(��С)·��
 * @param root:���ڵ�
 * @param key:bmax 1:�������·�� 0:������С·��
 */
int __bstree_get_path(bstree_node_t * root, int bmax)
{
	int left = 0;
	int right = 0;

	if(NULL == root)
		return 0;

	left = __bstree_get_path(root->left, bmax);
	right = __bstree_get_path(root->right, bmax);

	if(left > right && bmax)
		return left + 1;
	else
		return right + 1;
}

/**
 * @brief ����һ�����������Ľڵ�
 */
int __bstree_get_count(bstree_node_t * root)
{
	int left = 0;
	int right = 0;

	if(NULL == root)
		return 0;

	if(root->left)
		left = __bstree_get_count(root->left);

	if(root->right)
		right = __bstree_get_count(root->right);

	return left + right + 1;
}

/**
 * @brief �������ɾ��һ�������е����нڵ�
 */
void __bstree_erase(bstree_node_t * root, void * context, __bstree_destroy_node_cb cb)
{
	assert(root);

	while(root)
	{
		bstree_node_t * y = root->left;

		if(root->right)
			__bstree_erase(root->right, context, cb);

		cb(context, root);

		root = y;
	}
}
























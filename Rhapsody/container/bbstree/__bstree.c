/**
 *
 * @file __bstree.c 2007-8-17 14:46 记录二叉搜索树的常用操作与通用结构定义
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "__bstree.h"
#include <stdlib.h>
#include <assert.h>


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
 * @brief 查找某个节点
 * @param root:根节点
 * @param key:关键字
 * @param compare:比较回调函数
 */
bstree_node_t * __bstree_searchex(bstree_node_t * root, const void * key, ALG_COMPARE compare, bstree_node_t ** parent, const void * context)
{
	bstree_node_t * y = NULL;/* 记录最后一个不大于key的节点 */
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
 * @brief 查找某个节点
 * @param root:根节点
 * @param key:关键字
 * @param compare:比较回调函数
 */
bstree_node_t * __bstree_search(bstree_node_t * root, const void * key, ALG_COMPARE compare, const void * context)
{
	bstree_node_t * y = NULL;/* 记录最后一个不大于key的节点 */
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
 * @brief 计算一棵树的最大(最小)路径
 * @param root:根节点
 * @param key:bmax 1:计算最大路径 0:计算最小路径
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
 * @brief 计算一棵树所包含的节点
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
 * @brief 描述如何删除一个棵树中的所有节点
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
























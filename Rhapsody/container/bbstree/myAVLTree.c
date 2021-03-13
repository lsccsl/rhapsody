/**
 *
 * @file myAVLTree.c avlƽ����
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "myAVLTree.h"

#include <assert.h>
#include <string.h>

#include "__avl_tree.h"
#include "myutility.h"


/**
 *
 * @brief avl���ڵ㶨��
 *
 */
typedef struct __myavltree_node_t_
{
	__avltree_node_t base;

	/*
	* ��¼value����Ϣ
	*/
	void * userdata;
}myavltree_node_t;

/**
 *
 * @brief avl����ص���Ϣ�ṹ
 *
 */
typedef struct __myavltree_t_
{
	/*
	* �ڴ�ض���
	*/
	HMYMEMPOOL hm;

	/*
	* ���ڵ�
	*/
	myavltree_node_t * root;

	/*
	* �Ƚϻص�
	*/
	ALG_COMPARE compare;

	/* ���� */
	void * context;
}myavltree_t;


/**
 * @brief �Ƚ�
 */
static __INLINE__ int avltree_compare(myavltree_t * avl_tree, const void * key1, const void * key2)
{
	assert(avl_tree && avl_tree->compare);

	return avl_tree->compare(key1, key2, avl_tree->context);
}

/**
 * @brief �����ڵ�
 */
static __INLINE__ myavltree_node_t * avltree_create_node(myavltree_t * avl_tree, const void * key, const void * data)
{
	myavltree_node_t * node = (myavltree_node_t *)MyMemPoolMalloc(avl_tree->hm, sizeof(*node));
	if(NULL == node)
		return NULL;

	memset(node, 0, sizeof(*node));

	node->base.base.key = (void *)key;
	node->userdata = (void *)data;
	node->base.height = 1;

	return node;
}

static __INLINE__ void avltree_destroy_node(myavltree_t * avl_tree, myavltree_node_t * node)
{
	assert(avl_tree && node);

	MyMemPoolFree(avl_tree->hm, node);
}

static __INLINE__ size_t avltree_examin(myavltree_t * tree, myavltree_node_t * node)
{
	size_t left = 0;
	size_t right = 0;

	assert(tree);

	if(node->base.base.left)
	{
		left = avltree_examin(tree, (myavltree_node_t *)node->base.base.left);

		/*if(node->base.base.left->parent != (bstree_node_t *)node)
		{
			int a = 0;
		}*/
		assert(node->base.base.left->parent == (bstree_node_t *)node);
		assert(avltree_compare(tree, node->base.base.key, node->base.base.left->key) > 0);
	}
	else
		left = 0;

	if(node->base.base.right)
	{
		right = avltree_examin(tree, (myavltree_node_t *)node->base.base.right);
		assert(node->base.base.right->parent == (bstree_node_t *)node);
		assert(avltree_compare(tree, node->base.base.right->key, node->base.base.key) > 0);
	}
	else
		right = 0;

	assert(right + 1 == left || left + 1 == right || left == right);

	/*if(node->base.height != right + 1 && node->base.height != left + 1)
	{
		int a= 0;
	}*/
	assert(node->base.height == right + 1 || node->base.height == left + 1);

	/*if(right >= node->base.height || left >= node->base.height)
	{
		int b = 0;
	}*/
	assert(right < node->base.height && left < node->base.height);

	return (left > right)?(left + 1):(right + 1);
}

static __INLINE__ void avl_tree_erase(myavltree_t * avl_tree, myavltree_node_t * node)
{
	assert(node);

	while(node)
	{
		myavltree_node_t * y = (myavltree_node_t *)node->base.base.left;

		if(node->base.base.right)
			avl_tree_erase(avl_tree, (myavltree_node_t *)node->base.base.right);

		avltree_destroy_node(avl_tree, node);

		node = y;
	}
}


/**
 *
 * @brief ����avl��
 *
 */
HMYAVL_TREE MyAVLTreeConstruct(HMYMEMPOOL hm, ALG_COMPARE compare)
{
	myavltree_t * avl_tree = (myavltree_t *)MyMemPoolMalloc(hm, sizeof(*avl_tree));
	if(NULL == avl_tree)
		return NULL;

	assert(compare);

	avl_tree->hm = hm;
	avl_tree->compare = compare;
	avl_tree->root = NULL;

	return (HMYAVL_TREE)avl_tree;
}

/**
 *
 * @brief ����avl��
 *
 */
void MyAVLTreeDestruct(HMYAVL_TREE htree)
{
	myavltree_t * avl_tree = (myavltree_t *)htree;
	if(NULL == avl_tree)
		return;
	
	if(avl_tree->root)
		avl_tree_erase(avl_tree, avl_tree->root);

	MyMemPoolFree(avl_tree->hm, avl_tree);
}

/**
 *
 * @brief ���һ����¼
 *
 */
int MyAVLTreeInsert(HMYAVL_TREE htree, const void * key, const void * data)
{
	myavltree_node_t * node = NULL;
	myavltree_node_t * parent = NULL;

	myavltree_t * avl_tree = (myavltree_t *)htree;
	if(NULL == avl_tree)
		return -1;

	if(__bstree_searchex((bstree_node_t *)avl_tree->root, key, avl_tree->compare, (bstree_node_t **)&parent, avl_tree->context))
		return -1;

	node = avltree_create_node(avl_tree, key, data);

	/* �����ڵ� */
	if(NULL == node)
		return -1;

	/* �����Ϊ��,��root = node,�������� */
	if(NULL == avl_tree->root)
	{
		avl_tree->root = node;
		return 0;
	}

	assert(parent);

	node->base.base.parent = (bstree_node_t *)parent;

	if(avltree_compare(avl_tree, parent->base.base.key, key) > 0)
		parent->base.base.left = (bstree_node_t *)node;
	else
		parent->base.base.right = (bstree_node_t *)node;

	assert(parent->base.height <= 2 && parent->base.height >= 1);

	__avltree_balance((__avltree_node_t **)&avl_tree->root, (__avltree_node_t *)parent);

	return 0;
}

/**
 *
 * @brief ���ݹؼ���ɾ��һ����¼
 *
 */
int MyAVLTreeDel(HMYAVL_TREE htree, const void * key, void ** pkey, void ** pdata)
{
	myavltree_node_t * node = NULL;

	myavltree_t * avl_tree = (myavltree_t *)htree;
	if(NULL == avl_tree)
		return -1;

	node = (myavltree_node_t *)__bstree_search((bstree_node_t *)avl_tree->root, key, avl_tree->compare, avl_tree->context);
	if(NULL == node)
		return -1;

	__avltree_del((__avltree_node_t **)&avl_tree->root, (__avltree_node_t *)node);

	if(pkey)
		*pkey = node->base.base.key;
	if(pdata)
		*pdata = node->userdata;

	avltree_destroy_node(avl_tree, node);

	return 0;
}

/**
 *
 * @brief ���Ҽ�¼
 *
 */
HMYAVL_TREE_ITER MyAVLTreeSearch(HMYAVL_TREE htree, const void * key)
{
	myavltree_t * avl_tree = (myavltree_t *)htree;
	if(NULL == avl_tree || NULL == avl_tree->compare)
		return NULL;

	return (HMYAVL_TREE_ITER)__bstree_search((bstree_node_t *)avl_tree->root, key, avl_tree->compare, avl_tree->context);
}

/**
 *
 * @brief ��ȡ��������ֵ��
 *
 */
void * MyAVLTreeGetIterData(HMYAVL_TREE_ITER it)
{
	assert(it);

	return ((myavltree_node_t *)it)->userdata;
}

/**
 *
 * @brief ��ȡ�������Ĺؼ�����
 *
 */
const void * MyAVLTreeGetIterKey(HMYAVL_TREE_ITER it)
{
	assert(it);

	return ((myavltree_node_t *)it)->base.base.key;
}

/**
 *
 * @brief avl�Ƿ���Ϲ���
 *
 */
void MyAVLTreeExamin(HMYAVL_TREE htree)
{
	myavltree_t * avl_tree = (myavltree_t *)htree;

	assert(avl_tree);

	if(avl_tree->root)
		avltree_examin(avl_tree, avl_tree->root);
}

/**
 *
 * @brief ����avl���·��
 *
 */
int MyAVLTreeMaxPath(HMYAVL_TREE htree)
{
	myavltree_t * avl_tree = (myavltree_t *)htree;

	assert(avl_tree);

	return __bstree_get_path((bstree_node_t *)avl_tree->root, 1);
}

/**
 *
 * @brief ����avl���·��
 *
 */
int MyAVLTreeMinPath(HMYAVL_TREE htree)
{
	myavltree_t * avl_tree = (myavltree_t *)htree;

	assert(avl_tree);

	return __bstree_get_path((bstree_node_t *)avl_tree->root, 0);
}

/**
 *
 * @brief ����avl���·��
 *
 */
int MyAVLTreeGetCount(HMYAVL_TREE htree)
{
	myavltree_t * avl_tree = (myavltree_t *)htree;

	assert(avl_tree);

	return __bstree_get_count((bstree_node_t *)avl_tree->root);
}



















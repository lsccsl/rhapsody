/*
* 
* myrbtree.c 2007-3-28 23:02:55 �����
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/

/*
//�����
//         �������һ����ƽ���������������ڼ������ѧ���õ���һ�����ݽṹ�����͵���;��ʵ�ֹ������顣������1972����Rudolf Bayer�����ģ�
����֮Ϊ"�Գƶ���B��"�����ִ����������� Leo J. Guibas �� Robert Sedgewick ��1978��д��һƪ�����л�õġ����Ǹ��ӵģ�
�����Ĳ����������õ���������ʱ�䣬������ʵ�����Ǹ�Ч��: ��������O(log n)ʱ���������ң������ɾ���������n ������Ԫ�ص���Ŀ��
//
//�������һ�ֺ�����˼��ƽ�������������ͳ������Ҫ����ƽ�������(��Щ�鼮��������������Adelson-Velskii��Landis�������ΪAVL-��)��
��ˣ�������ںܶ�ط�����Ӧ�á���C++ STL�У��ܶಿ��(Ŀǰ����set, multiset, map, multimap)Ӧ���˺�����ı���(SGI STL�еĺ������һЩ�仯��
��Щ�޸��ṩ�˸��õ����ܣ��Լ���set������֧��)�� 
//
//
//����������
//
//        �������һ���ض����͵Ķ������������ڼ������ѧ��������֯���ݱ������ֵĿ��һ�ֽṹ���������ݿ鶼�洢�ڽڵ��С�
��Щ�ڵ��е�ĳһ���ڵ����ǵ�����ʼλ�õĹ��ܣ��������κνڵ�Ķ��ӣ����ǳ�֮Ϊ���ڵ����������������"����"��
���������ӵ��������ڵ㡣������Щ���Ӷ��������Լ��Ķ��ӣ��Դ����ơ��������ڵ�����˰������ӵ��������κ������ڵ��·����
//
//        ���һ���ڵ�û�ж��ӣ����ǳ�֮ΪҶ�ӽڵ㣬��Ϊ��ֱ�������������ı�Ե�ϡ������Ǵ��ض��ڵ�������쵽������ĳһ���֣�����������һ������
�ں�����У�Ҷ�ӱ��ٶ�Ϊ null ��ա�
//
//        ���ں����Ҳ�Ƕ�������������ǵ���ÿһ���ڵ㶼�ıȽ�ֵ��������ڻ�����������������е����нڵ㣬����С�ڻ�����������������е����нڵ㡣
��ȷ�����������ʱ�ܹ����ٵ������в��Ҹ�����ֵ��
//
//��;�ͺô�
//
//        �������AVL��һ�����Բ���ʱ�䡢ɾ��ʱ��Ͳ���ʱ���ṩ����ÿ��ܵ�����������
�ⲻֻ��ʹ������ʱ�����е�Ӧ���缴ʱӦ��(real time application)���м�ֵ��
����ʹ���������ṩ�����������������ݽṹ����Ϊ������ļ�ֵ�����磬�ڼ��㼸����ʹ�õĺܶ����ݽṹ�����Ի��ں������
//
//        ������ں���ʽ�����Ҳ�ر����ã���������������õĳ־����ݽṹ֮һ���������������������ͼ��ϣ�
��ͻ��֮�������ܱ���Ϊ��ǰ�İ汾������O(log n)��ʱ��֮�⣬������ĳ־ð汾��ÿ�β����ɾ����ҪO(log n)�Ŀռ䡣
//
//        ������� 2-3-4����һ�ֵ�ͬ�����仰˵������ÿ�� 2-3-4 ��������������һ������Ԫ����ͬ������ĺ������
�� 2-3-4 ���ϵĲ����ɾ������Ҳ��ͬ���ں��������ɫ��ת����ת����ʹ�� 2-3-4 ����Ϊ�������������߼�����Ҫ���ߣ�
��Ҳ�Ǻܶ�����㷨�Ľ̿����ں����֮ǰ���� 2-3-4 ����ԭ�򣬾��� 2-3-4 ����ʵ���в�����ʹ�á�
//
//����
//
//        �������ÿ���ڵ㶼����ɫ���ԵĶ������������ɫ��ֵ�Ǻ�ɫ���ɫ֮һ�����˶�����������е�һ��Ҫ��
���Ƕ��κ���Ч�ĺ����������������Ҫ��:
//
//        1.�ڵ��Ǻ�ɫ���ɫ�� 
//
//        2.���Ǻ�ɫ�� 
//
//        3.����Ҷ�ӣ��ⲿ�ڵ㣩���Ǻ�ɫ�� 
//
//        4.ÿ����ɫ�ڵ�������ӽڵ㶼�Ǻ�ɫ��(��ÿ��Ҷ�ӵ���������·���ϲ��������������ĺ�ɫ�ڵ�) 
//
//        5.��ÿ��Ҷ�ӵ���������·����������ͬ��Ŀ�ĺ�ɫ�ڵ㡣 
//
//        ��ЩԼ��ǿ���˺�����Ĺؼ�����: �Ӹ���Ҷ�ӵ���Ŀ���·����������̵Ŀ���·������������������������������ƽ��ġ�
��Ϊ����������롢ɾ���Ͳ���ĳ��ֵ��Ҫ�������ĸ߶ȳɱ���������ʱ�䣬����ڸ߶��ϵ������������������������¶��Ǹ�Ч�ģ�
����ͬ����ͨ�Ķ����������
//
//        Ҫ֪��Ϊʲô��Щ����ȷ������������ע�⵽����4������·�����������������ĺ�ɫ�ڵ���㹻�ˡ�
��̵Ŀ���·�����Ǻ�ɫ�ڵ㣬��Ŀ���·���н���ĺ�ɫ�ͺ�ɫ�ڵ㡣��Ϊ��������5�������·��������ͬ��Ŀ�ĺ�ɫ�ڵ㣬
��ͱ�����û��·���ܶ����κ�����·������������
//
//        �ںܶ������ݽṹ�ı�ʾ�У�һ���ڵ��п���ֻ��һ�����ӣ���Ҷ�ӽڵ�������ݡ������ַ�����ʾ������ǿ��ܵģ�
�������ı�һЩ���Բ�ʹ�㷨���ӡ�Ϊ�ˣ�����������ʹ�� "nil Ҷ��" ��"��(null)Ҷ��"������ͼ��ʾ�������������ݶ�ֻ�䵱���ڴ˽�����ָʾ��
��Щ�ڵ��ڻ�ͼ�о�����ʡ�ԣ���������Щ������ͬ����ԭ����ì�ܣ���ʵ���ϲ�������������йصĽ��������нڵ㶼���������ӣ�
�������е�һ�������������ǿ�Ҷ�ӡ�
//
//����
//
//        �ں������ֻ����������Ҫ�����ڶ���������Ĳ��������޸ģ���Ϊ��Ҳ�����������
���ǣ��ڲ����ɾ��֮�󣬺�����Կ��ܱ��Υ�档
�ָ����������Ҫ����(O(log n))����ɫ���(����ʵ�����Ƿǳ����ٵ�)���Ҳ�������������ת(���ڲ���������)��
����������ɾ������Ϊ O(log n) �Σ������������˷ǳ����ӵĲ����� 
*/

#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include "myutility.h"
#include "myrbtree.h"


typedef enum __rbtree_colour
{
	rbtree_colour_black,
	rbtree_colour_red,
}rbtree_colour;

typedef struct __myrbtree_node_t
{
	struct __myrbtree_node_t * left;
	struct __myrbtree_node_t * right;
	struct __myrbtree_node_t * parent;

	rbtree_colour colour;

	void * key;
	void * data;
}myrbtree_node_t;

typedef struct __myrbtree_t
{
	myrbtree_node_t * root;

	//�ڴ��
	HMYMEMPOOL hm;

	//�Ƚ������
	myrbtree_compare compare;
}myrbtree_t;

/*
*
*1 ��ʾ key1 �� key2 ��
*0 ��ʾ key1 �� key2 С 
*
*/
static __INLINE__ int rbtree_inter_compare(myrbtree_t * rbtree, const void * key1, const void * key2)
{
	assert(rbtree && rbtree->compare);

	return (*rbtree->compare)(key1, key2);
}

static __INLINE__ myrbtree_node_t * rbtree_inter_create_node(myrbtree_t * rbtree, const void * key, const void * data)
{
	myrbtree_node_t * node_new = NULL;

	assert(rbtree);

	node_new = (myrbtree_node_t *)MyMemPoolMalloc(rbtree->hm, sizeof(*node_new));

	if(NULL == node_new)
		return NULL;

	memset(node_new, 0, sizeof(*node_new));
	node_new->key = (void *)key;
	node_new->data = (void *)data;

	return node_new;
}

static __INLINE__ void rbtree_inter_destroy_node(myrbtree_t * rbtree, myrbtree_node_t * node)
{
	assert(rbtree && node);

	MyMemPoolFree(rbtree->hm, node);
}

/*
*
*����
*
*   A                node
*    \              /
*     \    ---->   /
*      node       A
*/
static __INLINE__ void rbtree_inter_rotate_left(/*myrbtree_t * rbtree*/myrbtree_node_t ** root, myrbtree_node_t * node)
{
	myrbtree_node_t * A_node = NULL;

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

/*
*
*����
*
*     A        node
*    /          \
*   /    --->    \
*  node           A
*/
static __INLINE__ void rbtree_inter_rotate_right(/*myrbtree_t * rbtree*/myrbtree_node_t ** root, myrbtree_node_t * node)
{
	myrbtree_node_t * A_node = NULL;

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

static __INLINE__ myrbtree_node_t * rbtree_inter_search(myrbtree_t * rbtree, myrbtree_node_t * root, const void * key)
{
	myrbtree_node_t * y = NULL;/* ��¼���һ��������key�Ľڵ� */
	myrbtree_node_t * x = root;

	assert(rbtree);

	while(x)
	{
		if(!rbtree_inter_compare(rbtree, x->key, key))
			y = x, x = x->right;
		else
			x = x->left;
	}

	return (NULL == y || rbtree_inter_compare(rbtree, key, y->key))?NULL:y;
}

static __INLINE__ myrbtree_node_t * rbtree_inter_searchex(myrbtree_t * rbtree, myrbtree_node_t * root, const void * key, myrbtree_node_t ** parent)
{
	myrbtree_node_t * y = NULL;/* ��¼���һ��������key�Ľڵ� */
	myrbtree_node_t * x = root;

	assert(rbtree && parent /*&& rbtree->root == root*/);

	*parent = root;

	while(x)
	{
		*parent = x;

		if(!rbtree_inter_compare(rbtree, x->key, key))
			y = x, x = x->right;
		else
			x = x->left;
	}

	return (NULL == y || rbtree_inter_compare(rbtree, key, y->key))?NULL:y;
}

static __INLINE__ int rbtree_inter_ismynode(const myrbtree_node_t * root, const myrbtree_node_t * node)
{
	int ret = 0;

	if(NULL == root)
		return 0;

	if(node == root)
		return 1;

	if(root->left)
		ret = rbtree_inter_ismynode(root->left, node);
	if(ret)
		return ret;

	if(root->right)
		return rbtree_inter_ismynode(root->right, node);

	return 0;
}

/*
*
*��ת�������ʹ֮���Ϻ�����Ĺ���
*rbtree:��Ҫ��ת�ĺ����
*node:�¼���Ľڵ�
*
*/
static __INLINE__ void rbtree_inter_rebalance(/*myrbtree_t * rbtree*/myrbtree_node_t ** root, myrbtree_node_t * node)
{
	assert(root && node && node->parent);

	//�¼���ڵ��Ϊ��
	node->colour = rbtree_colour_red;

	//������ڵ�Ϊ���ڵ�,���ݺ�����Ķ�����ڵ��Ϊ��
	if(node->parent == *root)
		return;

	//��Ϊ����㣬�游�ڵ�ش���
	assert(node->parent->parent);

	//������ڵ㲻Ϊ��
	while(node != *root && rbtree_colour_red == node->parent->colour)
	{
		//������ڵ����游�ڵ������
		if(node->parent == node->parent->parent->left)
		{
			//��������ڵ���ڣ���Ϊ��
			if(node->parent->parent->right && rbtree_colour_red == node->parent->parent->right->colour)
			{
				//�Ѹ��ڵ��벮���ڵ�Ϳ�ɺ�ɫ
				node->parent->colour = rbtree_colour_black;
				node->parent->parent->right->colour = rbtree_colour_black;

				//���游���Ϳ�ɺ�ɫ
				node->parent->parent->colour = rbtree_colour_red;

				//ָ��������
				node = node->parent->parent;
			}
			else
			{
				//�����������
				if(node == node->parent->left)
				{
					//nodeΪ��
					node->colour = rbtree_colour_red;

					//���ڵ�Ϊ��
					node->parent->colour = rbtree_colour_black;

					//�游�ڵ�Ϊ��
					node->parent->parent->colour = rbtree_colour_red;

					//���ڵ�Ϊ������ת
					rbtree_inter_rotate_right(root, node->parent);
				}
				else
				{
					myrbtree_node_t * temp = node->parent;

					//nodeΪ��
					node->colour = rbtree_colour_black;

					//���ڵ�Ϊ��
					node->parent->colour = rbtree_colour_red;

					//�游�ڵ�Ϊ��
					node->parent->parent->colour = rbtree_colour_red;

					//nodeΪ������ת
					rbtree_inter_rotate_left(root, node);

					//���ڵ�Ϊ������ת����ת
					rbtree_inter_rotate_right(root, node);

					node = temp;
				}
			}
		}
		//������ڵ����游�ڵ���Һ���
		else
		{
			//��������ڵ���ڣ���Ϊ��
			if(node->parent->parent->left && rbtree_colour_red == node->parent->parent->left->colour)
			{
				//�Ѹ��ڵ��벮���ڵ�Ϳ�ɺ�ɫ
				node->parent->colour = rbtree_colour_black;
				node->parent->parent->left->colour = rbtree_colour_black;

				//���游���Ϳ�ɺ�ɫ
				node->parent->parent->colour = rbtree_colour_red;

				//ָ��������
				node = node->parent->parent;
			}
			else
			{
				//�����������
				if(node == node->parent->right)
				{
					//nodeΪ��
					node->colour = rbtree_colour_red;

					//���ڵ�Ϊ��
					node->parent->colour = rbtree_colour_black;

					//�游�ڵ�Ϊ��
					node->parent->parent->colour = rbtree_colour_red;

					//���ڵ�Ϊ������
					rbtree_inter_rotate_left(root, node->parent);
				}
				else
				{
					myrbtree_node_t * temp = node->parent;

					//nodeΪ��
					node->colour = rbtree_colour_black;

					//���ڵ�Ϊ��
					node->parent->colour = rbtree_colour_red;

					//�游�ڵ�Ϊ��
					node->parent->parent->colour = rbtree_colour_red;

					//nodeΪ������ת
					rbtree_inter_rotate_right(root, node);

					//���ڵ�Ϊ������
					rbtree_inter_rotate_left(root, node);

					node = temp;
				}
			}
		}
	}

	(*root)->colour = rbtree_colour_black;
}

/*
*
*���һ�ڵ�
*rbtree:��
*parent:�½ڵ�ĸ��ڵ�
*key:�½ڵ�Ĺؼ���
*
*/
static __INLINE__ void rbtree_inter_insert(myrbtree_t * rbtree, myrbtree_node_t ** root, myrbtree_node_t * node_new, myrbtree_node_t * parent)
{
	assert(rbtree && node_new && root);

	//������ڵ�Ϊ�գ�������ӵ��Ǹ��ڵ�
	if(NULL == parent)
	{
		//���ڵ���к�ɫ
		node_new->colour = rbtree_colour_black;
		*root = node_new;

		assert(NULL == *root || NULL == (*root)->parent);

		return;
	}

	node_new->parent = parent;

	//�ȽϽڵ��ֵ�븸�ڵ��ֵ��С
	if(rbtree_inter_compare(rbtree, node_new->key,parent->key))
		parent->right = node_new;
	else
		parent->left = node_new;

	//��ת����ʹ֮���Ϻ�����Ĺ���
	rbtree_inter_rebalance(root, node_new);
}

/*
*
*��rb����ɾ��һ���ڵ�
*
*ɾ�����ڵ� z , �������y��ȫȡ��z(λ��,��ɫ,�û�����), ɾ��z�ݱ��ɾ��y
*
* ͼ1
*      a                         a
*     / \                       / \
*    ?   z                     ?   y
*       / \       ---->           / \ 
*      ?   b                     ?   b
*         / \                       / \
*        y   ?            del ---> z   ?
*         \                         \
*          ?1                        ?1
*
*
*���z�Ǻ�ɫ ��?1��Ӧ���������ڵ�ֱ��Ų��z��Ӧ��λ��,�������ɾ������������֤�˺������ƽ����
*���z�Ǻ�ɫ ����?1��Ӧ���������ڵ�xΪ��ɫ�����?1Ų��z��λ�ã�����x�ĳɺ�ɫ���������ɾ������������֤�˺������ƽ����
*���z�Ǻ�ɫ ����?1��Ӧ���������ڵ�xҲΪ��ɫ��
*ͼ2
*
*     ?
*    / \
*   ?   x_parent
*      / \
*     x   ?2 = w
*    / \
*   ?   ?
*ͼ2������Ҫ���ٵ����
*x�����֧������ȥ����һ����ɫ�Ľ�㣬��x���Ǻ�ɫ������ ?2 �Ǹ���֧��ȣ�"����һ����ɫ�Ľ��"
*
*ͼ3
*       x_parent              x_parent->parent
*       /   \                   /           \
*      x     w     -->        x_parent       w_new
*     / \   / \               / \
*    ?   ? ?b  ?b            ?   ?
*����?b ?b��Ϊ"��" wҲΪ�� ��� wͿ�ɺ�  ��ʱx��֧��w��֧"�ڲ���"��ͬ , 
*(x_parent�п���Ϊ�죬���ⲻӰ����,��Ϊ����һ��ѭ����,x = x_parent,ѭ�����˳�,x��x_parent����Ϳ�ɺ�,��Υ�����������)
*��x = x_parent�� w = w_new�����֧"����һ��" �������ϵ��� (x = x_parent x_parent = x->parent) �������ڵ�Ϊ��,�������ɾ������������֤�˺������ƽ��
*
*���wΪ��,ͨ����ת�������ݻ���ͼ3�����
*�ݻ���������:
*��wͿ�ɺ�ɫ,x_parentͿ�ɺ�ɫ(���ݺ��������x_parent��Ϊ��ɫ, w�������ӽڵ��Ϊ��ɫ)
*��x_parentΪ������
*                   w 
*                  / \
*          x_parent    ?b 
*          /    \
*         x     ?b = w_new(��Ϊ��)
*        / \
*       ?   ?
*  x ��֧�� ?b��֧����һ��
*
* ��������wΪ�컹��Ϊ��,���ն������ݻ���wΪ�ڵ����
*
*w���ӽڵ����к�ɫ�ڵ�����
*����������Կ��ǰ�w�ĺ�ɫ�ڵ�ת�Ƶ�x��֧����Ϊx��֧�ĸ��ڵ㣬��Ϳ�ɺ�ɫ���������ɾ��������֤�˺������ƽ��
*����任���� ���a��Ϊ�죬���Ծ�����ת�任Ϊ��
*
*     x_parent?
*      /      \
*     x        w��      
*    / \      / \
*   ?   ?    ��b ��a
*
*              w?
*             /  \
*      x_parent�� ��a
*     /  \
*    x   ��b
*�������ɾ��������֤�˺������ƽ����
*
*/
static __INLINE__ myrbtree_node_t * rbtree_inter_rebalance_for_erase(/*myrbtree_t * rbtree*/myrbtree_node_t ** root, myrbtree_node_t * node)
{
	myrbtree_node_t * x = NULL;
	myrbtree_node_t * x_parent = NULL;
	myrbtree_node_t * y = node;

	assert(node && root);
	
	if(NULL == node->left)
		x = node->right;
	else if(NULL == node->right)
		x = node->left;
	else
	{
		//���node����Ҷ�ӽ�㣬��Ѱ������������"��"�ߵ��Ǹ��ڵ����node
		y = node->right;
		while(y->left)
			y = y->left;

		x = y->right;
	}

	//Ҫɾ���Ľڵ����Һ��Ӷ���Ϊ��,y��node ����
	if(y != node)
	{
		//��ɫ����
		rbtree_colour colour = y->colour;
		y->colour = node->colour;
		node->colour = colour;

		if(node->parent)
		{
			if(node == node->parent->left)
				node->parent->left = y;
			else
				node->parent->right = y;
		}
		else if(node == *root)
			*root = y;
		else
			assert(0);//����ߵ���һ�� bug here

		if(node->left)
			node->left->parent = y;

		//���y��node���Һ���
		if(y == node->right)
		{
			x_parent = y;
		}
		else
		{
			if(node->right)
				node->right->parent = y;

			assert(y->parent);
			x_parent = y->parent;

			if(y == x_parent->left)
				x_parent->left = x;
			else
				assert(0);//x_parent->right = x; //y��������x_parent���Һ��� ����ߵ���һ��,bug

			y->right = node->right;
		}

		y->parent = node->parent;
		y->left = node->left;

		y = node;
	}
	else//y����Ҫɾ���Ľڵ㣬�������滻����y����һ���ӽڵ�Ϊ��
	{
		//���Ҫɾ�����Ǹ��ڵ�
		if(y == *root)
		{
			*root = x;
			if(*root)
			{
				(*root)->colour = rbtree_colour_black;
				(*root)->parent = NULL;
			}

			assert(NULL == *root || NULL == (*root)->parent);

			return y;
		}
		else
		{
			assert(y->parent);

			x_parent = y->parent;
			if(y == x_parent->left)
				x_parent->left = x;
			else
				x_parent->right = x;
		}
	}

	assert(x_parent);	

	if(x)
		x->parent = x_parent;

	//ת����
	//*      a                         a
	//*     / \                       / \
	//*    ?   z                     ?   y
	//*       / \       ---->           / \ 
	//*      ?   b                     ?   b
	//*         / \                       / \
	//*        y   ?            del ---> z   ?
	//*         \                         \
	//*          ?1                        ?1

	//���Ҫɾ���Ľڵ�Ϊ��ɫ����������
	if(y->colour == rbtree_colour_red)
	{
		assert(NULL == *root || NULL == (*root)->parent);
		return y;
	}

	while((x != *root) && (x == NULL || rbtree_colour_black == x->colour))
	{
		assert(x_parent);

		if(x == x_parent->left)
		{
			myrbtree_node_t * w = x_parent->right;
			assert(w);//x��֧Ϊ0/1���ڽ��,w��֧������һ���ڽ��,����w��֧������Ϊnull,

			//���w�Ǻ죬��Ҫ����ת��,
			if(rbtree_colour_red == w->colour)
			{
				//x_parent�ض��Ǻ�ɫ��,��w�������ӽڵ�ز��գ���һ��Ϊ��ɫ
				assert(rbtree_colour_black == x_parent->colour);
				assert(w->left && rbtree_colour_black == w->left->colour);
				assert(w->right && rbtree_colour_black == w->right->colour);

				w->colour = rbtree_colour_black;
				x_parent->colour = rbtree_colour_red;

				//��ת
				rbtree_inter_rotate_left(root, w);

				w = x_parent->right;
			}

			//x��֧��w��֧"����һ��",��x,w��Ϊ��
			assert(w);

			//���w�������ӽڵ��Ϊ��
			if( (NULL == w->left || rbtree_colour_black == w->left->colour) &&
				(NULL == w->right || rbtree_colour_black == w->right->colour) )
			{
				//x_parent�п���Ϊ�죬���ⲻӰ����,��Ϊ����һ��ѭ����,x = x_parent,ѭ�����˳�,x��x_parent����Ϳ�ɺ�,��Υ�����������
				w->colour = rbtree_colour_red;
				x = x_parent;
				x_parent = x->parent;
				continue;
			}

			//���������һ���ӽڵ�Ϊ��, ͨ����ת�任,��Ϊ����Ǹ��ڵ�ʼ��Ϊ�ҽڵ�
			if(NULL == w->right || rbtree_colour_black == w->right->colour)
			{
				assert(w->left && rbtree_colour_red == w->left->colour);//�����߼��ó�,��Ϊ����,�ұ�Ϊ��ɫ
				
				w->left->colour = rbtree_colour_black;
				w->colour = rbtree_colour_red;
				rbtree_inter_rotate_right(root, w->left);
				w = x_parent->right;
			}

			//�����ݱ���������
			//*     x_parent?
			//*      /      \
			//*     x        w��      
			//*    / \      / \
			//*   ?   ?    ?b ��a
			assert(w->right && rbtree_colour_red == w->right->colour);//�����߼�,��������س���

			//��ת,�����ƽ��Ĺ���,�任��������ͼ
			//*     x_parent?
			//*      /      \
			//*     x        w��      
			//*    / \      / \
			//*   ?   ?    ��b ��a
			//*
			//*              w?
			//*             /  \
			//*      x_parent�� ��a
			//*     /  \
			//*    x   ��b
			w->colour = x_parent->colour;
			x_parent->colour = rbtree_colour_black;
			w->right->colour = rbtree_colour_black;
			rbtree_inter_rotate_left(root, w);
			break;
		}
		else//x �� x_parent���Һ���,������x == x_parent->left��ͬ,�������෴
		{
			myrbtree_node_t * w = x_parent->left;
			assert(w);//x��֧Ϊ0/1���ڽ��,w��֧������һ���ڽ��,����w��֧������Ϊnull,

			//���w�Ǻ죬��Ҫ����ת��,
			if(rbtree_colour_red == w->colour)
			{
				//x_parent�ض��Ǻ�ɫ��,��w�������ӽڵ�ز��գ���һ��Ϊ��ɫ
				assert(rbtree_colour_black == x_parent->colour);
				assert(w->left && rbtree_colour_black == w->left->colour);
				assert(w->right && rbtree_colour_black == w->right->colour);

				w->colour = rbtree_colour_black;
				x_parent->colour = rbtree_colour_red;

				//��ת
				rbtree_inter_rotate_right(root, w);

				w = x_parent->left;
			}

			//x��֧��w��֧"����һ��",��x,w��Ϊ��
			assert(w);

			//���w�������ӽڵ��Ϊ��
			if( (NULL == w->left || rbtree_colour_black == w->left->colour) &&
				(NULL == w->right || rbtree_colour_black == w->right->colour) )
			{
				//x_parent�п���Ϊ�죬���ⲻӰ����,��Ϊ����һ��ѭ����,x = x_parent,ѭ�����˳�,x��x_parent����Ϳ�ɺ�,��Υ�����������
				w->colour = rbtree_colour_red;
				x = x_parent;
				x_parent = x->parent;
				continue;
			}

			//���������һ���ӽڵ�Ϊ��, ͨ����ת�任,��Ϊ����Ǹ��ڵ�ʼ��Ϊ�ҽڵ�
			if(NULL == w->left || rbtree_colour_black == w->left->colour)
			{
				assert(w->right && rbtree_colour_red == w->right->colour);//�����߼��ó�,��Ϊ����,�ұ�Ϊ��ɫ
				
				w->right->colour = rbtree_colour_black;
				w->colour = rbtree_colour_red;
				rbtree_inter_rotate_left(root, w->right);
				w = x_parent->left;
			}

			assert(w->left && rbtree_colour_red == w->left->colour);//�����߼�,��������س���

			w->colour = x_parent->colour;
			x_parent->colour = rbtree_colour_black;
			w->left->colour = rbtree_colour_black;
			rbtree_inter_rotate_right(root, w);
			break;
		}
	}

	if(x) 
		x->colour = rbtree_colour_black;

	//assert(!rbtree_inter_ismynode(rbtree->root, y));
	assert(NULL == (*root) || NULL == (*root)->parent);

	return y;
}

static __INLINE__ void rbtree_inter_del(myrbtree_t * rbtree, myrbtree_node_t ** root, myrbtree_node_t * node, void ** key, void ** data)
{
	assert(rbtree && node && root);

	//��תƽ������
	node = rbtree_inter_rebalance_for_erase(root, node);

	if(NULL == node)
		return;

	if(data)
		*data = node->data;

	if(key)
		*key = node->key;

	//����node
	rbtree_inter_destroy_node(rbtree, node);
}

static __INLINE__ int rbtree_inter_countlayer(myrbtree_node_t * root, int bmax)
{
	int left = 0;
	int right = 0;

	if(NULL == root)
		return 0;

	left = rbtree_inter_countlayer(root->left, bmax);
	right = rbtree_inter_countlayer(root->right, bmax);

	if(left > right && bmax)
		return left + 1;
	else
		return right + 1;
}

static __INLINE__ myrbtree_node_t * rbtree_inter_begin(myrbtree_t * rbtree)
{
	myrbtree_node_t * node;

	assert(rbtree);

	node = rbtree->root;
	if(NULL == node)
		return NULL;

	while(node->left)
		node = node->left;

	return node;
}

static __INLINE__ myrbtree_node_t * rbtree_inter_end(myrbtree_t * rbtree)
{
	myrbtree_node_t * node;

	assert(rbtree);

	node = rbtree->root;
	if(NULL == node)
		return NULL;

	while(node->right)
		node = node->right;

	return node;
}

static __INLINE__ void rbtree_inter_erase(myrbtree_t * rbtree, myrbtree_node_t * node)
{
	assert(node);

	while(node)
	{
		myrbtree_node_t * y = node->left;

		if(node->right)
			rbtree_inter_erase(rbtree, node->right);

		rbtree_inter_destroy_node(rbtree, node);

		node = y;
	}
}

static __INLINE__ int rbtree_inter_realcount(myrbtree_node_t * root)
{
	int left = 0;
	int right = 0;

	if(NULL == root)
		return 0;

	if(root->left)
		left = rbtree_inter_realcount(root->left);

	if(root->right)
		right = rbtree_inter_realcount(root->right);

	return left + right + 1;
}

static __INLINE__ int rbtree_inter_examin(myrbtree_t * rbtree, myrbtree_node_t * node)
{
	int left = 0;
	int right = 0;

	if(NULL == node)
		return 0;

	if(node->left)
	{
		assert(rbtree_inter_compare(rbtree, node->key, node->left->key));
		assert(node->left->parent == node);
		left = rbtree_inter_examin(rbtree, node->left);
	}

	if(node->right)
	{
		assert(rbtree_inter_compare(rbtree, node->right->key, node->key));
		assert(node->right->parent == node);
		right = rbtree_inter_examin(rbtree, node->right);
	}

	assert(left == right);

	if(rbtree_colour_black == node->colour)
		return left + 1;
	else
		return left;
}


/*
*
*����rb��
*
*/
HMYRBTREE MyRBTreeConstruct(HMYMEMPOOL hm, myrbtree_compare compare)
{
	myrbtree_t * rbtree = NULL;

	rbtree = (myrbtree_t *)MyMemPoolMalloc(hm, sizeof(*rbtree));

	if(NULL == rbtree)
		return NULL;

	rbtree->compare = compare;
	rbtree->hm = hm;
	rbtree->root = NULL;

	return (HMYRBTREE)rbtree;
}

/*
*
*����rb��
*
*/
void MyRBTreeDestruct(HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return;

	//������,�ͷ�ÿ���ڵ�
	if(rbtree->root)
		rbtree_inter_erase(rbtree, rbtree->root);

	MyMemPoolFree(rbtree->hm, rbtree);
}

/*
*
*ɾ�����нڵ�
*
*/
void MyRBTreeClear(HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return;

	if(NULL == rbtree->root)
		return;

	//������,�ͷ�ÿ���ڵ�
	rbtree_inter_erase(rbtree, rbtree->root);
	rbtree->root = NULL;
}

/*
*
*��rb���в���һ���ڵ�
*
*/
HMYRBTREE_ITER MyRBTreeInsertEqual(HMYRBTREE htree, const void * key, const void * userdata)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;
	myrbtree_node_t * node_new = NULL;
	myrbtree_node_t * parent = NULL;

	if(NULL == rbtree)
        return NULL;

	rbtree_inter_searchex(rbtree, rbtree->root, key, &parent);

	node_new = rbtree_inter_create_node(rbtree, key, userdata);
	if(NULL == node_new)
		return NULL;

	rbtree_inter_insert(rbtree, &rbtree->root, node_new, parent);

	return (HMYRBTREE_ITER)node_new;
}

/*
*
*��rb���в���һ���ڵ�
*
*/
HMYRBTREE_ITER MyRBTreeInsertUnique(HMYRBTREE htree, const void * key, const void * userdata)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	myrbtree_node_t * parent = NULL;
	myrbtree_node_t * node_new = NULL;

	if(NULL == rbtree)
        return NULL;
	
	node_new = rbtree_inter_searchex(rbtree, rbtree->root, key, &parent);
	if(node_new != NULL)
		return (HMYRBTREE_ITER)node_new;

	node_new = rbtree_inter_create_node(rbtree, key, userdata);
	if(NULL == node_new)
		return NULL;

	rbtree_inter_insert(rbtree, &rbtree->root, node_new, parent);

	return (HMYRBTREE_ITER)node_new;
}

/*
*
*��rb����ɾ��һ���ڵ� iterʧЧ
*
*/
void MyRBTreeDelIter(HMYRBTREE htree, HMYRBTREE_ITER iter, void ** key, void ** data)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;
	myrbtree_node_t * node = (myrbtree_node_t *)iter;

	if(NULL == rbtree || NULL == node)
		return;

	assert(rbtree_inter_search(rbtree, rbtree->root, node->key));

	rbtree_inter_del(rbtree, &(rbtree->root), node, key, data);
}

/*
*
*���ݼ�ֵɾ��һ���ڵ�
*�ɹ�ɾ������0, ���򷵻�-1
*
*/
int MyRBTreeDelKey(HMYRBTREE htree, const void * key, void ** key_ret, void ** data_ret)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;
	myrbtree_node_t * node = NULL;

	if(NULL == rbtree)
		return -1;

	node = rbtree_inter_search(rbtree, rbtree->root, key);

	if(NULL == node)
		return -1;

	rbtree_inter_del(rbtree, &(rbtree->root), node, key_ret, data_ret);

	return 0;
}

/*
*
*��ȡ�ڵ���û�����
*
*/
void * MyRBTreeGetIterData(const HMYRBTREE_ITER iter)
{
	myrbtree_node_t * node = (myrbtree_node_t *)iter;

	if(NULL == node)
		return NULL;

	return node->data;
}

/*
*
*��ȡ�ڵ�ļ�
*
*/
const void * MyRBTreeGetIterKey(const HMYRBTREE_ITER iter)
{
	myrbtree_node_t * node = (myrbtree_node_t *)iter;

	if(NULL == node)
		return NULL;

	return node->key;
}

/*
*
*���ҽڵ�
*
*/
HMYRBTREE_ITER MyRBTreeSearch(const HMYRBTREE htree, const void * key)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;
	myrbtree_node_t * node = NULL;

	if(NULL == rbtree || NULL == rbtree->compare)
		return NULL;

	node = rbtree_inter_search(rbtree, rbtree->root, key);

	return (HMYRBTREE_ITER)node;
}

/*
*
*����������
*
*/
int MyRBTreeLayer(const HMYRBTREE htree, int bmax)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return 0;

	return rbtree_inter_countlayer(rbtree->root, bmax);
}

/*
*
*"��ȡ��һ���ڵ�"
*
*/
HMYRBTREE_ITER MyRBTreeBegin(const HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return 0;

	return (HMYRBTREE_ITER)rbtree_inter_begin(rbtree);
}

/*
*
*"��ȡ���һ���ڵ�"
*
*/
HMYRBTREE_ITER MyRBTreeEnd(const HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return 0;

	return (HMYRBTREE_ITER)rbtree_inter_end(rbtree);
}

/*
*
*��ȡ��һ���ڵ�
*
*/
HMYRBTREE_ITER MyRBTreeGetNext(const HMYRBTREE_ITER it)
{
	myrbtree_node_t * node = (myrbtree_node_t *)it;

	if(NULL == node)
		return NULL;

	if(NULL == node->right)
	{
		while(node->parent && node == node->parent->right)
			node = node->parent;

		if(node->parent && node == node->parent->left)
			return (HMYRBTREE_ITER)node->parent;
		else
			return NULL;
	}
	else
	{
		node = node->right;
		while(node->left)
			node = node->left;

		return (HMYRBTREE_ITER)node;
	}
}

/*
*
*��ȡ��һ���ڵ�
*
*/
HMYRBTREE_ITER MyRBTreeGetPrev(const HMYRBTREE_ITER it)
{
	myrbtree_node_t * node = (myrbtree_node_t *)it;

	if(node->left)
	{
		node = node->left;
		while(node->right)
			node = node->right;
		return (HMYRBTREE_ITER)node;
	}
	else
	{
		while(node->parent && node == node->parent->left)
			node = node->parent;

		if(node->parent && node == node->parent->right)
			return (HMYRBTREE_ITER)node->parent;
		else
			return NULL;
	}		
}

/*
*
*��������Ƿ�Ϸ�
*
*/
int MyRBTreeExamin(const HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return 0;

	return rbtree_inter_examin(rbtree, rbtree->root);
}

/*
*
*��ȡ����
*
*/
int MyRBTreeGetRealCount(const HMYRBTREE htree)
{
	myrbtree_t * rbtree = (myrbtree_t *)htree;

	if(NULL == rbtree)
		return 0;

	return rbtree_inter_realcount(rbtree->root);
}

#include "mymap.c"

#ifdef WIN32
	#include "MyStringSetEx.c"
#endif





























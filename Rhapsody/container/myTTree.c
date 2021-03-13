/**
 *
 * @file myTTree.c T��
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief
 * T ����avl������չ(����˵��avl����b���ۺϵĲ���)
 *  ��һ��node����һ���ؼ�������.
 *  ������Ĺؼ����Ѿ����ź���
 *  oracle ten times, fastdb, mysql cluster��,ʹ����T����Ϊ����.
 *
 * ��ӹؼ�����T��
 * �Ӹ��ڵ㿪ʼ��Ѱ,������ҵ�bound����,�����,�����������,�����С����һ��Ԫ��A.
 * �н���greate lower bound(��������ҵ��Ǹ��ڵ�),����������A,�����
 * �������,��Ӧ�����½ڵ㣬���Ҹ���ʵ�������ת����֤avl����ƽ��
 *
 * ���û���ҵ����ʵĽڵ㣬��������(��)��ӣ���Ӳ����򴴽��½ڵ㣬������ʵ�������ת��
 *
 * ɾ���ؼ���
 *  �Ӹ��ڵ㿪ʼ��Ѱ,����ҵ�
 *  a �����ǰ�ڵ�ĸ�����ɾ��֮����Ȼ�������ޣ��������
 *  b �����ǰ�ڵ����С��С�ޣ��������ڲ��ڵ㣬���greate lower bound�ڵ��н�
 *    ���greate lower bound�ڵ���leaf�ڵ� ͬc
 *    ���greate lower bound�ڵ���half leaf�ڵ� ͬd
 *
 *  c �����half leaf�ڵ㣬�ж��Ƿ���Ժϲ���Ȼ�������Ҫ��ת��
 *  d �����leaf�ڵ�,�ж��Ƿ�Ҫɾ����Ȼ�������Ҫ��ת��
 *
 * ������������:
 *   fastDB��mysqlʹ�õĶ��Ƕ��ֲ��Ҷ�λ����
 */
#include "myTTree.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "__avl_tree.h"
#include "myvector.h"
#include "__vector_inter.h"
#include "__bstree.h"


typedef struct __myttree_key_t_
{
	/*
	* ����,���������Ϣ
	*/
	HMYVECTOR hv_index;
}myttree_key_t;

typedef struct __myttree_node_t_
{
	/*
	* avlƽ�����ڵ�
	*/
	__avltree_node_t avl_node;

	myttree_key_t key;
}myttree_node_t;

typedef struct __myttree_t_
{
	HMYMEMPOOL hm;

	ALG_COMPARE compare;
	void * context;

	/*
	* T���ĸ��ڵ�
	*/
	myttree_node_t * root;

	/*
	* һ���ڵ�Ĺؼ��ָ�������
	*/
	size_t underflow;

	/*
	* һ���ڵ�Ĺؼ���������
	*/
	size_t overflow;
}myttree_t;


/**
 * ������������
 */
#define ttree_sort_index_array(ttree, hv_index) do{\
		MyAlgQuickSort(__vector_inter_get_array((hv_index)),\
			__vector_inter_get_count((hv_index)), \
			__vector_inter_get_array_step_size((hv_index)),\
			ttree_compare, NULL, NULL, NULL, ttree, NULL, 0);\
	}while(0)


/**
 * T���Ƚ�
 */
static __INLINE__ int ttree_compare(const void * data1, const void * data2, const void * context)
{
	HMYVECTOR_ITER it1 = *((HMYVECTOR_ITER *)data1);
	HMYVECTOR_ITER it2 = *((HMYVECTOR_ITER *)data2);
	myttree_t * ttree = (myttree_t *)context;

	assert(ttree && it1 && it2 && ttree->compare);

	return ttree->compare(__vector_inter_get_iter_data(it1), __vector_inter_get_iter_data(it2), ttree->context);
}

/**
 * �����������
 */
static __INLINE__ int ttree_search(myttree_t * ttree, HMYVECTOR hv_index, const void * index_info, size_t * index)
{
	assert(ttree && hv_index && index_info);

	{
		__vector_inter_get_temp_it(it, 0, (void *)index_info);

		return MyBinarySearch1(__vector_inter_get_array(hv_index),
			__vector_inter_get_count(hv_index),
			__vector_inter_get_array_step_size(hv_index),
			&it, ttree_compare, index, ttree);
	}
}

/**
 * ����һ��T���ڵ�
 */
static __INLINE__ myttree_node_t * ttree_create_node(myttree_t * ttree, myttree_node_t * parent)
{
	myttree_node_t * node = (myttree_node_t *)MyMemPoolMalloc((assert(ttree), ttree->hm), sizeof(*node));

	if(NULL == node)
		return NULL;

	memset(node, 0, sizeof(*node));

	node->key.hv_index = __vector_inter_create(ttree->hm, ttree->overflow + 1, NULL, NULL);
	node->avl_node.height = 1;
	node->avl_node.base.parent = (bstree_node_t *)parent;

	return node;
}


/**
 * ����һ��T���ڵ�
 */
static __INLINE__ void ttree_destroy_node(myttree_t * ttree, myttree_node_t * node)
{
	assert(ttree && node && node->key.hv_index);

	__vector_inter_destroy(node->key.hv_index);

	MyMemPoolFree(ttree->hm, node);
}

/**
 * @brief ������__bstree_erase����������ͷ�ÿ���ڵ�
 * @param context:�û�������������
 * @param root:���ڵ�
 */
static void __ttree_destroy_node_cb(void * context, bstree_node_t * root)
{
	ttree_destroy_node((myttree_t *)context, (myttree_node_t *)root);
}


/**
 * @brief ���Һ��ʵĽڵ�λ��
 */
static __INLINE__ myttree_node_t * ttree_search_bound(myttree_t * ttree, myttree_node_t * node, const void * key, myttree_node_t ** parent)
{
	HMYVECTOR_ITER it_first = NULL;
	HMYVECTOR_ITER it_last = NULL;

	assert(ttree && ttree->compare && parent);

	while(node)
	{
		HMYVECTOR hv = node->key.hv_index;

		assert(__vector_inter_get_count(hv));

		*parent = node;

		it_first = __vector_get_head(hv);
		it_last = __vector_get_tail(hv);

		if(ttree->compare(key, __vector_inter_get_iter_data(it_first), ttree->context) < 0)
		{
			node = (myttree_node_t *)node->avl_node.base.left;
			continue;
		}

		if(ttree->compare(key, __vector_inter_get_iter_data(it_last), ttree->context) > 0)
		{
			node = (myttree_node_t *)node->avl_node.base.right;
			continue;
		}

		return node;
	}

	return NULL;
}


/**
 * @brief ��Ҷ�ڵ㴦���
 */
static __INLINE__ int ttree_add_leaf(myttree_t * ttree,
									 myttree_node_t * current_node,
									 const void * index_info)
{
	HMYVECTOR hv_index = NULL;

	assert(ttree && current_node && ttree->compare);
	assert(NULL == current_node->avl_node.base.right && NULL == current_node->avl_node.base.left);

	hv_index = current_node->key.hv_index;
	assert(hv_index && hv_index->el_array && hv_index->el_array_size);

	/*
	* ���û������
	*/
	if(__vector_inter_get_count(hv_index) < ttree->overflow)
	{
		/*
		* �ں��ʵ�λ�����
		* todo:����������Դ��T��ʵ��,ʵ�������ǲ�����ô����,ֱ�ӽ����ݲ嵽���ʵ�λ��.
		*/
		__vector_inter_add(hv_index, index_info, 0);
		ttree_sort_index_array(ttree, hv_index);

		return 0;
	}
	else
	{
		myttree_node_t * node = ttree_create_node(ttree, current_node);
		if(NULL == node)
			return -1;

		current_node->avl_node.base.right = (bstree_node_t *)node;

		/*
		* �ں��ʵ�λ�����
		* todo:����������Դ��T��ʵ��,ʵ�������ǲ�����ô����,ֱ�ӽ����ݲ嵽���ʵ�λ��.
		*/
		__vector_inter_add(hv_index, index_info, 0);
		ttree_sort_index_array(ttree, hv_index);

		/*
		* ����˽ڵ�,��ת��ƽ��
		* ����ת֮ǰ,��Ҫ�Ե�ǰ������״�����ж�,����Ԫ������,��ֹinter �ڵ����underflow�����
		*
		* ����1                 ����2(������1Ϊ����)
		*        A                    A
		*       /                      \
		*      B                        B
		*       \                       /
		*        C                     C
		* ��ʱ��Ҫ��B��������C,��ֹ����C�Ľڵ�underflow,��Ϊ��ת��,C����Ϊinter,����ͼ
		*              C                   C
		*             / \      ����       / \
		*            B   A               A   B
		*
		* �ڱ�����,����C��ΪB�ҽڵ�,����ֻ��������1,����2�ڱ������ǲ����ܳ��ֵ�
		*/

		if(current_node->avl_node.base.parent)
		{
			/* ���������״Ϊ����1 */
			if((bstree_node_t *)current_node == current_node->avl_node.base.parent->left && NULL == current_node->avl_node.base.parent->right)
			{
				/*
				* ��B�е�Ԫ�ؽ�ΪC
				* ��B�е���СԪ�ؽ��C
				* B C����Ԫ������
				*/
				__vector_inter_add(node->key.hv_index, __vector_get_head_data(hv_index), 0);
				__vector_inter_del(hv_index, 0);

				current_node->key.hv_index = node->key.hv_index;
				node->key.hv_index = hv_index;
			}
			else
			{
				/*
				* �ڵ�������
				* ȡ�����ڵ�node_max,node_max��Ϊ���ҷ�֧,����ת��
				*/
				__vector_inter_add(node->key.hv_index, __vector_get_tail_data(hv_index), 0);
				__vector_inter_del(hv_index, __vector_inter_get_count(hv_index) - 1);
			}
		}
		else
		{
			/*
			* �ڵ�������
			* ȡ�����ڵ�node_max,node_max��Ϊ���ҷ�֧,����ת��
			*/
			__vector_inter_add(node->key.hv_index, __vector_get_tail_data(hv_index), 0);
			__vector_inter_del(hv_index, __vector_inter_get_count(hv_index) - 1);
		}

		__avltree_balance((__avltree_node_t **)&ttree->root, (__avltree_node_t *)current_node);

		return 0;
	}
}

/**
 * @brief �ڰ�Ҷ�ڵ����
 */
static __INLINE__ int ttree_add_half_leaf(myttree_t * ttree,
										  myttree_node_t * current_node,
										  const void * index_info)
{
	HMYVECTOR hv_index = NULL;

	assert(ttree && current_node && ttree->compare);
	assert(NULL == current_node->avl_node.base.left || NULL == current_node->avl_node.base.right);
	assert(current_node->avl_node.base.left || current_node->avl_node.base.right);

	/*
	* ����ǰ�Ҷ�ڵ�
	* �����������������
	* ���ֻ��������,ȡ�����ڵ��Ϊ�·�֧
	* ���ֻ��������,ȡ�����ڵ��Ϊ�·�֧ ttree_add(left, max_index_info)
	*/
	hv_index = current_node->key.hv_index;
	assert(hv_index && hv_index->el_array && hv_index->el_count && hv_index->el_array_size);

	if(__vector_inter_get_count(hv_index) < ttree->overflow)
	{
		/*
		* û������
		* �ں��ʵ�λ�����
		* todo:����������Դ��T��ʵ��,ʵ�������ǲ�����ô����,ֱ�ӽ����ݲ嵽���ʵ�λ��.
		*/
		__vector_inter_add(hv_index, index_info, 0);
		ttree_sort_index_array(ttree, hv_index);

		return 0;
	}
	else
	{
		if(current_node->avl_node.base.left)
		{
			myttree_node_t * node = ttree_create_node(ttree, current_node);
			if(NULL == node)
				return -1;

			/* �����߼�,��������س��� */
			assert(NULL == current_node->avl_node.base.right);

			/*
			* ����˽ڵ�,���Ǹ����߼��ó�,��ʱ�ǲ���Ҫ��ת����
			*/
			current_node->avl_node.base.right = (bstree_node_t *)node;

			/*
			* ����
			* �ں��ʵ�λ�����
			* todo:����������Դ��T��ʵ��,ʵ�������ǲ�����ô����,ֱ�ӽ����ݲ嵽���ʵ�λ��.
			*/
			__vector_inter_add(hv_index, index_info, 0);
			ttree_sort_index_array(ttree, hv_index);

			__vector_inter_add(node->key.hv_index, __vector_get_tail_data(hv_index), 0);
			__vector_inter_del(hv_index, __vector_inter_get_count(hv_index) - 1);

			return 0;
		}
		else
		{
			/*
			* ����
			* �ں��ʵ�λ�����
			* todo:����������Դ��T��ʵ��,ʵ�������ǲ�����ô����,ֱ�ӽ����ݲ嵽���ʵ�λ��.
			*/
			__vector_inter_add(hv_index, index_info, 0);
			ttree_sort_index_array(ttree, hv_index);

			/* �����߼�,��������س��� */
			assert(current_node->avl_node.base.right);
			assert(NULL == current_node->avl_node.base.right->left && NULL == current_node->avl_node.base.right->right);

			{
				int ret = ttree_add_leaf(ttree, (myttree_node_t *)(current_node->avl_node.base.right), __vector_get_tail_data(hv_index));
				assert(ret == 0);
			}

			__vector_inter_del(hv_index, __vector_inter_get_count(hv_index) - 1);

			return 0;
		}
	}
}

/**
 * @brief ���ڲ��ڵ����
 */
static __INLINE__ int ttree_add_inter(myttree_t * ttree,
									  myttree_node_t * current_node,
									  const void * index_info)
{
	HMYVECTOR hv_index = NULL;

	assert(ttree && current_node && ttree->compare);
	assert(current_node->avl_node.base.left && current_node->avl_node.base.right);

	hv_index = current_node->key.hv_index;
	assert(hv_index && hv_index->el_array && hv_index->el_array_size && hv_index->el_count);

	if(__vector_inter_get_count(hv_index) < ttree->overflow)
	{
		/*
		* û������
		* �ں��ʵ�λ�����
		* todo:����������Դ��T��ʵ��,ʵ�������ǲ�����ô����,ֱ�ӽ����ݲ嵽���ʵ�λ��.
		*/
		__vector_inter_add(hv_index, index_info, 0);
		ttree_sort_index_array(ttree, hv_index);

		return 0;
	}
	else
	{
		/* least upper bound */
		myttree_node_t * lub_node = NULL;

		/*
		* ����
		* �ں��ʵ�λ�����
		* todo:����������Դ��T��ʵ��,ʵ�������ǲ�����ô����,ֱ�ӽ����ݲ嵽���ʵ�λ��.
		*/
		void * ii = NULL;

		__vector_inter_add(hv_index, index_info, 0);
		ttree_sort_index_array(ttree, hv_index);

		ii = __vector_get_tail_data(hv_index);

		__vector_inter_del(hv_index, __vector_inter_get_count(hv_index) - 1);

		/*
		* �����������,ȡ�����ڵ�,ttree_add(right, max_key);
		* ���뵽least upper bound�ڵ���
		*/
		lub_node = (myttree_node_t *)current_node->avl_node.base.right;
		assert(lub_node);

		while(lub_node->avl_node.base.left)
			lub_node = (myttree_node_t *)lub_node->avl_node.base.left;

		if(lub_node->avl_node.base.right)
		{
			int ret = ttree_add_half_leaf(ttree, lub_node, ii);
			assert(0 == ret);
		}
		else
		{
			int ret = ttree_add_leaf(ttree, lub_node, ii);
			assert(0 == ret);
		}

		return 0;
	}
}

/**
 * @brief ��ӽڵ�
 */
static __INLINE__ int ttree_add(myttree_t * ttree,
								const void * index_info)
{
	myttree_node_t * parent = NULL;
	myttree_node_t * node = NULL;

	assert(ttree && ttree->root && ttree->compare);

	/*
	* Ѱ��bound node
	*/
	if(ttree->root->avl_node.base.left || ttree->root->avl_node.base.right)
		node = ttree_search_bound(ttree, ttree->root, index_info, &parent);
	else
		node = ttree->root;

	if(node)
	{
		/*
		* ����ҵ�
		*/
		HMYVECTOR hv_index = node->key.hv_index;
		size_t index = 0;

		/*
		* ����ڽڵ����ҵ��ؼ���,���ʧ��,������
		*/
		if(0 == ttree_search(ttree, hv_index, index_info, &index))
			return -1;

		if(NULL == node->avl_node.base.left && NULL == node->avl_node.base.right)
		{
			/*
			* ��Ҷ�ڵ㴦���
			*/
			return ttree_add_leaf(ttree, node, index_info);
		}
		else if(NULL != node->avl_node.base.left && NULL != node->avl_node.base.right)
		{
			/*
			* ���ڲ��ڵ����
			*/
			return ttree_add_inter(ttree, node, index_info);
		}
		else
		{
			/*
			* �ڰ�Ҷ�ڵ㴦���
			*/
			return ttree_add_half_leaf(ttree, node, index_info);
		}
	}

	assert(parent);
	assert(NULL == parent->avl_node.base.left || NULL == parent->avl_node.base.right);

	if(NULL == parent->avl_node.base.left && NULL == parent->avl_node.base.right)
	{
		/*
		* ��Ҷ�ڵ����
		*/
		return ttree_add_leaf(ttree, parent, index_info);
	}
	else
	{
		/*
		* �ڰ�Ҷ�ڵ����
		*/
		return ttree_add_half_leaf(ttree, parent, index_info);
	}
}


/**
 * @brief ��Ҷ�ڵ㴦ɾ��
 */
static __INLINE__ int ttree_del_leaf(myttree_t * ttree,
									 myttree_node_t * current_node,
									 size_t index)
{
	HMYVECTOR hv_index = NULL;

	assert(ttree && current_node && (current_node->avl_node.base.parent || current_node == ttree->root));
	assert(NULL == current_node->avl_node.base.left && NULL == current_node->avl_node.base.right);

	hv_index = current_node->key.hv_index;

	__vector_inter_del(hv_index, index);

	/*
	* ������нڵ�
	*/
	if(__vector_inter_get_count(hv_index))
		return 0;

	/*
	* ���û������Ϊ����,����Ҫɾ���ڵ�,����ת��balance
	* ע��:���parentΪ��,������ǰ�ڵ��Ǹ��ڵ�,��ɾ�����ڵ�
	*/
	if(current_node->avl_node.base.parent)
	{
		/*
		* ɾ���˽ڵ�,��ת��ƽ��
		* ����ת֮ǰ,��Ҫ�Ե�ǰ������״�����ж�,����Ԫ������,��ֹinter �ڵ����underflow�����
		*
		* ��ͼ X ��ʾҪɾ���Ľڵ�
		* ����1                 ����2(������1Ϊ����)
		*        A                    A
		*       / \                  / \
		*      B   X                X   B
		*       \                       /
		*        C                     C
		* ��ʱ��Ҫ��B��������C,��ֹ����C�Ľڵ�underflow,��Ϊ��ת��,C����Ϊinter,����ͼ
		*              C                   C
		*             / \      ����       / \
		*            B   A               A   B
		*/

		if((bstree_node_t *)current_node == current_node->avl_node.base.parent->left)
		{
			myttree_node_t * b_node = (myttree_node_t *)(current_node->avl_node.base.parent->right);
			if(b_node)
			{
				myttree_node_t * c_node = (myttree_node_t *)(current_node->avl_node.base.parent->right->left);
				if(c_node && NULL == b_node->avl_node.base.right && __vector_inter_get_count(c_node->key.hv_index) <= ttree->underflow)
				{
					/*
					* ����2
					* ��B "��С����һ����<range>" ���C
					*/
					size_t range = ttree->underflow + 1 - __vector_inter_get_count(c_node->key.hv_index);

					assert(__vector_inter_get_count(b_node->key.hv_index) > ttree->underflow);

					__vector_move_range_to_end(c_node->key.hv_index, b_node->key.hv_index, 
						0, range);

					ttree_sort_index_array(ttree, c_node->key.hv_index);
				}
			}

			current_node->avl_node.base.parent->left = NULL;
		}
		else
		{
			myttree_node_t * b_node = (myttree_node_t *)(current_node->avl_node.base.parent->left);
			if(b_node)
			{
				myttree_node_t * c_node = (myttree_node_t *)(current_node->avl_node.base.parent->left->right);
				if(c_node && NULL == b_node->avl_node.base.left && __vector_inter_get_count(c_node->key.hv_index) <= ttree->underflow)
				{
					/*
					* ����1
					* ��B "������һ����<range>" ��C
					*/
					size_t range = ttree->underflow + 1 - __vector_inter_get_count(c_node->key.hv_index);

					assert(__vector_inter_get_count(b_node->key.hv_index) > ttree->underflow);

					__vector_move_range_to_end(c_node->key.hv_index, b_node->key.hv_index, 
						__vector_inter_get_count(b_node->key.hv_index) - range, range);

					ttree_sort_index_array(ttree, c_node->key.hv_index);
				}
			}

			current_node->avl_node.base.parent->right = NULL;
		}

		__avltree_balance((__avltree_node_t **)&ttree->root, (__avltree_node_t *)(current_node->avl_node.base.parent));

		/*
		* ���Ǹ��ڵ�,�ͷ�
		*/
		ttree_destroy_node(ttree, current_node);
	}

	return 0;
}

/**
 * @brief �ڰ�Ҷ�ڵ㴦ɾ��
 */
static __INLINE__ int ttree_del_half_leaf(myttree_t * ttree,
										  myttree_node_t * current_node,
										  size_t index)
{
	HMYVECTOR hv_index = NULL;
	myttree_node_t * sub_node = NULL;

	assert(ttree && current_node);
	assert(NULL == current_node->avl_node.base.left || NULL == current_node->avl_node.base.right);

	hv_index = current_node->key.hv_index;

	assert(hv_index && hv_index->el_array && hv_index->el_count && hv_index->el_array_size);
	assert(index <= hv_index->el_count);

	__vector_inter_del(hv_index, index);

	/*
	* ���û������(��ν��underflow)
	*/
	if(__vector_inter_get_count(hv_index) > ttree->underflow)
		return 0;

	/*
	* if underflow
	* ����ж��Ƿ���Ժϲ���Ӧ���ӽڵ�,
	* (����avl���Ķ���,��ʱ���ӽڵ�һ��ΪҶ�ڵ�)
	*/
	if(current_node->avl_node.base.right)
	{
		sub_node = (myttree_node_t *)(current_node->avl_node.base.right);
	
		assert(sub_node && NULL == sub_node->avl_node.base.left && NULL == sub_node->avl_node.base.right);
		assert(__vector_inter_get_count(sub_node->key.hv_index));

		/*
		* ��������ӽڵ�,ȡ�ӽڵ���С�ļ��뵱ǰ�ڵ�,����T���Ķ���,��֪��ʱ,�¼���Ľڵ�һ��������,���Բ�������
		*/
		assert(ttree->compare(__vector_inter_get_index_data(sub_node->key.hv_index, 0), 
			__vector_inter_get_index_data(hv_index, __vector_inter_get_count(hv_index) - 1),
			ttree->context) > 0);
		__vector_inter_add(hv_index, __vector_inter_get_index_data(sub_node->key.hv_index, 0), 0);

		return ttree_del_leaf(ttree, sub_node, 0);
	}
	else
	{
		size_t new_del_index = 0;

		sub_node = (myttree_node_t *)(current_node->avl_node.base.left);

		assert(sub_node && NULL == sub_node->avl_node.base.left && NULL == sub_node->avl_node.base.right);
		assert(__vector_inter_get_count(sub_node->key.hv_index));

		new_del_index = __vector_inter_get_count(sub_node->key.hv_index) - 1;

		/*
		* ��������ӽڵ�,ȡ�ӽڵ����ļ��뵱ǰ�ڵ�,����T���Ķ���,��֪��ʱ,�¼���Ľڵ�һ������С��,������Ҫ����
		*/
		assert(ttree->compare(__vector_inter_get_index_data(sub_node->key.hv_index, new_del_index), 
			__vector_inter_get_index_data(hv_index, 0),
			ttree->context) < 0);
		__vector_inter_add(hv_index, __vector_inter_get_index_data(sub_node->key.hv_index, new_del_index), 0);
		ttree_sort_index_array(ttree, hv_index);

		return ttree_del_leaf(ttree, sub_node, new_del_index);
	}
}

/**
 * @brief ���ڲ��ڵ���ɾ��
 */
static __INLINE__ int ttree_del_inter(myttree_t * ttree,
												   myttree_node_t * current_node,
												   size_t index)
{
	HMYVECTOR hv_index = NULL;
	myttree_node_t * lub_node = NULL;

	assert(current_node && ttree);
	assert(current_node->avl_node.base.left && current_node->avl_node.base.right);

	hv_index = current_node->key.hv_index;

	assert(hv_index && hv_index->el_array && hv_index->el_count && hv_index->el_array_size);
	assert(index <= hv_index->el_count);

	__vector_inter_del(hv_index, index);

	/*
	* ���û������(��ν��underflow)
	*/
	if(__vector_inter_get_count(hv_index) > ttree->underflow)
		return 0;

	/*
	* ������� underflow,��least upper bound��һ����С�ڵ�
	* ��Ϊ <Ҷ�ڵ�/��Ҷ�ڵ�> ����Ŀ���� <inter�ڵ�> Ҫ��
	* ��least upper bound��,���Լ����ƶ�����
	*/
	lub_node = (myttree_node_t *)(current_node->avl_node.base.right);
	while(lub_node->avl_node.base.left)
		lub_node = (myttree_node_t *)(lub_node->avl_node.base.left);

	assert(__vector_inter_get_count(lub_node->key.hv_index));
	__vector_inter_add(hv_index, __vector_inter_get_index_data(lub_node->key.hv_index, 0), 0);

	/* �����߼�,�������س��� */
	assert(NULL == lub_node->avl_node.base.left);
	if(lub_node->avl_node.base.right)
		ttree_del_half_leaf(ttree, lub_node, 0);
	else
		ttree_del_leaf(ttree, lub_node, 0);

	return 0;
}

/**
 * @brief ɾ���ڵ�
 *
 * ɾ���ؼ���
 *  �Ӹ��ڵ㿪ʼ��Ѱ,����ҵ�
 *  a �����ǰ�ڵ�ĸ�����ɾ��֮����Ȼ�������ޣ��������
 *  b �����ǰ�ڵ����С��С�ޣ��������ڲ��ڵ㣬���greate lower bound�ڵ��н�
 *    ���greate lower bound�ڵ���leaf�ڵ� ͬc
 *    ���greate lower bound�ڵ���half leaf�ڵ� ͬd
 *
 *  c �����half leaf�ڵ㣬�ж��Ƿ���Ժϲ���Ȼ�������Ҫ��ת��
 *  d �����leaf�ڵ�,�ж��Ƿ�Ҫɾ����Ȼ�������Ҫ��ת��
 *
 * @brief ɾ���ڵ�(ע��2)
 *	1 �Ҳ���,ʧ��
 *	2 �ҵ�,����û�г�������,ɾ��,return suc
 *	3 �ҵ�,���ɾ��֮���������,��g.l.b�ڵ��,g.l.bҲ����Ҷ�ڵ�,Ҳ���ǰ�Ҷ�ڵ�
 *		�����ǰ�ڵ��Ѿ���Ҷ�ڵ�,ֱ��ɾ��,����ڵ����Ѿ�û��Ԫ��,ȥ������ڵ� ��תƽ����
 *		�����ǰ�ڵ��ǰ�Ҷ�ڵ�,�����ܷ����ӽڵ�ϲ�(�ܺϲ�,����Ҫ��תƽ����)
 */
static __INLINE__ int ttree_del(myttree_t * ttree,
								const void * index_info,
								void ** index_info_out)
{
	size_t index = 0;
	HMYVECTOR hv_index = NULL;

	myttree_node_t * parent = NULL;
	myttree_node_t * node = ttree_search_bound(ttree, ttree->root, index_info, &parent);

	if(NULL == node)
		return -1;

	hv_index = node->key.hv_index;
	assert(hv_index && hv_index->el_array && hv_index->el_count && hv_index->el_array_size);

	/*
	* ����ڽڵ����ҵ��ؼ���,ɾ��ʧ��,����
	*/
	if(0 != ttree_search(ttree, hv_index, index_info, &index))
		return -1;

	if(index_info_out)
		*index_info_out = __vector_inter_get_index_data(hv_index, index);

	if(node->avl_node.base.left && node->avl_node.base.right)
	{
		/*
		* ���ڲ��ڵ���ɾ��
		*/
		ttree_del_inter(ttree, node, index);
	}
	else if(NULL == node->avl_node.base.left && NULL == node->avl_node.base.right)
	{
		/*
		* ��Ҷ�ڵ㴦ɾ��
		*/
		ttree_del_leaf(ttree, node, index);
	}
	else
	{
		/*
		* �ڰ�Ҷ�ڵ㴦ɾ��
		*/
		ttree_del_half_leaf(ttree, node, index);
	}

	return 0;
}


/**
 *
 * @brief T������
 *
 * @param hm:�ڴ��
 * @param compare:�Ƚϻص�����
 * @param key_op:�����ؼ��ֵĹ��������뿽��
 * @param data_op:�������ݵĹ��������뿽��
 *
 */
HMYTTREE MyTTreeConstruct(HMYMEMPOOL hm, ALG_COMPARE compare, 
						  size_t underflow, size_t overflow)
{
	HMYTTREE httree = (HMYTTREE)MyMemPoolMalloc(hm, sizeof(*httree));
	if(NULL == httree)
		return NULL;

	assert(compare);

	memset(httree, 0, sizeof(*httree));

	httree->hm = hm;
	httree->compare = compare;
	httree->overflow = overflow;
	httree->underflow = underflow;

	httree->root = ttree_create_node(httree, NULL);
	if(NULL == httree->root)
	{
		MyMemPoolFree(httree->hm, httree);
		return NULL;
	}

	return httree;
}

/**
 * @brief T������
 */
void MyTTreeDestruct(HMYTTREE httree)
{
	if(NULL == httree)
		return;

	assert(httree->root);

	__bstree_erase((bstree_node_t *)httree->root, httree, __ttree_destroy_node_cb);

	MyMemPoolFree(httree->hm, httree);
}

/**
 * @brief ��Ӽ�¼
 *
 * @param key:�ؼ���
 * @param key_size:�ؼ��ֻ������Ĵ�С
 * @param data:����
 * @param data_size:���ݵĴ�С
 */
int MyTTreeAdd(HMYTTREE httree, const void * index_info)
{
	if(NULL == httree)
		return -1;

	assert(httree->root);

	return ttree_add(httree, index_info);
}

/**
 * @brief ɾ����¼
 *
 * @param key:Ҫɾ���Ĺؼ���
 */
int MyTTreeDel(HMYTTREE httree, const void * index_info, void ** index_info_out)
{
	if(NULL == httree)
		return -1;

	assert(httree->root);

	return ttree_del(httree, index_info, index_info_out);
}

/**
 * @brief ���Ҽ�¼
 *
 * @param key:Ҫɾ���Ĺؼ���
 */
int MyTTreeSearch(HMYTTREE httree, const void * index_info, void ** index_info_out)
{
	size_t index = 0;
	HMYVECTOR hv_index = NULL;

	myttree_node_t * parent;
	myttree_node_t * node = NULL;

	if(NULL == httree)
		return -1;

	node = ttree_search_bound(httree, httree->root, index_info, &parent);
	if(NULL == node)
		return -1;

	hv_index = node->key.hv_index;
	assert(hv_index && hv_index->el_array && hv_index->el_count && hv_index->el_array_size);

	/*
	* ����ڽڵ����ҵ��ؼ���,���ʧ��,������
	*/
	if(0 != ttree_search(httree, hv_index, index_info, &index))
		return -1;

	if(index_info_out)
		*index_info_out = __vector_inter_get_index_data(hv_index, index);

	return 0;
}


/**
 * ��ȡT���еĽڵ����
 */
static __INLINE__ size_t ttree_get_count(myttree_node_t * node)
{
	size_t mid_size = 0;
	size_t r_size = 0;
	size_t l_size = 0;

	assert(node && node->key.hv_index);

	mid_size = __vector_inter_get_count(node->key.hv_index);

	if(node->avl_node.base.left)
		l_size = ttree_get_count((myttree_node_t *)(node->avl_node.base.left));

	if(node->avl_node.base.right)
		r_size = ttree_get_count((myttree_node_t *)(node->avl_node.base.right));

	return mid_size + r_size + l_size;
}

/**
 * ��ȡT���еĽڵ����
 */
size_t MyTTreeGetCount(HMYTTREE httree)
{
	if(NULL == httree)
		return 0;

	assert(httree->root);
	return ttree_get_count(httree->root);
}


/**
 * @brief ���T���Ϸ���
 */
static __INLINE__ size_t ttree_examin(myttree_t * ttree, myttree_node_t * node, int bprint)
{
	size_t left = 0;
	size_t right = 0;
	HMYVECTOR hv_index = NULL;

	assert(ttree && node && ttree->compare);

	hv_index = node->key.hv_index;

	if(bprint)
	{
		printf("examin node:");
		MyVectorPrint(hv_index);
	}

	assert(hv_index && hv_index->el_array && hv_index->el_array_size && 
		(hv_index->el_count || (node == ttree->root && NULL == ttree->root->avl_node.base.left && NULL == ttree->root->avl_node.base.right)));

	/*
	* �ڵ��index�����������
	*/
	assert(0 == MyAlgSortOK(__vector_inter_get_array((hv_index)),
			__vector_inter_get_count((hv_index)),
			__vector_inter_get_array_step_size((hv_index)),
			ttree_compare, ttree));

	/*
	* ��Ҷ�ڵ�,������Ԫ�ظ���Ӧ���ܳ��������������
	*/
	if(node->avl_node.base.left || node->avl_node.base.right)
		assert(hv_index->el_count > ttree->underflow && hv_index->el_count <= ttree->overflow);
	else
		assert(hv_index->el_count || 
			(node == ttree->root && NULL == ttree->root->avl_node.base.left && NULL == ttree->root->avl_node.base.right));

	if(node->avl_node.base.left)
	{
		/*
		* ��ǰ�ڵ��������֧��"��"
		*/
		HMYVECTOR_ITER it1 = NULL;
		HMYVECTOR_ITER it2 = NULL;
		myttree_node_t * node_left = (myttree_node_t *)(node->avl_node.base.left);


		it1 = __vector_get_head(hv_index);
		it2 = __vector_get_tail(node_left->key.hv_index);

		if(bprint)
		{
			printf("left:");
			MyVectorPrint(node_left->key.hv_index);
		}

		assert(ttree->compare(__vector_inter_get_iter_data(it1), __vector_inter_get_iter_data(it2), ttree->context) > 0);

		left = ttree_examin(ttree, (myttree_node_t *)node->avl_node.base.left, bprint);
		assert(node->avl_node.base.left->parent == (bstree_node_t *)node);
	}
	else
		left = 0;

	if(node->avl_node.base.right)
	{
		/*
		* ��ǰ�ڵ��������֧��"С"
		*/
		HMYVECTOR_ITER it1 = NULL;
		HMYVECTOR_ITER it2 = NULL;
		myttree_node_t * node_right = (myttree_node_t *)(node->avl_node.base.right);

		it1 = __vector_get_tail(hv_index);
		it2 = __vector_get_head(node_right->key.hv_index);

		if(bprint)
		{
			printf("right:");
			MyVectorPrint(node_right->key.hv_index);
		}

		assert(ttree->compare(__vector_inter_get_iter_data(it1), 
			__vector_inter_get_iter_data(it2),
			ttree->context) < 0);

		right = ttree_examin(ttree, (myttree_node_t *)node->avl_node.base.right, bprint);
		assert(node->avl_node.base.right->parent == (bstree_node_t *)node);
	}
	else
		right = 0;

	assert(right + 1 == left || left + 1 == right || left == right);

	assert(node->avl_node.height == right + 1 || node->avl_node.height == left + 1);
	assert(right < node->avl_node.height && left < node->avl_node.height);

	return (left > right)?(left + 1):(right + 1);
}

/**
 * ���T���ĺϷ���
 */
void MyTTreeExamin(HMYTTREE httree, int bprint)
{
	assert(httree && httree->root && httree->compare);

	ttree_examin(httree, httree->root, bprint);
}

















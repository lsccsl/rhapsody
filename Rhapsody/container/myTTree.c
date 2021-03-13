/**
 *
 * @file myTTree.c T树
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief
 * T 树是avl树的扩展(或者说是avl树与b树综合的产物)
 *  即一个node包含一个关键字数组.
 *  数组里的关键字已经被排好序
 *  oracle ten times, fastdb, mysql cluster等,使用了T树做为索引.
 *
 * 添加关键字至T树
 * 从根节点开始找寻,如果能找到bound数组,则添加,如果数组已满,抽出最小的那一个元素A.
 * 行进至greate lower bound(左子树最靠右的那个节点),如果可以添加A,则添加
 * 如果不能,则应创建新节点，并且根据实际情况旋转树保证avl树的平衡
 *
 * 如果没有找到合适的节点，则在最右(左)添加，添加不了则创建新节点，并根据实际情况旋转树
 *
 * 删除关键字
 *  从根节点开始找寻,如果找到
 *  a 如果当前节点的个数在删除之后仍然大于下限，操作完成
 *  b 如果当前节点个数小于小限，并且是内部节点，则从greate lower bound节点中借
 *    如果greate lower bound节点是leaf节点 同c
 *    如果greate lower bound节点是half leaf节点 同d
 *
 *  c 如果是half leaf节点，判断是否可以合并，然后根据需要旋转树
 *  d 如果是leaf节点,判断是否要删除，然后根据需要旋转树
 *
 * 关于数组排序:
 *   fastDB与mysql使用的都是二分查找定位排序
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
	* 数组,存放索引信息
	*/
	HMYVECTOR hv_index;
}myttree_key_t;

typedef struct __myttree_node_t_
{
	/*
	* avl平衡树节点
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
	* T树的根节点
	*/
	myttree_node_t * root;

	/*
	* 一个节点的关键字个数下限
	*/
	size_t underflow;

	/*
	* 一个节点的关键个数上限
	*/
	size_t overflow;
}myttree_t;


/**
 * 索引数组排序
 */
#define ttree_sort_index_array(ttree, hv_index) do{\
		MyAlgQuickSort(__vector_inter_get_array((hv_index)),\
			__vector_inter_get_count((hv_index)), \
			__vector_inter_get_array_step_size((hv_index)),\
			ttree_compare, NULL, NULL, NULL, ttree, NULL, 0);\
	}while(0)


/**
 * T树比较
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
 * 索引数组查找
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
 * 创建一个T树节点
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
 * 销毁一个T树节点
 */
static __INLINE__ void ttree_destroy_node(myttree_t * ttree, myttree_node_t * node)
{
	assert(ttree && node && node->key.hv_index);

	__vector_inter_destroy(node->key.hv_index);

	MyMemPoolFree(ttree->hm, node);
}

/**
 * @brief 描述在__bstree_erase函数中如何释放每个节点
 * @param context:用户的上下文数据
 * @param root:根节点
 */
static void __ttree_destroy_node_cb(void * context, bstree_node_t * root)
{
	ttree_destroy_node((myttree_t *)context, (myttree_node_t *)root);
}


/**
 * @brief 查找合适的节点位置
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
 * @brief 在叶节点处添加
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
	* 如果没有上溢
	*/
	if(__vector_inter_get_count(hv_index) < ttree->overflow)
	{
		/*
		* 在合适的位置添加
		* todo:看了其它开源的T树实现,实际上它们不是这么做的,直接将数据插到合适的位置.
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
		* 在合适的位置添加
		* todo:看了其它开源的T树实现,实际上它们不是这么做的,直接将数据插到合适的位置.
		*/
		__vector_inter_add(hv_index, index_info, 0);
		ttree_sort_index_array(ttree, hv_index);

		/*
		* 添加了节点,旋转树平衡
		* 在旋转之前,需要对当前树的形状进行判断,调整元素数组,防止inter 节点出现underflow的情况
		*
		* 情形1                 情形2(与情形1为镜像)
		*        A                    A
		*       /                      \
		*      B                        B
		*       \                       /
		*        C                     C
		* 此时需要所B中数组搬给C,防止出现C的节点underflow,因为旋转后,C将成为inter,如下图
		*              C                   C
		*             / \      或者       / \
		*            B   A               A   B
		*
		* 在本例中,由于C必为B右节点,所以只考虑情形1,情形2在本例中是不可能出现的
		*/

		if(current_node->avl_node.base.parent)
		{
			/* 如果树的形状为情形1 */
			if((bstree_node_t *)current_node == current_node->avl_node.base.parent->left && NULL == current_node->avl_node.base.parent->right)
			{
				/*
				* 把B中的元素借为C
				* 将B中的最小元素借给C
				* B C互换元素数组
				*/
				__vector_inter_add(node->key.hv_index, __vector_get_head_data(hv_index), 0);
				__vector_inter_del(hv_index, 0);

				current_node->key.hv_index = node->key.hv_index;
				node->key.hv_index = hv_index;
			}
			else
			{
				/*
				* 节点上溢了
				* 取出最大节点node_max,node_max成为新右分支,并旋转树
				*/
				__vector_inter_add(node->key.hv_index, __vector_get_tail_data(hv_index), 0);
				__vector_inter_del(hv_index, __vector_inter_get_count(hv_index) - 1);
			}
		}
		else
		{
			/*
			* 节点上溢了
			* 取出最大节点node_max,node_max成为新右分支,并旋转树
			*/
			__vector_inter_add(node->key.hv_index, __vector_get_tail_data(hv_index), 0);
			__vector_inter_del(hv_index, __vector_inter_get_count(hv_index) - 1);
		}

		__avltree_balance((__avltree_node_t **)&ttree->root, (__avltree_node_t *)current_node);

		return 0;
	}
}

/**
 * @brief 在半叶节点添加
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
	* 如果是半叶节点
	* 区分左右子树的情况
	* 如果只有左子树,取出最大节点成为新分支
	* 如果只有右子树,取出最大节点成为新分支 ttree_add(left, max_index_info)
	*/
	hv_index = current_node->key.hv_index;
	assert(hv_index && hv_index->el_array && hv_index->el_count && hv_index->el_array_size);

	if(__vector_inter_get_count(hv_index) < ttree->overflow)
	{
		/*
		* 没有上溢
		* 在合适的位置添加
		* todo:看了其它开源的T树实现,实际上它们不是这么做的,直接将数据插到合适的位置.
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

			/* 根据逻辑,这个条件必成立 */
			assert(NULL == current_node->avl_node.base.right);

			/*
			* 添加了节点,但是根据逻辑得出,此时是不需要旋转树的
			*/
			current_node->avl_node.base.right = (bstree_node_t *)node;

			/*
			* 上溢
			* 在合适的位置添加
			* todo:看了其它开源的T树实现,实际上它们不是这么做的,直接将数据插到合适的位置.
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
			* 上溢
			* 在合适的位置添加
			* todo:看了其它开源的T树实现,实际上它们不是这么做的,直接将数据插到合适的位置.
			*/
			__vector_inter_add(hv_index, index_info, 0);
			ttree_sort_index_array(ttree, hv_index);

			/* 根据逻辑,这个条件必成立 */
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
 * @brief 在内部节点添加
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
		* 没有上溢
		* 在合适的位置添加
		* todo:看了其它开源的T树实现,实际上它们不是这么做的,直接将数据插到合适的位置.
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
		* 上溢
		* 在合适的位置添加
		* todo:看了其它开源的T树实现,实际上它们不是这么做的,直接将数据插到合适的位置.
		*/
		void * ii = NULL;

		__vector_inter_add(hv_index, index_info, 0);
		ttree_sort_index_array(ttree, hv_index);

		ii = __vector_get_tail_data(hv_index);

		__vector_inter_del(hv_index, __vector_inter_get_count(hv_index) - 1);

		/*
		* 如果出现上溢,取出最大节点,ttree_add(right, max_key);
		* 加入到least upper bound节点里
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
 * @brief 添加节点
 */
static __INLINE__ int ttree_add(myttree_t * ttree,
								const void * index_info)
{
	myttree_node_t * parent = NULL;
	myttree_node_t * node = NULL;

	assert(ttree && ttree->root && ttree->compare);

	/*
	* 寻找bound node
	*/
	if(ttree->root->avl_node.base.left || ttree->root->avl_node.base.right)
		node = ttree_search_bound(ttree, ttree->root, index_info, &parent);
	else
		node = ttree->root;

	if(node)
	{
		/*
		* 如果找到
		*/
		HMYVECTOR hv_index = node->key.hv_index;
		size_t index = 0;

		/*
		* 如果在节点中找到关键字,添加失败,并返回
		*/
		if(0 == ttree_search(ttree, hv_index, index_info, &index))
			return -1;

		if(NULL == node->avl_node.base.left && NULL == node->avl_node.base.right)
		{
			/*
			* 在叶节点处添加
			*/
			return ttree_add_leaf(ttree, node, index_info);
		}
		else if(NULL != node->avl_node.base.left && NULL != node->avl_node.base.right)
		{
			/*
			* 在内部节点添加
			*/
			return ttree_add_inter(ttree, node, index_info);
		}
		else
		{
			/*
			* 在半叶节点处添加
			*/
			return ttree_add_half_leaf(ttree, node, index_info);
		}
	}

	assert(parent);
	assert(NULL == parent->avl_node.base.left || NULL == parent->avl_node.base.right);

	if(NULL == parent->avl_node.base.left && NULL == parent->avl_node.base.right)
	{
		/*
		* 在叶节点添加
		*/
		return ttree_add_leaf(ttree, parent, index_info);
	}
	else
	{
		/*
		* 在半叶节点添加
		*/
		return ttree_add_half_leaf(ttree, parent, index_info);
	}
}


/**
 * @brief 在叶节点处删除
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
	* 如果还有节点
	*/
	if(__vector_inter_get_count(hv_index))
		return 0;

	/*
	* 如果没有数组为空了,则需要删除节点,并旋转树balance
	* 注意:如果parent为空,表明当前节点是根节点,不删除根节点
	*/
	if(current_node->avl_node.base.parent)
	{
		/*
		* 删除了节点,旋转树平衡
		* 在旋转之前,需要对当前树的形状进行判断,调整元素数组,防止inter 节点出现underflow的情况
		*
		* 如图 X 表示要删除的节点
		* 情形1                 情形2(与情形1为镜像)
		*        A                    A
		*       / \                  / \
		*      B   X                X   B
		*       \                       /
		*        C                     C
		* 此时需要所B中数组搬给C,防止出现C的节点underflow,因为旋转后,C将成为inter,如下图
		*              C                   C
		*             / \      或者       / \
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
					* 情形2
					* 把B "最小的那一部分<range>" 借给C
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
					* 情形1
					* 把B "最大的那一部分<range>" 给C
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
		* 不是根节点,释放
		*/
		ttree_destroy_node(ttree, current_node);
	}

	return 0;
}

/**
 * @brief 在半叶节点处删除
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
	* 如果没有下溢(所谓的underflow)
	*/
	if(__vector_inter_get_count(hv_index) > ttree->underflow)
		return 0;

	/*
	* if underflow
	* 如果判断是否可以合并相应的子节点,
	* (根据avl树的定义,此时的子节点一定为叶节点)
	*/
	if(current_node->avl_node.base.right)
	{
		sub_node = (myttree_node_t *)(current_node->avl_node.base.right);
	
		assert(sub_node && NULL == sub_node->avl_node.base.left && NULL == sub_node->avl_node.base.right);
		assert(__vector_inter_get_count(sub_node->key.hv_index));

		/*
		* 如果在右子节点,取子节点最小的加入当前节点,根据T树的定义,可知此时,新加入的节点一定是最大的,所以不必排序
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
		* 如果在右子节点,取子节点最大的加入当前节点,根据T树的定义,可知此时,新加入的节点一定是最小的,所以需要排序
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
 * @brief 在内部节点中删除
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
	* 如果没有下溢(所谓的underflow)
	*/
	if(__vector_inter_get_count(hv_index) > ttree->underflow)
		return 0;

	/*
	* 如果下溢 underflow,向least upper bound借一个最小节点
	* 因为 <叶节点/半叶节点> 的数目比起 <inter节点> 要少
	* 向least upper bound借,可以减少移动负担
	*/
	lub_node = (myttree_node_t *)(current_node->avl_node.base.right);
	while(lub_node->avl_node.base.left)
		lub_node = (myttree_node_t *)(lub_node->avl_node.base.left);

	assert(__vector_inter_get_count(lub_node->key.hv_index));
	__vector_inter_add(hv_index, __vector_inter_get_index_data(lub_node->key.hv_index, 0), 0);

	/* 根据逻辑,此条件必成立 */
	assert(NULL == lub_node->avl_node.base.left);
	if(lub_node->avl_node.base.right)
		ttree_del_half_leaf(ttree, lub_node, 0);
	else
		ttree_del_leaf(ttree, lub_node, 0);

	return 0;
}

/**
 * @brief 删除节点
 *
 * 删除关键字
 *  从根节点开始找寻,如果找到
 *  a 如果当前节点的个数在删除之后仍然大于下限，操作完成
 *  b 如果当前节点个数小于小限，并且是内部节点，则从greate lower bound节点中借
 *    如果greate lower bound节点是leaf节点 同c
 *    如果greate lower bound节点是half leaf节点 同d
 *
 *  c 如果是half leaf节点，判断是否可以合并，然后根据需要旋转树
 *  d 如果是leaf节点,判断是否要删除，然后根据需要旋转树
 *
 * @brief 删除节点(注释2)
 *	1 找不到,失败
 *	2 找到,并且没有出现下溢,删除,return suc
 *	3 找到,如果删除之后出现下溢,向g.l.b节点借,g.l.b也许是叶节点,也许是半叶节点
 *		如果当前节点已经是叶节点,直接删除,如果节点中已经没有元素,去除这个节点 旋转平衡树
 *		如果当前节点是半叶节点,看看能否与子节点合并(能合并,则需要旋转平衡树)
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
	* 如果在节点中找到关键字,删除失败,返回
	*/
	if(0 != ttree_search(ttree, hv_index, index_info, &index))
		return -1;

	if(index_info_out)
		*index_info_out = __vector_inter_get_index_data(hv_index, index);

	if(node->avl_node.base.left && node->avl_node.base.right)
	{
		/*
		* 在内部节点中删除
		*/
		ttree_del_inter(ttree, node, index);
	}
	else if(NULL == node->avl_node.base.left && NULL == node->avl_node.base.right)
	{
		/*
		* 在叶节点处删除
		*/
		ttree_del_leaf(ttree, node, index);
	}
	else
	{
		/*
		* 在半叶节点处删除
		*/
		ttree_del_half_leaf(ttree, node, index);
	}

	return 0;
}


/**
 *
 * @brief T树构造
 *
 * @param hm:内存池
 * @param compare:比较回调函数
 * @param key_op:描述关键字的构造析构与拷贝
 * @param data_op:描述数据的构造析构与拷贝
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
 * @brief T树析构
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
 * @brief 添加记录
 *
 * @param key:关键字
 * @param key_size:关键字缓冲区的大小
 * @param data:数据
 * @param data_size:数据的大小
 */
int MyTTreeAdd(HMYTTREE httree, const void * index_info)
{
	if(NULL == httree)
		return -1;

	assert(httree->root);

	return ttree_add(httree, index_info);
}

/**
 * @brief 删除记录
 *
 * @param key:要删除的关键字
 */
int MyTTreeDel(HMYTTREE httree, const void * index_info, void ** index_info_out)
{
	if(NULL == httree)
		return -1;

	assert(httree->root);

	return ttree_del(httree, index_info, index_info_out);
}

/**
 * @brief 查找记录
 *
 * @param key:要删除的关键字
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
	* 如果在节点中找到关键字,添加失败,并返回
	*/
	if(0 != ttree_search(httree, hv_index, index_info, &index))
		return -1;

	if(index_info_out)
		*index_info_out = __vector_inter_get_index_data(hv_index, index);

	return 0;
}


/**
 * 获取T树中的节点个数
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
 * 获取T树中的节点个数
 */
size_t MyTTreeGetCount(HMYTTREE httree)
{
	if(NULL == httree)
		return 0;

	assert(httree->root);
	return ttree_get_count(httree->root);
}


/**
 * @brief 检查T树合法性
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
	* 节点的index数组必须有序
	*/
	assert(0 == MyAlgSortOK(__vector_inter_get_array((hv_index)),
			__vector_inter_get_count((hv_index)),
			__vector_inter_get_array_step_size((hv_index)),
			ttree_compare, ttree));

	/*
	* 非叶节点,数组中元素个数应不能出现上溢或者下溢
	*/
	if(node->avl_node.base.left || node->avl_node.base.right)
		assert(hv_index->el_count > ttree->underflow && hv_index->el_count <= ttree->overflow);
	else
		assert(hv_index->el_count || 
			(node == ttree->root && NULL == ttree->root->avl_node.base.left && NULL == ttree->root->avl_node.base.right));

	if(node->avl_node.base.left)
	{
		/*
		* 当前节点必须比左分支的"大"
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
		* 当前节点必须比左分支的"小"
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
 * 检查T树的合法性
 */
void MyTTreeExamin(HMYTTREE httree, int bprint)
{
	assert(httree && httree->root && httree->compare);

	ttree_examin(httree, httree->root, bprint);
}

















/**
 *
 * @file myBTree.c B树
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief
 * 根节点如果是非叶子节点,则必有两个子节点.
 *
 * 一个m阶的B树 每个非叶子节点含有n(m/2 ～ m-1)个关键字 含有n+1个子树
 * 所有的叶子节点为空
 * 每个节点的关键字顺序排列
 *
 * 其中 key(P1) >= k1  && key(P2) < k2
 *
 *
 * 添加节点时，在底层添加，
 * 如果满了，则分裂该节点，并将中间的关键字往上移,
 * 递归直到根节点,
 * 如果根节点也满了，此时树长高了一层.
 *
 *
 * 删除节点时.可以使用自顶向下的方示来实现.
 * 从根节点开始搜寻要删除的key_del,
 * 确定key_del所在的分支，如果分支的关键字个数等于t(最小度数),
 * 则进行合并（左右兄弟均有不足的情况），或者借（左右兄弟有一个满）。
 *
 * 如果找到key_del,用前驱，或者后继替代key_del,递归删除前驱/后继
 *
 * 如此递归直到叶节点
 *
 * todo:B树实际应用是与辅存相结合的.
 */
#include "myBTree.h"

#include <assert.h>
#include <stdio.h>

#include "myvector.h"
#include "__vector_inter.h"


typedef struct __mybtree_node_t_
{
	/*
	* 关键字数组
	*/
	HMYVECTOR hv_key;

	/*
	* 子节点数组
	*/
	HMYVECTOR hv_sub;
}mybtree_node_t;

typedef struct __mybtree_t_
{
	/* 根节点 */
	mybtree_node_t * root;

	/* 内存池 */
	HMYMEMPOOL hm;

	/* 节点的最小子节点个数 */
	size_t min_sub;

	/*
	* 比较回调,与保留字段
	*/
	ALG_COMPARE compare;
	void * context;

	/* 记录数 */
	size_t index_count;
}mybtree_t;


/**
 * T树比较
 */
static __INLINE__ int btree_compare(const void * data1, const void * data2, const void * context)
{
	HMYVECTOR_ITER it1 = *((HMYVECTOR_ITER *)data1);
	HMYVECTOR_ITER it2 = *((HMYVECTOR_ITER *)data2);
	mybtree_t * btree = (mybtree_t *)context;

	assert(btree && it1 && it2 && btree->compare);

	return btree->compare(__vector_inter_get_iter_data(it1), __vector_inter_get_iter_data(it2), btree->context);
}

/**
 * 索引数组查找
 */
static __INLINE__ int btree_search(mybtree_t * btree, HMYVECTOR hv_index, const void * index_info, size_t * index)
{
	assert(btree && hv_index);

	{
		__vector_inter_get_temp_it(it, 0, (void *)index_info);

		return MyBinarySearch1(__vector_inter_get_array(hv_index),
			__vector_inter_get_count(hv_index),
			__vector_inter_get_array_step_size(hv_index),
			&it, btree_compare, index, btree);
	}
}

/**
 * 销毁一个b树节点
 */
static __INLINE__ void destroy_btree_node(mybtree_t * btree, mybtree_node_t * node)
{
	assert(btree && node);

	assert(node->hv_key);
	assert(node->hv_sub);

	assert(__vector_inter_get_count(node->hv_key) == 0);
	assert(__vector_inter_get_count(node->hv_sub) == 0 || __vector_get_head_data(node->hv_sub) == btree->root);

	__vector_inter_destroy(node->hv_key);
	__vector_inter_destroy(node->hv_sub);

	MyMemPoolFree(btree->hm, node);
}

/**
 * 清空B树
 */
static __INLINE__ void btree_clear(mybtree_t * btree, mybtree_node_t * node)
{
	size_t i = 0;
	size_t sub_count = 0;

	assert(btree);
	assert(btree->root);

	sub_count = __vector_inter_get_count(node->hv_sub);
	for(i = 0; i < sub_count; i ++)
	{
		assert(__vector_inter_get_index_data(node->hv_sub, i));

		btree_clear(btree, (mybtree_node_t *)__vector_inter_get_index_data(node->hv_sub, i));
	}

	if(node != btree->root)
	{
		__vector_inter_destroy(node->hv_sub);
		__vector_inter_destroy(node->hv_key);
		MyMemPoolFree(btree->hm, node);
	}
	else
	{
		__vector_inter_clear(node->hv_sub);
		__vector_inter_clear(node->hv_key);
	}
}

/**
 * 销毁B树
 */
static void btree_destroy(mybtree_t * btree)
{
	assert(btree);
	assert(btree->root);

	btree_clear(btree, btree->root);

	destroy_btree_node(btree, btree->root);

	MyMemPoolFree(btree->hm, btree);
}

/**
 * 搜索B树
 */
static __INLINE__ mybtree_node_t * search_btree(mybtree_t * btree, const void * index_info, mybtree_node_t ** parent, size_t * pindex)
{
	mybtree_node_t * node = NULL;

	assert(btree && btree->root && btree->compare);
	assert(btree->root->hv_key && btree->root->hv_sub);
	assert(pindex);

	node = btree->root;

	while(1)
	{
		if(parent)
			*parent = node;

		assert(__vector_inter_get_count(node->hv_key) + 1 == __vector_inter_get_count(node->hv_sub) || node == btree->root ||
			__vector_inter_get_count(node->hv_sub) == 0);

		if(0 == btree_search(btree, node->hv_key, index_info, pindex))
			return node;

		if(0 == __vector_inter_get_count(node->hv_sub))
			return NULL;

		assert(*pindex < __vector_inter_get_count(node->hv_sub));

		node = (mybtree_node_t *)__vector_inter_get_index_data(node->hv_sub, *pindex);
	}

	return NULL;
}

/**
 * 创建一个b树节点
 */
static __INLINE__ mybtree_node_t * create_btree_node(mybtree_t * btree)
{
	mybtree_node_t * node = NULL;

	assert(btree);

	node = (mybtree_node_t *)MyMemPoolMalloc(btree->hm, sizeof(*node));
	if(NULL == node)
		return NULL;

	node->hv_key = __vector_inter_create(btree->hm, btree->min_sub * 2 + 1, NULL, NULL);
	node->hv_sub = __vector_inter_create(btree->hm, btree->min_sub * 2 + 1, NULL, NULL);

	if(NULL == node->hv_key || NULL == node->hv_sub)
		goto create_btree_node_err_;

	return node;

create_btree_node_err_:

	if(node->hv_key)
		__vector_inter_destroy(node->hv_key);
	if(node->hv_sub)
		__vector_inter_destroy(node->hv_sub);

	MyMemPoolFree(btree->hm, node);

	return NULL;
}

/**
 * 添加节点
 */
static __INLINE__ int btree_add(mybtree_t * btree, void * index_info)
{
	/*
	* 使用自顶向下的方法添加节点,
	* 添加必然会在"叶节点"处发生
	* 在查找添加节点的过程中,沿途依次分裂节点
	* 在叶节点添加,根据需要分裂叶节点
	*  
	* 自顶向下的缺点:
	* 使用自顶向下的方法,如果添加与删除是交替进行的,
	* 则有可有造成一个节点因为添加分裂之后,又因为删除而合并
	*
	* 回逆法:
	* 1: 如果叶节点未满,直接添加
	* 2: 如果叶节点已满,分裂,将中间关键字加入父节点的适当位置
	* 3: 如果父节点已满,递归向上
	* 回朔的缺点:分裂更新父节点问题,以及每个子节点的父亲点中索引的计算
	*
	* 下面的代码描述了自顶向下的添加一个节点至B树中的过程
	*/

	size_t index = 0;
	size_t old_index = 0;
	mybtree_node_t * node = NULL;
	mybtree_node_t * parent = NULL;
	mybtree_node_t * node_new = NULL;

	assert(btree);
	assert(btree->root && btree->compare);

	node = btree->root;
	while(1)
	{
		assert(__vector_inter_get_count(node->hv_key) + 1 == __vector_inter_get_count(node->hv_sub) || node == btree->root ||
			__vector_inter_get_count(node->hv_sub) == 0);

		if(0 == btree_search(btree, node->hv_key, index_info, &index))
			return -1;

		assert(index <= __vector_inter_get_count(node->hv_key));

		if(__vector_inter_get_count(node->hv_sub) == 0)
			break;

		if(__vector_inter_get_count(node->hv_sub) < btree->min_sub * 2)
		{
			parent = node;
			node = (mybtree_node_t *)__vector_inter_get_index_data(node->hv_sub, index);
			old_index = index;
			continue;
		}

		/* 处于临界点,应当分裂 */
		node_new = create_btree_node(btree);
		if(NULL == node_new)
			return -1;

		__vector_move_range_to_end(node_new->hv_key, node->hv_key, btree->min_sub, btree->min_sub - 1);

		assert(btree->min_sub == __vector_inter_get_count(node_new->hv_key) + 1);
		assert(btree->min_sub == __vector_inter_get_count(node->hv_key));

		/*
		* 如果是根节点,则需要创建父节点
		*/
		if(NULL == parent)
		{
			assert(node == btree->root);

			parent = create_btree_node(btree);
			assert(parent);
			btree->root = parent;

			__vector_inter_insert_at(parent->hv_sub, 0, node, 0);
		}
		__vector_move_range_to_index(parent->hv_key, old_index, node->hv_key, __vector_inter_get_count(node->hv_key) - 1, 1);

		__vector_inter_insert_at(parent->hv_sub, old_index + 1, node_new, 0);

		{
			mybtree_node_t * temp = (mybtree_node_t *)__vector_inter_get_index_data(node->hv_sub, index);

			__vector_move_range_to_end(node_new->hv_sub, node->hv_sub, btree->min_sub, btree->min_sub);

			assert(__vector_inter_get_count(node->hv_sub) <= btree->min_sub * 2);
			assert(__vector_inter_get_count(node->hv_sub) >= btree->min_sub);
			assert(__vector_inter_get_count(node->hv_key) + 1 == __vector_inter_get_count(node->hv_sub));

			assert(__vector_inter_get_count(node_new->hv_sub) <= btree->min_sub * 2);
			assert(__vector_inter_get_count(node_new->hv_sub) >= btree->min_sub);
			assert(__vector_inter_get_count(node_new->hv_key) + 1 == __vector_inter_get_count(node_new->hv_sub));

			/*
			* 根据所在分支,决定父节点以及相应的下标
			*/
			if(index < btree->min_sub)
			{
				old_index = index;
				parent = node;
				node = temp;
			}
			else
			{
				old_index = index - btree->min_sub;
				parent = node_new;
				node = temp;
			}
		}
	}

	if(__vector_inter_get_count(node->hv_key) < btree->min_sub * 2 - 1)
	{
		/* 
		* 找到合适的"叶节点" 
		* 如果当前叶节点没有上溢了 
		*/
		__vector_inter_insert_at(node->hv_key, index, index_info, 0);

		btree->index_count ++;

		return 0;
	}

	/* 如果当前叶节点上溢了 */
	node_new = create_btree_node(btree);
	if(NULL == node_new)
		return -1;

	__vector_inter_insert_at(node->hv_key, index, index_info, 0);

	/*
	* 节点分裂
	* 前t个节点 后t-1个节点成为分裂后的节点, 中间的那个节点加入父节点
	*/
	__vector_move_range_to_end(node_new->hv_key, node->hv_key, btree->min_sub + 1, btree->min_sub - 1);
	assert(btree->min_sub - 1 == __vector_inter_get_count(node_new->hv_key));

	/*
	* 如果是根节点,则需要创建父节点
	*/
	if(NULL == parent)
	{
		assert(node == btree->root);

		parent = create_btree_node(btree);
		assert(parent);
		btree->root = parent;

		__vector_inter_insert_at(parent->hv_sub, 0, node, 0);
	}
	__vector_move_range_to_index(parent->hv_key, old_index, node->hv_key, __vector_inter_get_count(node->hv_key) - 1, 1);

	/*
	* 分列生成的新节点加入数组
	*/
	__vector_inter_insert_at(parent->hv_sub, old_index + 1, node_new, 0);

	assert(__vector_inter_get_count(parent->hv_sub) <= btree->min_sub * 2);
	assert(__vector_inter_get_count(parent->hv_sub) >= btree->min_sub || parent == btree->root);
	assert(__vector_inter_get_count(parent->hv_key) + 1 == __vector_inter_get_count(parent->hv_sub));

	btree->index_count ++;

	return 0;
}


/**
 * @brief B树的创建
 * @param mini_sub:最小子节点的数目
 */
HMYBTREE MyBTreeConstruct(HMYMEMPOOL hm, ALG_COMPARE compare, size_t min_sub)
{
#define DEF_MIN_SUB 3

	mybtree_t * btree = (mybtree_t *)MyMemPoolMalloc(hm, sizeof(*btree));
	if(NULL == btree)
		return NULL;

	btree->hm = hm;
	btree->compare = compare;
	btree->context = NULL;
	btree->min_sub = min_sub;
	btree->index_count = 0;

	if(btree->min_sub <= DEF_MIN_SUB)
		btree->min_sub = DEF_MIN_SUB;

	btree->root = create_btree_node(btree);
	if(NULL == btree->root)
		goto MyBTreeConstruct_err_;

	return btree;

MyBTreeConstruct_err_:

	btree_destroy(btree);

	return NULL;

#undef DEF_MIN_SUB
}

/**
 * @brief B树的销毁
 */
void MyBTreeDestruct(HMYBTREE hbtree)
{
	if(NULL == hbtree)
		return;

	assert(hbtree->root);

	btree_destroy(hbtree);
}

/**
 * @brief 往B树中添加一个节点
 * @param index_info:要添加的索引信息
 * @return 0:成功 其它:失败
 */
int MyBTreeAdd(HMYBTREE hbtree, void * index_info)
{
	if(NULL == hbtree)
		return -1;

	assert(hbtree->root);

	return btree_add(hbtree, index_info);	
}

/**
 * @brief 从B树中删除一个节点
 * @param index_info:要删除的索引信息
 * @param index_info_out:返回存储在B树里的索引信息指针给用户
 * @return 0:成功 其它:失败
 */
int MyBTreeDel(HMYBTREE hbtree, void * index_info, void ** index_info_out)
{
	int bfind_key = 0;
	mybtree_node_t * current_node = NULL;

	/*
	* 采用自顶向下的做法
	* 在删除的过程中,沿途根据需要合并节点
	* 
	* 如果要删除的key在当前的节点当中
	* 1:如果key的前驱包含关键字数 >= min_sub,则将前驱的最大值补至key的位置, 反之如果是后继,则将最小值来补key,然后对相应的子节点进行递归key_new
	* 2:如果前驱后继都将近临界,则应合并前驱与后继,与key,然后对合并后的节点进行递归
	* 3.如果当前节点是叶节点,删除key即可
	* 
	* 如果要删除的key不在当前节点中
	* 找出key可能在的分支
	* 如果分支的子节点数大临界,则递归向下
	* 如果分支的子节点数小于临界值,
	*   如果存后一个大于min_sub的兄弟,则从兄弟中借,并移动相应的子分支
	*   如果左右兄弟都已经是临界
	*       则应将分支与其中一个兄弟合并,对合并后的节点进行递归
	*/

	if(NULL == hbtree)
		return -1;

	assert(hbtree->root);

	current_node = hbtree->root;

	while(1)
	{
		size_t index = 0;

		assert(current_node->hv_key);
		assert(__vector_inter_get_count(current_node->hv_key) + 1 == __vector_inter_get_count(current_node->hv_sub)
			|| current_node == hbtree->root
			|| __vector_inter_get_count(current_node->hv_sub) == 0);

		if(0 == btree_search(hbtree, current_node->hv_key, index_info, &index))
		{
			assert(index < __vector_inter_get_count(current_node->hv_sub) || __vector_inter_get_count(current_node->hv_sub) == 0);

			if(index_info_out && 0 == bfind_key)
			{
				bfind_key = 1;
				*index_info_out = __vector_inter_get_index_data(current_node->hv_key, index);

				if(hbtree->index_count)
					hbtree->index_count --;
			}

			if(__vector_inter_get_count(current_node->hv_sub) == 0)
			{
				/* 如果根节点有子分支,则删除不可能在根节点处发生 */
				assert((__vector_inter_get_count(hbtree->root->hv_sub) && hbtree->root != current_node) || 
					(0 == __vector_inter_get_count(hbtree->root->hv_sub) && hbtree->root == current_node));

				/*
				* 3.如果当前节点是叶节点,删除key即可
				*/
				__vector_inter_del(current_node->hv_key, index);

				assert(__vector_inter_get_count(current_node->hv_key) + 1 >= hbtree->min_sub || current_node == hbtree->root);

				return 0;
			}
			else
			{
				/*
				* 如果要删除的key在当前的节点当中
				* 1:如果key的前驱包含关键字数 >= min_sub,则将前驱的最大值补至key的位置, 反之如果是后继,则将最小值来补key,然后对相应的子节点进行递归key_new
				* 2:如果前驱后继都将近临界,则应合并前驱与后继,与key,然后对合并后的节点进行递归
				*/
				mybtree_node_t * last = (mybtree_node_t *)__vector_inter_get_index_data(current_node->hv_sub, index);
				mybtree_node_t * next = (mybtree_node_t *)__vector_inter_get_index_data(current_node->hv_sub, index + 1);

				assert(last && next);

				if(__vector_inter_get_count(last->hv_key) >= hbtree->min_sub)
				{
					mybtree_node_t * prev_node = last;
					while(__vector_inter_get_count(prev_node->hv_sub))
						prev_node = (mybtree_node_t *)__vector_get_tail_data(prev_node->hv_sub);

					assert(prev_node);

					index_info = __vector_get_tail_data(prev_node->hv_key);
					__vector_inter_update_index(current_node->hv_key, index, index_info, 0);

					/* 如果前驱包含的关键字数大于min_sub,可以直接从前驱中完成删除 */
					if(__vector_inter_get_count(prev_node->hv_key) >= hbtree->min_sub)
						current_node = prev_node;
					else/* 前驱的关键数小于min_sub需要重新递归删除 */
						current_node = last;

					assert(current_node);

					continue;
				}
				else if(__vector_inter_get_count(next->hv_key) >= hbtree->min_sub)
				{
					mybtree_node_t * sub_node = next;
					while(__vector_inter_get_count(sub_node->hv_sub))
						sub_node = (mybtree_node_t *)__vector_get_head_data(sub_node->hv_sub);

					assert(sub_node);

					index_info = __vector_get_head_data(sub_node->hv_key);
					__vector_inter_update_index(current_node->hv_key, index, index_info, 0);

					/* 如果后继包含的关键字数大于min_sub,可以直接从后继中完成删除 */
					if(__vector_inter_get_count(sub_node->hv_key) >= hbtree->min_sub)
						current_node = sub_node;
					else/* 后继的关键数小于min_sub需要重新递归删除 */
						current_node = next;

					assert(current_node);

					continue;
				}
				else
				{
					assert(last && next);

					__vector_move_range_to_end(last->hv_key, current_node->hv_key, index, 1);
					__vector_move_range_to_end(last->hv_key, next->hv_key, 0, __vector_inter_get_count(next->hv_key));

					if(__vector_inter_get_count(next->hv_sub))
						__vector_move_range_to_end(last->hv_sub, next->hv_sub, 0, __vector_inter_get_count(next->hv_sub));
					__vector_inter_del(current_node->hv_sub, index + 1);

					assert(current_node);
					assert(__vector_inter_get_count(current_node->hv_key) + 1 == __vector_inter_get_count(current_node->hv_sub));
					if(__vector_inter_get_count(current_node->hv_sub) <= 1)
					{
						assert(current_node == hbtree->root);

						assert(__vector_inter_get_count(current_node->hv_sub));
						assert(0 == __vector_inter_get_count(current_node->hv_key));
						assert(__vector_get_head_data(current_node->hv_sub) == last);

						hbtree->root = last;
						destroy_btree_node(hbtree, current_node);
					}

					destroy_btree_node(hbtree, next);

					assert(__vector_inter_get_count(last->hv_key) + 1 == __vector_inter_get_count(last->hv_sub) || 0 == __vector_inter_get_count(last->hv_sub));

					current_node = last;

					continue;
				}
			}
		}
		else
		{
			mybtree_node_t * sub = NULL;

			if(__vector_inter_get_count(current_node->hv_sub) == 0)
				return -1;

			assert(__vector_inter_get_count(current_node->hv_key) + 1 >= hbtree->min_sub || current_node == hbtree->root);
			assert(__vector_inter_get_count(current_node->hv_key) + 1 == __vector_inter_get_count(current_node->hv_sub));
			assert(index < __vector_inter_get_count(current_node->hv_sub));

			sub = (mybtree_node_t *)__vector_inter_get_index_data(current_node->hv_sub, index);

			/*
			* 如果要删除的key不在当前节点中
			* 找出key可能在的分支
			*/
			if(__vector_inter_get_count(sub->hv_key) >= hbtree->min_sub)
			{
				/*
				* 如果分支的子节点数大临界,则递归向下
				*/
				current_node = (mybtree_node_t *)__vector_inter_get_index_data(current_node->hv_sub, index);
				continue;
			}
			else
			{
				/*
				* 如果分支的子节点数小于临界值,
				*   如果存后一个大于min_sub的兄弟,则从兄弟中借,并移动相应的子分支
				*   如果左右兄弟都已经是临界
				*       则应将分支与其中一个兄弟合并,对合并后的节点进行递归
				*/
				mybtree_node_t * prev = NULL;
				mybtree_node_t * next = NULL;
				
				if(index)
					prev = (mybtree_node_t *)__vector_inter_get_index_data(current_node->hv_sub, index - 1);

				if(index + 1 < __vector_inter_get_count(current_node->hv_sub))
					next = (mybtree_node_t *)__vector_inter_get_index_data(current_node->hv_sub, index + 1);

				/* 至少一个key,两个子分支 */
				assert(prev || next);

				if(prev && __vector_inter_get_count(prev->hv_key) >= hbtree->min_sub)
				{
					void * key_ii = __vector_inter_get_index_data(current_node->hv_key, index - 1);
					void * min_key_ii = __vector_get_tail_data(prev->hv_key);

					assert(index);

					__vector_inter_insert_at(sub->hv_key, 0, key_ii, 0);
					__vector_inter_update_index(current_node->hv_key, index - 1, min_key_ii, 0);
					__vector_inter_del(prev->hv_key, __vector_inter_get_count(prev->hv_key) - 1); 

					if(__vector_inter_get_count(prev->hv_sub))
						__vector_move_range_to_index(sub->hv_sub, 0, prev->hv_sub, __vector_inter_get_count(prev->hv_sub) - 1, 1);

					assert(__vector_inter_get_count(sub->hv_key) + 1 == __vector_inter_get_count(sub->hv_sub) || 0 == __vector_inter_get_count(sub->hv_sub));
					assert(__vector_inter_get_count(prev->hv_key) + 1 == __vector_inter_get_count(prev->hv_sub) || 0 == __vector_inter_get_count(prev->hv_sub));
					assert(__vector_inter_get_count(current_node->hv_key) + 1 == __vector_inter_get_count(current_node->hv_sub));
					assert(__vector_inter_get_count(sub->hv_sub) >= hbtree->min_sub + 1 || 0 == __vector_inter_get_count(sub->hv_sub));
					assert(__vector_inter_get_count(prev->hv_sub) >= hbtree->min_sub || 0 == __vector_inter_get_count(prev->hv_sub));
					assert(__vector_inter_get_count(current_node->hv_sub) >= hbtree->min_sub || current_node == hbtree->root);

					assert(0 == MyAlgSortOK(__vector_inter_get_array((sub->hv_key)), __vector_inter_get_count((sub->hv_key)), __vector_inter_get_array_step_size((sub->hv_key)), btree_compare, hbtree));
					assert(0 == MyAlgSortOK(__vector_inter_get_array((prev->hv_key)), __vector_inter_get_count((prev->hv_key)), __vector_inter_get_array_step_size((prev->hv_key)), btree_compare, hbtree));
					assert(0 == MyAlgSortOK(__vector_inter_get_array((current_node->hv_key)), __vector_inter_get_count((current_node->hv_key)), __vector_inter_get_array_step_size((current_node->hv_key)), btree_compare, hbtree));

					current_node = sub;
					continue;
				}
				else if(next && __vector_inter_get_count(next->hv_key) >= hbtree->min_sub)
				{
					void * key_ii = __vector_inter_get_index_data(current_node->hv_key, index);
					void * max_key_ii = __vector_get_head_data(next->hv_key);

					__vector_inter_add(sub->hv_key, key_ii, 0);
					__vector_inter_update_index(current_node->hv_key, index, max_key_ii, 0);
					__vector_inter_del(next->hv_key, 0); 

					if(__vector_inter_get_count(next->hv_sub))
						__vector_move_range_to_end(sub->hv_sub, next->hv_sub, 0, 1);

					assert(__vector_inter_get_count(sub->hv_key) + 1 == __vector_inter_get_count(sub->hv_sub) || 0 == __vector_inter_get_count(sub->hv_sub));
					assert(__vector_inter_get_count(next->hv_key) + 1 == __vector_inter_get_count(next->hv_sub) || 0 == __vector_inter_get_count(next->hv_sub));
					assert(__vector_inter_get_count(current_node->hv_key) + 1 == __vector_inter_get_count(current_node->hv_sub));
					assert(__vector_inter_get_count(sub->hv_sub) >= hbtree->min_sub + 1 || 0 == __vector_inter_get_count(sub->hv_sub));
					assert(__vector_inter_get_count(next->hv_sub) >= hbtree->min_sub || 0 == __vector_inter_get_count(next->hv_sub));
					assert(__vector_inter_get_count(current_node->hv_sub) >= hbtree->min_sub || current_node == hbtree->root);

					assert(0 == MyAlgSortOK(__vector_inter_get_array((sub->hv_key)), __vector_inter_get_count((sub->hv_key)), __vector_inter_get_array_step_size((sub->hv_key)), btree_compare, hbtree));
					assert(0 == MyAlgSortOK(__vector_inter_get_array((next->hv_key)), __vector_inter_get_count((next->hv_key)), __vector_inter_get_array_step_size((next->hv_key)), btree_compare, hbtree));
					assert(0 == MyAlgSortOK(__vector_inter_get_array((current_node->hv_key)), __vector_inter_get_count((current_node->hv_key)), __vector_inter_get_array_step_size((current_node->hv_key)), btree_compare, hbtree));

					current_node = sub;
					continue;
				}
				else
				{
					assert(__vector_inter_get_count(current_node->hv_sub));

					if(prev)
					{
						assert(index);

						__vector_move_range_to_end(prev->hv_key, current_node->hv_key, index - 1, 1);
						__vector_move_range_to_end(prev->hv_key, sub->hv_key, 0, __vector_inter_get_count(sub->hv_key));

						if(__vector_inter_get_count(sub->hv_sub))
							__vector_move_range_to_end(prev->hv_sub, sub->hv_sub, 0, __vector_inter_get_count(sub->hv_sub));
						__vector_inter_del(current_node->hv_sub, index);

						assert(__vector_inter_get_count(current_node->hv_sub) >= hbtree->min_sub || current_node == hbtree->root);

						assert(current_node);
						if(__vector_inter_get_count(current_node->hv_sub) <= 1)
						{
							assert(current_node == hbtree->root);

							assert(__vector_inter_get_count(current_node->hv_sub));
							assert(0 == __vector_inter_get_count(current_node->hv_key));
							assert(__vector_get_head_data(current_node->hv_sub) == prev);

							hbtree->root = prev;
							destroy_btree_node(hbtree, current_node);
						}

						destroy_btree_node(hbtree, sub);

						current_node = prev;
						continue;
					}
					else if(next)
					{
						assert(index + 1 < __vector_inter_get_count(current_node->hv_sub));

						__vector_move_range_to_end(sub->hv_key, current_node->hv_key, index, 1);
						__vector_move_range_to_end(sub->hv_key, next->hv_key, 0, __vector_inter_get_count(next->hv_key));

						if(__vector_inter_get_count(next->hv_sub))
							__vector_move_range_to_end(sub->hv_sub, next->hv_sub, 0, __vector_inter_get_count(next->hv_sub));
						__vector_inter_del(current_node->hv_sub, index + 1);

						assert(__vector_inter_get_count(current_node->hv_sub) >= hbtree->min_sub || current_node == hbtree->root);

						assert(current_node);
						if(__vector_inter_get_count(current_node->hv_sub) <= 1)
						{
							assert(current_node == hbtree->root);

							assert(__vector_inter_get_count(current_node->hv_sub));
							assert(0 == __vector_inter_get_count(current_node->hv_key));
							assert(__vector_get_head_data(current_node->hv_sub) == sub);

							hbtree->root = sub;
							destroy_btree_node(hbtree, current_node);
						}

						destroy_btree_node(hbtree, next);

						current_node = sub;
						continue;
					}
					else
						assert(0);//bug here,至少一个key,两个子分支
				}
			}
		}
	}

	return 0;
}

/**
 * @brief 在B树中查找
 * @param index_info:要查找的索引信息
 * @param index_info_out:返回存储在B树里的索引信息指针给用户
 * @return 0:成功 其它:失败
 */
int MyBTreeSearch(HMYBTREE hbtree, const void * index_info, void ** index_info_out)
{
	size_t index = 0;
	mybtree_node_t * node = NULL;

	if(NULL == hbtree)
		return -1;

	node = search_btree(hbtree, index_info, NULL, &index);

	if(NULL == node)
		return -1;

	if(index_info_out)
		*index_info_out = __vector_inter_get_index_data(node->hv_key, index);

	return 0;
}

/**
 * @brief 获取B树中节点的个数
 */
size_t MyBTreeGetCount(HMYBTREE hbtree)
{
	if(NULL == hbtree)
		return 0;

	return hbtree->index_count;
}

/**
 * @brief 递归计算B树中的节点个数
 */
static size_t btree_cal_count(mybtree_t * btree, mybtree_node_t * node)
{
	size_t i = 0;
	size_t count = 0;

	assert(btree && node);

	count += __vector_inter_get_count(node->hv_key);

	for(i = 0; i < __vector_inter_get_count(node->hv_sub); i ++)
	{
		mybtree_node_t * sub = (mybtree_node_t *)__vector_inter_get_index_data(node->hv_sub, i);

		count += btree_cal_count(btree, sub);
	}

	return count;
}

/**
 * @brief 递归计算B树中的节点个数
 */
size_t MyBTreeCalCount(HMYBTREE hbtree)
{
	assert(hbtree);

	return btree_cal_count(hbtree, hbtree->root);
}


/**
 * @brief 检查一棵B树是否合法
 */
static int btree_examin(mybtree_t * btree, mybtree_node_t * node, 
						mybtree_node_t * next_thread_node, size_t min_node_index, size_t max_node_index,
						int layer, int look_printf)
{
	int sub_layer = 0;
	int current_layer = layer + 1;
	size_t i = 0;

	assert(btree && node);

	/* 关键字的个数必须符合要求 */
	assert(__vector_inter_get_count(node->hv_key) + 1 <= 2 * btree->min_sub);
	if(node != btree->root)
		assert(__vector_inter_get_count(node->hv_key) >= btree->min_sub - 1);
	assert((__vector_inter_get_count(node->hv_sub) == __vector_inter_get_count(node->hv_key) + 1) ||
		__vector_inter_get_count(node->hv_sub) == 0);

	if(look_printf)
	{
#ifdef _MBCSV6
		printf("%d %x", current_layer, node);
#else
#ifdef WIN32
		printf("%d %x", current_layer, (long long)node); 
#else
		printf("%d %x", current_layer, node);
#endif
#endif
		MyVectorPrint(node->hv_key);
	}

	/* 关键字必须有序 */
	assert(0 == MyAlgSortOK(__vector_inter_get_array((node->hv_key)),
		__vector_inter_get_count((node->hv_key)),
		__vector_inter_get_array_step_size((node->hv_key)),
		btree_compare, btree));

	for(i = 0; i < __vector_inter_get_count(node->hv_sub); i ++)
	{
		int current_sub_layer = 0;
		mybtree_node_t * sub = (mybtree_node_t *)__vector_inter_get_index_data(node->hv_sub, i);

		assert(sub);

		if(look_printf)
		{
#ifdef _MBCSV6
			printf("[%s:%d]%x ", __FILE__, __LINE__, node);
			printf("%x ", sub);
#else
#ifdef WIN32
			printf("[%s:%d]%x ", __FILE__, __LINE__, (long long)node);
			printf("%x ", (long long)sub);
#else
			printf("[%s:%d]%x ", __FILE__, __LINE__, node);
			printf("%x ", sub);
#endif
#endif
			printf("layer:%d sub:%d ", current_layer,i);
			MyVectorPrint(sub->hv_key);
		}

		assert(0 == MyAlgSortOK(__vector_inter_get_array((sub->hv_key)),
			__vector_inter_get_count((sub->hv_key)),
			__vector_inter_get_array_step_size((sub->hv_key)),
			btree_compare, btree));

		/* 每个分支的最大值与最小值必须满足b树的定义 */
		if(i + 1 < __vector_inter_get_count(node->hv_sub))
		{
			void * max_parent_key = __vector_inter_get_index_data(node->hv_key, i);
			void * max_key = __vector_get_tail_data(sub->hv_key);
			assert(btree->compare(max_key, max_parent_key, btree->context) < 0);
		}
		else if(next_thread_node)
		{
			/* 最大值限制 */
			void * branch_max_key = __vector_inter_get_index_data(next_thread_node->hv_key, max_node_index);
			void * max_key = __vector_get_tail_data(sub->hv_key);
			assert(btree->compare(max_key, branch_max_key, btree->context) < 0);
		}

		if(i)
		{
			void * min_parent_key = __vector_inter_get_index_data(node->hv_key, i - 1);
			void * min_key = __vector_get_head_data(sub->hv_key);
			assert(btree->compare(min_key, min_parent_key, btree->context) > 0);
		}
		else if(next_thread_node)
		{
			/* 最小值限制 */
			void * branch_min_key = __vector_inter_get_index_data(next_thread_node->hv_key, min_node_index);
			void * min_key = __vector_get_head_data(sub->hv_key);
			assert(btree->compare(min_key, branch_min_key, btree->context) > 0);
		}

		if(0 == i || i + 1 == __vector_inter_get_count(node->hv_sub))
			current_sub_layer = btree_examin(btree, sub, next_thread_node, min_node_index, max_node_index, current_layer, look_printf);
		else
			current_sub_layer = btree_examin(btree, sub, node, i - 1, i, current_layer, look_printf);

		if(i)
			assert(sub_layer == current_sub_layer);/* 每个分支的层数必须相同 */
		else
			sub_layer = current_sub_layer;
	}

	return current_layer + sub_layer;
}

/**
 * @brief 检查一棵B树是否合法
 */
int MyBTreeExamin(HMYBTREE hbtree, int look_printf)
{

	if(look_printf)
		printf("\n[%s:%d] MyBTreeExamin\r\n", __FILE__, __LINE__);

	assert(hbtree);
	assert(hbtree->root);

	return btree_examin(hbtree, hbtree->root, NULL, 0, 0, 0, look_printf);
}



























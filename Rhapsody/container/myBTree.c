/**
 *
 * @file myBTree.c B��
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief
 * ���ڵ�����Ƿ�Ҷ�ӽڵ�,����������ӽڵ�.
 *
 * һ��m�׵�B�� ÿ����Ҷ�ӽڵ㺬��n(m/2 �� m-1)���ؼ��� ����n+1������
 * ���е�Ҷ�ӽڵ�Ϊ��
 * ÿ���ڵ�Ĺؼ���˳������
 *
 * ���� key(P1) >= k1  && key(P2) < k2
 *
 *
 * ��ӽڵ�ʱ���ڵײ���ӣ�
 * ������ˣ�����Ѹýڵ㣬�����м�Ĺؼ���������,
 * �ݹ�ֱ�����ڵ�,
 * ������ڵ�Ҳ���ˣ���ʱ��������һ��.
 *
 *
 * ɾ���ڵ�ʱ.����ʹ���Զ����µķ�ʾ��ʵ��.
 * �Ӹ��ڵ㿪ʼ��ѰҪɾ����key_del,
 * ȷ��key_del���ڵķ�֧�������֧�Ĺؼ��ָ�������t(��С����),
 * ����кϲ��������ֵܾ��в��������������߽裨�����ֵ���һ��������
 *
 * ����ҵ�key_del,��ǰ�������ߺ�����key_del,�ݹ�ɾ��ǰ��/���
 *
 * ��˵ݹ�ֱ��Ҷ�ڵ�
 *
 * todo:B��ʵ��Ӧ�����븨�����ϵ�.
 */
#include "myBTree.h"

#include <assert.h>
#include <stdio.h>

#include "myvector.h"
#include "__vector_inter.h"


typedef struct __mybtree_node_t_
{
	/*
	* �ؼ�������
	*/
	HMYVECTOR hv_key;

	/*
	* �ӽڵ�����
	*/
	HMYVECTOR hv_sub;
}mybtree_node_t;

typedef struct __mybtree_t_
{
	/* ���ڵ� */
	mybtree_node_t * root;

	/* �ڴ�� */
	HMYMEMPOOL hm;

	/* �ڵ����С�ӽڵ���� */
	size_t min_sub;

	/*
	* �Ƚϻص�,�뱣���ֶ�
	*/
	ALG_COMPARE compare;
	void * context;

	/* ��¼�� */
	size_t index_count;
}mybtree_t;


/**
 * T���Ƚ�
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
 * �����������
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
 * ����һ��b���ڵ�
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
 * ���B��
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
 * ����B��
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
 * ����B��
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
 * ����һ��b���ڵ�
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
 * ��ӽڵ�
 */
static __INLINE__ int btree_add(mybtree_t * btree, void * index_info)
{
	/*
	* ʹ���Զ����µķ�����ӽڵ�,
	* ��ӱ�Ȼ����"Ҷ�ڵ�"������
	* �ڲ�����ӽڵ�Ĺ�����,��;���η��ѽڵ�
	* ��Ҷ�ڵ����,������Ҫ����Ҷ�ڵ�
	*  
	* �Զ����µ�ȱ��:
	* ʹ���Զ����µķ���,��������ɾ���ǽ�����е�,
	* ���п������һ���ڵ���Ϊ��ӷ���֮��,����Ϊɾ�����ϲ�
	*
	* ���淨:
	* 1: ���Ҷ�ڵ�δ��,ֱ�����
	* 2: ���Ҷ�ڵ�����,����,���м�ؼ��ּ��븸�ڵ���ʵ�λ��
	* 3: ������ڵ�����,�ݹ�����
	* ��˷��ȱ��:���Ѹ��¸��ڵ�����,�Լ�ÿ���ӽڵ�ĸ��׵��������ļ���
	*
	* ����Ĵ����������Զ����µ����һ���ڵ���B���еĹ���
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

		/* �����ٽ��,Ӧ������ */
		node_new = create_btree_node(btree);
		if(NULL == node_new)
			return -1;

		__vector_move_range_to_end(node_new->hv_key, node->hv_key, btree->min_sub, btree->min_sub - 1);

		assert(btree->min_sub == __vector_inter_get_count(node_new->hv_key) + 1);
		assert(btree->min_sub == __vector_inter_get_count(node->hv_key));

		/*
		* ����Ǹ��ڵ�,����Ҫ�������ڵ�
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
			* �������ڷ�֧,�������ڵ��Լ���Ӧ���±�
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
		* �ҵ����ʵ�"Ҷ�ڵ�" 
		* �����ǰҶ�ڵ�û�������� 
		*/
		__vector_inter_insert_at(node->hv_key, index, index_info, 0);

		btree->index_count ++;

		return 0;
	}

	/* �����ǰҶ�ڵ������� */
	node_new = create_btree_node(btree);
	if(NULL == node_new)
		return -1;

	__vector_inter_insert_at(node->hv_key, index, index_info, 0);

	/*
	* �ڵ����
	* ǰt���ڵ� ��t-1���ڵ��Ϊ���Ѻ�Ľڵ�, �м���Ǹ��ڵ���븸�ڵ�
	*/
	__vector_move_range_to_end(node_new->hv_key, node->hv_key, btree->min_sub + 1, btree->min_sub - 1);
	assert(btree->min_sub - 1 == __vector_inter_get_count(node_new->hv_key));

	/*
	* ����Ǹ��ڵ�,����Ҫ�������ڵ�
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
	* �������ɵ��½ڵ��������
	*/
	__vector_inter_insert_at(parent->hv_sub, old_index + 1, node_new, 0);

	assert(__vector_inter_get_count(parent->hv_sub) <= btree->min_sub * 2);
	assert(__vector_inter_get_count(parent->hv_sub) >= btree->min_sub || parent == btree->root);
	assert(__vector_inter_get_count(parent->hv_key) + 1 == __vector_inter_get_count(parent->hv_sub));

	btree->index_count ++;

	return 0;
}


/**
 * @brief B���Ĵ���
 * @param mini_sub:��С�ӽڵ����Ŀ
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
 * @brief B��������
 */
void MyBTreeDestruct(HMYBTREE hbtree)
{
	if(NULL == hbtree)
		return;

	assert(hbtree->root);

	btree_destroy(hbtree);
}

/**
 * @brief ��B�������һ���ڵ�
 * @param index_info:Ҫ��ӵ�������Ϣ
 * @return 0:�ɹ� ����:ʧ��
 */
int MyBTreeAdd(HMYBTREE hbtree, void * index_info)
{
	if(NULL == hbtree)
		return -1;

	assert(hbtree->root);

	return btree_add(hbtree, index_info);	
}

/**
 * @brief ��B����ɾ��һ���ڵ�
 * @param index_info:Ҫɾ����������Ϣ
 * @param index_info_out:���ش洢��B�����������Ϣָ����û�
 * @return 0:�ɹ� ����:ʧ��
 */
int MyBTreeDel(HMYBTREE hbtree, void * index_info, void ** index_info_out)
{
	int bfind_key = 0;
	mybtree_node_t * current_node = NULL;

	/*
	* �����Զ����µ�����
	* ��ɾ���Ĺ�����,��;������Ҫ�ϲ��ڵ�
	* 
	* ���Ҫɾ����key�ڵ�ǰ�Ľڵ㵱��
	* 1:���key��ǰ�������ؼ����� >= min_sub,��ǰ�������ֵ����key��λ��, ��֮����Ǻ��,����Сֵ����key,Ȼ�����Ӧ���ӽڵ���еݹ�key_new
	* 2:���ǰ����̶������ٽ�,��Ӧ�ϲ�ǰ������,��key,Ȼ��Ժϲ���Ľڵ���еݹ�
	* 3.�����ǰ�ڵ���Ҷ�ڵ�,ɾ��key����
	* 
	* ���Ҫɾ����key���ڵ�ǰ�ڵ���
	* �ҳ�key�����ڵķ�֧
	* �����֧���ӽڵ������ٽ�,��ݹ�����
	* �����֧���ӽڵ���С���ٽ�ֵ,
	*   ������һ������min_sub���ֵ�,����ֵ��н�,���ƶ���Ӧ���ӷ�֧
	*   ��������ֵܶ��Ѿ����ٽ�
	*       ��Ӧ����֧������һ���ֵܺϲ�,�Ժϲ���Ľڵ���еݹ�
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
				/* ������ڵ����ӷ�֧,��ɾ���������ڸ��ڵ㴦���� */
				assert((__vector_inter_get_count(hbtree->root->hv_sub) && hbtree->root != current_node) || 
					(0 == __vector_inter_get_count(hbtree->root->hv_sub) && hbtree->root == current_node));

				/*
				* 3.�����ǰ�ڵ���Ҷ�ڵ�,ɾ��key����
				*/
				__vector_inter_del(current_node->hv_key, index);

				assert(__vector_inter_get_count(current_node->hv_key) + 1 >= hbtree->min_sub || current_node == hbtree->root);

				return 0;
			}
			else
			{
				/*
				* ���Ҫɾ����key�ڵ�ǰ�Ľڵ㵱��
				* 1:���key��ǰ�������ؼ����� >= min_sub,��ǰ�������ֵ����key��λ��, ��֮����Ǻ��,����Сֵ����key,Ȼ�����Ӧ���ӽڵ���еݹ�key_new
				* 2:���ǰ����̶������ٽ�,��Ӧ�ϲ�ǰ������,��key,Ȼ��Ժϲ���Ľڵ���еݹ�
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

					/* ���ǰ�������Ĺؼ���������min_sub,����ֱ�Ӵ�ǰ�������ɾ�� */
					if(__vector_inter_get_count(prev_node->hv_key) >= hbtree->min_sub)
						current_node = prev_node;
					else/* ǰ���Ĺؼ���С��min_sub��Ҫ���µݹ�ɾ�� */
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

					/* �����̰����Ĺؼ���������min_sub,����ֱ�ӴӺ�������ɾ�� */
					if(__vector_inter_get_count(sub_node->hv_key) >= hbtree->min_sub)
						current_node = sub_node;
					else/* ��̵Ĺؼ���С��min_sub��Ҫ���µݹ�ɾ�� */
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
			* ���Ҫɾ����key���ڵ�ǰ�ڵ���
			* �ҳ�key�����ڵķ�֧
			*/
			if(__vector_inter_get_count(sub->hv_key) >= hbtree->min_sub)
			{
				/*
				* �����֧���ӽڵ������ٽ�,��ݹ�����
				*/
				current_node = (mybtree_node_t *)__vector_inter_get_index_data(current_node->hv_sub, index);
				continue;
			}
			else
			{
				/*
				* �����֧���ӽڵ���С���ٽ�ֵ,
				*   ������һ������min_sub���ֵ�,����ֵ��н�,���ƶ���Ӧ���ӷ�֧
				*   ��������ֵܶ��Ѿ����ٽ�
				*       ��Ӧ����֧������һ���ֵܺϲ�,�Ժϲ���Ľڵ���еݹ�
				*/
				mybtree_node_t * prev = NULL;
				mybtree_node_t * next = NULL;
				
				if(index)
					prev = (mybtree_node_t *)__vector_inter_get_index_data(current_node->hv_sub, index - 1);

				if(index + 1 < __vector_inter_get_count(current_node->hv_sub))
					next = (mybtree_node_t *)__vector_inter_get_index_data(current_node->hv_sub, index + 1);

				/* ����һ��key,�����ӷ�֧ */
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
						assert(0);//bug here,����һ��key,�����ӷ�֧
				}
			}
		}
	}

	return 0;
}

/**
 * @brief ��B���в���
 * @param index_info:Ҫ���ҵ�������Ϣ
 * @param index_info_out:���ش洢��B�����������Ϣָ����û�
 * @return 0:�ɹ� ����:ʧ��
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
 * @brief ��ȡB���нڵ�ĸ���
 */
size_t MyBTreeGetCount(HMYBTREE hbtree)
{
	if(NULL == hbtree)
		return 0;

	return hbtree->index_count;
}

/**
 * @brief �ݹ����B���еĽڵ����
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
 * @brief �ݹ����B���еĽڵ����
 */
size_t MyBTreeCalCount(HMYBTREE hbtree)
{
	assert(hbtree);

	return btree_cal_count(hbtree, hbtree->root);
}


/**
 * @brief ���һ��B���Ƿ�Ϸ�
 */
static int btree_examin(mybtree_t * btree, mybtree_node_t * node, 
						mybtree_node_t * next_thread_node, size_t min_node_index, size_t max_node_index,
						int layer, int look_printf)
{
	int sub_layer = 0;
	int current_layer = layer + 1;
	size_t i = 0;

	assert(btree && node);

	/* �ؼ��ֵĸ����������Ҫ�� */
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

	/* �ؼ��ֱ������� */
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

		/* ÿ����֧�����ֵ����Сֵ��������b���Ķ��� */
		if(i + 1 < __vector_inter_get_count(node->hv_sub))
		{
			void * max_parent_key = __vector_inter_get_index_data(node->hv_key, i);
			void * max_key = __vector_get_tail_data(sub->hv_key);
			assert(btree->compare(max_key, max_parent_key, btree->context) < 0);
		}
		else if(next_thread_node)
		{
			/* ���ֵ���� */
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
			/* ��Сֵ���� */
			void * branch_min_key = __vector_inter_get_index_data(next_thread_node->hv_key, min_node_index);
			void * min_key = __vector_get_head_data(sub->hv_key);
			assert(btree->compare(min_key, branch_min_key, btree->context) > 0);
		}

		if(0 == i || i + 1 == __vector_inter_get_count(node->hv_sub))
			current_sub_layer = btree_examin(btree, sub, next_thread_node, min_node_index, max_node_index, current_layer, look_printf);
		else
			current_sub_layer = btree_examin(btree, sub, node, i - 1, i, current_layer, look_printf);

		if(i)
			assert(sub_layer == current_sub_layer);/* ÿ����֧�Ĳ���������ͬ */
		else
			sub_layer = current_sub_layer;
	}

	return current_layer + sub_layer;
}

/**
 * @brief ���һ��B���Ƿ�Ϸ�
 */
int MyBTreeExamin(HMYBTREE hbtree, int look_printf)
{

	if(look_printf)
		printf("\n[%s:%d] MyBTreeExamin\r\n", __FILE__, __LINE__);

	assert(hbtree);
	assert(hbtree->root);

	return btree_examin(hbtree, hbtree->root, NULL, 0, 0, 0, look_printf);
}



























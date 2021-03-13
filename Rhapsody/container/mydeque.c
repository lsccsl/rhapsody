/*
*
* mydeque.c 双向增长队列 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/


#include "mydeque.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>


#include "myutility.h"


typedef struct __mydeque_node_t_
{
	void * data;
	size_t data_size;
}mydeque_node_t;

typedef struct __mydeque_buffer_t_
{
	//当前缓冲的起始与长度
	mydeque_node_t ** first;
	size_t len;
}mydeque_buffer_t;

typedef struct __mydeque_iter_t_
{
	//指向map对应的"下标位置"
	size_t map_index;

	//当前"指针"实际所应指的位置,
	size_t node_index;
}mydeque_iter_t;

typedef struct __mydeque_t_
{
	HMYMEMPOOL hm;
	myobj_ops data_ops;

	mydeque_buffer_t ** map;
	size_t map_size;

	size_t buffer_size;

	mydeque_iter_t first;
	mydeque_iter_t last;

	//记录队列最小的头索引与最大的尾索引,用于释放时使用
	size_t min_head;
	size_t max_tail;

	size_t element_count;
}mydeque_t;


#define CAL_MAP_SIZE(x) ((x) * sizeof(mydeque_buffer_t *))
#define CAL_BUFFER_SIZE(x) ((x) * sizeof(mydeque_node_t *))
#define CAL_MAP_START(s, len) ( ((s) - (len)) / 2 )
#define CAL_MAP_HALF(s) (((s) - 1)/ 2)

static __INLINE__ mydeque_node_t * deque_inter_create_node(mydeque_t * dq, const void * data, const size_t data_size)
{
	mydeque_node_t * node = NULL;
	assert(dq);

	if(data && data_size)
	{
		node = (mydeque_node_t *)MyMemPoolMalloc(dq->hm, sizeof(*node) + data_size);
		if(NULL == node)
			return NULL;

		node->data = (char *)node + sizeof(*node);
		node->data_size = data_size;

		if(dq->data_ops.construct)
			dq->data_ops.construct(node->data, node->data_size, NULL, 0);

		if(dq->data_ops.copy)
			dq->data_ops.copy(node->data, node->data_size, data, data_size);
		else
			memcpy(node->data, data, node->data_size);
	}
	else
	{
		node = (mydeque_node_t *)MyMemPoolMalloc(dq->hm, sizeof(*node));
		if(NULL == node)
			return NULL;

		node->data = (void *)data;
		node->data_size = data_size;
	}

	return node;
}

static __INLINE__ void deque_inter_destroy_node(mydeque_t * dq, mydeque_node_t * node)
{
	assert(dq && node);

	if(node->data && node->data_size && dq->data_ops.destruct)
		dq->data_ops.destruct(node->data, node->data_size);

	MyMemPoolFree(dq->hm, node);
}

static __INLINE__ int deque_expand_map(mydeque_t * dq)
{
	mydeque_buffer_t ** new_map = NULL;
	size_t new_size = 0;
	size_t start = 0;
	size_t map_max_len = 0;
	size_t map_current_len = 0;

	assert(dq && dq->map_size);
	assert((dq->last.map_index - dq->first.map_index) < dq->map_size);

	//分配新的map
	new_size = dq->map_size * 2;
	new_map = (mydeque_buffer_t **)MyMemPoolMalloc(dq->hm, CAL_MAP_SIZE(new_size));
	if(NULL == new_map)
		return -1;

	memset(new_map, 0, CAL_MAP_SIZE(new_size));

	//拷贝原有的map至中间
	map_max_len = dq->max_tail - dq->min_head + 1;
	start = CAL_MAP_START(new_size, map_max_len);


	//重新计算first last
	map_current_len = dq->last.map_index - dq->first.map_index + 1;
	dq->first.map_index = start + (dq->first.map_index - dq->min_head);
	dq->last.map_index = dq->first.map_index + map_current_len - 1;

	if(dq->min_head > dq->first.map_index)
		dq->min_head = dq->first.map_index;
	if(dq->max_tail < dq->last.map_index)
		dq->max_tail = dq->last.map_index;

	memcpy(new_map + start, dq->map + dq->min_head, CAL_MAP_SIZE(map_max_len));

	//释放原有的map
	MyMemPoolFree(dq->hm, dq->map);

	dq->map = new_map;
	dq->map_size = new_size;

	return 0;
}

static __INLINE__ mydeque_buffer_t * deque_inter_create_buffer(mydeque_t * dq)
{
	mydeque_buffer_t * b = NULL;
	size_t len = 0;

	assert(dq);

	len = CAL_BUFFER_SIZE(dq->buffer_size) + sizeof(mydeque_buffer_t);
	b = (mydeque_buffer_t *)MyMemPoolMalloc(dq->hm, len);
	if(NULL == b)
		return NULL;

	memset(b, 0, len);;

	b->first = (mydeque_node_t **)((char*)b + sizeof(mydeque_buffer_t));
	b->len = dq->buffer_size;

	return b;
}

static __INLINE__ void deque_inter_reset_pointer(mydeque_t * dq)
{
	assert(dq);

	dq->first.map_index = dq->last.map_index = CAL_MAP_HALF(dq->map_size);
	assert(dq->map[dq->first.map_index]);

	dq->first.node_index = dq->last.node_index = dq->map[dq->first.map_index]->len / 2;
}

static __INLINE__ int deque_inter_del_head(mydeque_t * dq)
{
	assert(dq);

	if(dq->first.map_index == dq->last.map_index && dq->first.node_index == dq->last.node_index)
		return -1;

	assert(NULL != dq->map[dq->first.map_index]);
	assert(NULL != dq->map[dq->first.map_index]->first[dq->first.node_index]);
	
	deque_inter_destroy_node(dq, dq->map[dq->first.map_index]->first[dq->first.node_index]);
	dq->element_count --;

	dq->first.node_index ++;
	if(dq->first.map_index == dq->last.map_index && dq->first.node_index >= dq->last.node_index)
		deque_inter_reset_pointer(dq);
	else if(dq->map[dq->first.map_index]->len <= dq->first.node_index)
	{
		dq->first.map_index ++;
		dq->first.node_index = 0;
	}

	assert(dq->first.map_index < dq->map_size);
	assert(dq->map[dq->first.map_index]);
	assert(dq->first.node_index < dq->map[dq->first.map_index]->len);

	return 0;
}

static __INLINE__ int deque_inter_del_tail(mydeque_t * dq)
{
	assert(dq);

	if(dq->first.map_index == dq->last.map_index && dq->first.node_index == dq->last.node_index)
		return -1;

	if(0 == dq->last.node_index)
	{
		dq->last.map_index --;
		dq->last.node_index = dq->map[dq->last.map_index]->len - 1;
	}
	else
		dq->last.node_index --;

	assert(NULL != dq->map[dq->last.map_index]);
	assert(NULL != dq->map[dq->last.map_index]->first[dq->last.node_index]);

	deque_inter_destroy_node(dq, dq->map[dq->last.map_index]->first[dq->last.node_index]);
	dq->element_count --;

	if(dq->first.map_index == dq->last.map_index && dq->first.node_index == dq->last.node_index)
		deque_inter_reset_pointer(dq);

	assert(dq->last.map_index < dq->map_size);
	assert(dq->map[dq->last.map_index]);
	assert(dq->last.node_index < dq->map[dq->last.map_index]->len);

	return 0;
}

static __INLINE__ void deque_inter_destroy(mydeque_t * dq)
{
	size_t i = 0;

	assert(dq);
	assert(dq->max_tail < dq->map_size);

	while(0 == deque_inter_del_head(dq)){}

	for(i = dq->min_head; i <= dq->max_tail; i ++)
	{
		if(dq->map[i])
			MyMemPoolFree(dq->hm, dq->map[i]);
	}

	MyMemPoolFree(dq->hm, dq->map);
	MyMemPoolFree(dq->hm, dq);
}


/*
*
*构造
*
*/
HMYDEQUE MyDequeConstruct(HMYMEMPOOL hm, const myobj_ops * data_ops, const size_t buffer_size, const size_t map_size)
{
#define DEF_BUFFER_SIZE 512
#define DEF_MAP_SIZE 8

	mydeque_t * dq = (mydeque_t *)MyMemPoolMalloc(hm, sizeof(*dq));
	if(NULL == dq)
		return NULL;

	dq->hm = hm;

	if(data_ops)
	{
		dq->data_ops.construct = data_ops->construct;
		dq->data_ops.copy = data_ops->copy;
		dq->data_ops.destruct = data_ops->destruct;
	}
	else
		memset(&dq->data_ops, 0, sizeof(dq->data_ops));

	if(buffer_size)
		dq->buffer_size = buffer_size;
	else
		dq->buffer_size = DEF_BUFFER_SIZE;

	if(map_size)
		dq->map_size = map_size;
	else
		dq->map_size = DEF_MAP_SIZE;

	dq->map = (mydeque_buffer_t **)MyMemPoolMalloc(dq->hm, CAL_MAP_SIZE(dq->map_size));
	if(NULL == dq->map)
		goto MyDequeConstruct_err_;
	memset(dq->map, 0, CAL_MAP_SIZE(dq->map_size));
	dq->min_head = dq->max_tail = dq->last.map_index = dq->first.map_index = CAL_MAP_START(dq->map_size, 1);	

	dq->map[dq->first.map_index] = deque_inter_create_buffer(dq);
	if(NULL == dq->map[dq->first.map_index])
		goto MyDequeConstruct_err_;
	dq->first.node_index = dq->last.node_index = (dq->buffer_size)/2;

	dq->element_count = 0;
	
	return (HMYDEQUE)dq;

MyDequeConstruct_err_:

	deque_inter_destroy(dq);

	return NULL;

#undef DEF_BUFFER_SIZE
#undef DEF_MAP_SIZE
}

/*
*
*析构
*
*/
void MyDequeDestruct(HMYDEQUE hdq)
{
	mydeque_t * dq = (mydeque_t *)hdq;
	if(NULL == dq)
		return;

	deque_inter_destroy(dq);
}

/*
*
*从头部添加
*
*/
int MyDequeAddHead(HMYDEQUE hdq, const void * data, const size_t data_size)
{
	mydeque_node_t * node = NULL;
	mydeque_t * dq = (mydeque_t *)hdq;
	if(NULL == dq)
		return -1;

	assert(dq->first.map_index);
	if(0 == dq->first.node_index)
	{
		if(0 == (dq->first.map_index - 1))
		{
			if(0 != deque_expand_map(dq))
				return -1;
		}

		assert(dq->first.map_index);
		dq->first.map_index --;
		assert(dq->first.map_index);

		if(NULL == dq->map[dq->first.map_index])
		{
			mydeque_buffer_t * b = deque_inter_create_buffer(dq);
			if(NULL == b)
			{
				dq->first.map_index ++;
				return -1;
			}
			dq->map[dq->first.map_index] = b;
		}

		if(dq->min_head > dq->first.map_index)
			dq->min_head = dq->first.map_index;

		dq->first.node_index = dq->map[dq->first.map_index]->len - 1;
	}
	else
		dq->first.node_index --;

	node = deque_inter_create_node(dq, data, data_size);
	if(NULL == node)
		return -1;

	dq->map[dq->first.map_index]->first[dq->first.node_index] = node;
	dq->element_count ++;

	return 0;
}

/*
*
*从尾部添加
*
*/
int MyDequeAddTail(HMYDEQUE hdq, const void * data, const size_t data_size)
{
	mydeque_node_t * node = NULL;
	mydeque_t * dq = (mydeque_t *)hdq;
	if(NULL == dq)
		return -1;

	assert(dq->buffer_size >= dq->last.node_index && dq->map_size > dq->last.map_index);

	if(dq->map[dq->last.map_index]->len == dq->last.node_index)
	{
		if((dq->map_size - 1) == dq->last.map_index)
		{
			if(0 != deque_expand_map(dq))
				return -1;
		}

		dq->last.map_index ++;

		assert(dq->map_size > dq->last.map_index);
		if(NULL == dq->map[dq->last.map_index])
		{
			mydeque_buffer_t * b = deque_inter_create_buffer(dq);
			if(NULL == b)
			{
				dq->last.map_index ++;
				return -1;
			}
			dq->map[dq->last.map_index] = b;
		}

		if(dq->max_tail < dq->last.map_index)
			dq->max_tail = dq->last.map_index;

		dq->last.node_index = 0;
	}

	node = deque_inter_create_node(dq, data, data_size);
	if(NULL == node)
		return -1;

	dq->map[dq->last.map_index]->first[dq->last.node_index] = node;
	dq->element_count ++;

	dq->last.node_index ++;

	return 0;
}

/*
*
*从头部删除
*
*/
int MyDequeDelHead(HMYDEQUE hdq)
{
	mydeque_t * dq = (mydeque_t *)hdq;
	if(NULL == dq)
		return -1;

	return deque_inter_del_head(dq);
}

/*
*
*从尾部删除
*
*/
int MyDequeDelTail(HMYDEQUE hdq)
{
	mydeque_t * dq = (mydeque_t *)hdq;
	if(NULL == dq)
		return -1;

	return deque_inter_del_tail(dq);
}

/*
*
*获取头部
*
*/
int MyDequeGetHead(HMYDEQUE hdq, void ** data, size_t * data_size)
{
	mydeque_t * dq = (mydeque_t *)hdq;
	if(NULL == dq)
		return -1;

	if(0 == dq->element_count)
		return -1;

	assert(dq->map);
	assert(dq->map_size > dq->first.map_index);
	assert(dq->map[dq->first.map_index]);
	assert(dq->map[dq->first.map_index]->first);
	assert(dq->map[dq->first.map_index]->len > dq->first.node_index);
	assert(dq->map[dq->first.map_index]->first[dq->first.node_index]);

	if(data)
		*data = dq->map[dq->first.map_index]->first[dq->first.node_index]->data;
	if(data_size)
		*data_size = dq->map[dq->first.map_index]->first[dq->first.node_index]->data_size;

	return 0;
}

/*
*
*获取尾部
*
*/
int MyDequeGetTail(HMYDEQUE hdq, void ** data, size_t * data_size)
{
	size_t node_index = 0;
	size_t map_index = 0;
	mydeque_t * dq = (mydeque_t *)hdq;
	if(NULL == dq)
		return -1;

	if(0 == dq->element_count)
		return -1;

	map_index = dq->last.map_index;
	if(dq->last.node_index == 0)
	{
		map_index --;
		node_index = dq->map[map_index]->len - 1;
	}
	else
		node_index = dq->last.node_index - 1;

	assert(dq->map);
	assert(dq->map_size > map_index);
	assert(dq->map[map_index]);
	assert(dq->map[map_index]->first);
	assert(dq->map[map_index]->len >= dq->last.node_index);
	assert(dq->map[map_index]->first[node_index]);

	if(data)
		*data = dq->map[map_index]->first[node_index]->data;
	if(data_size)
		*data_size = dq->map[map_index]->first[node_index]->data_size;

	return 0;
}

/*
*
*获取尾部
*
*/
size_t MyDequeGetCount(HMYDEQUE hdq)
{
	mydeque_t * dq = (mydeque_t *)hdq;
	if(NULL == dq)
		return 0;

	return dq->element_count;
}

/*
*
*获取尾部
*
*/
void MyDequePrint(HMYDEQUE hdq)
{
	int i = 0;
	mydeque_iter_t it = {0};
	mydeque_t * dq = (mydeque_t *)hdq;
	assert(dq && dq->map && dq->map_size);

	it = dq->first;
	for(i = 0; ; i ++)
	{
		if(0 == i % 16)
			printf("\n");

		if(it.map_index > dq->last.map_index)
			break;
		else if(it.node_index >= dq->last.node_index && it.map_index == dq->last.map_index)
			break;

		assert(dq->map[it.map_index] != NULL);
		assert(dq->map[it.map_index]->first[it.node_index] != NULL);

#ifdef _MBCSV6
		printf("%d ", (long)dq->map[it.map_index]->first[it.node_index]->data);
#else
#ifdef WIN32
		printf("%d ", (long long)dq->map[it.map_index]->first[it.node_index]->data);
#else
		printf("%d ", (long)dq->map[it.map_index]->first[it.node_index]->data);
#endif
#endif

		it.node_index ++;

		if(it.map_index > dq->last.map_index)
			break;
		else if(it.node_index >= dq->last.node_index && it.map_index == dq->last.map_index)
			break;

		if(it.node_index >= dq->map[it.map_index]->len)
		{
			it.node_index = 0;
			it.map_index ++;
		}
	}

	printf("\n");
}


























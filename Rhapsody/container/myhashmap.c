/*
*
* myhashmap.c hash映射 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/

#include "myhashmap.h"

typedef struct __myhashmap_node_t_
{
	myhash_node_t base;

	size_t key_size;
	size_t data_size;
}myhashmap_node_t;

typedef struct __myhashmap_t_
{
	myhash_t base;

	MYOBJ_CONSTRUCT key_construct;
	MYOBJ_DESTRUCT	key_destruct;
	MYOBJ_CONSTRUCT data_construct;
	MYOBJ_DESTRUCT	data_destruct;
	MYOBJ_COPY		key_copy;
	MYOBJ_COPY		data_copy;
}myhashmap_t;


static __INLINE__ myhashmap_node_t * hashmap_inter_create_node(myhashmap_t * h, 
													const void * key, 
													const size_t key_size, 
													const void * data, 
													const size_t data_size)
{
	myhashmap_node_t * node = NULL;

	size_t key_alignment = 0;
	size_t data_alignment = 0;

	assert(h && h->base.hash_array && h->base.hash_array_size);

	if(key_size)
		key_alignment = CAL_SYS_ALIGMENT(key_size);
	else
		key_alignment = 0;

	if(data_size)
		data_alignment = CAL_SYS_ALIGMENT(data_size);
	else
		data_alignment = 0;

	node = (myhashmap_node_t *)MyMemPoolMalloc(h->base.hm, sizeof(*node) + key_size + key_alignment + data_size + data_alignment);
	if(NULL == node)
		return NULL;

	if(0 == key_size || NULL == key)
		node->base.key = (void *)key;
	else
	{
		node->base.key = (char *)node + sizeof(*node);

		//如果有键的构造函数
		if(h->key_construct)
			h->key_construct(node->base.key, key_size, NULL, 0);

		//如果有键的拷贝构造函数
		if(h->key_copy)
			h->key_copy(node->base.key, key_size, key, key_size);
		else
			memcpy(node->base.key, key, key_size);
		*((char *)(node->base.key) + key_size) = 0;
	}
	node->key_size = key_size;

	if(0 == data_size || NULL == data)
		node->base.data = (void *)data;
	else
	{
		node->base.data = (char *)node + sizeof(*node) + key_size + key_alignment;

		//如果有值的构造函数
		if(h->data_construct)
			h->data_construct(node->base.data, data_size, NULL, 0);

		//如果有值的拷贝构造函数
		if(h->data_copy)
			h->data_copy(node->base.data, data_size, data, data_size);
		else
			memcpy(node->base.data, data, data_size);
		*((char *)(node->base.data) + data_size) = 0;
	}
	node->data_size = data_size;

	node->base.next = NULL;

	return node;
}

static __INLINE__ void hashmap_inter_destroy_node(myhashmap_t * h, myhashmap_node_t * node)
{
	assert(h && node);

	//如果存在键析构函数
	if(h->key_destruct && node->key_size)
		h->key_destruct(node->base.key, node->key_size);

	//如果存在值析构函数
	if(h->data_destruct && node->data_size)
		h->data_destruct(node->base.data, node->data_size);

	MyMemPoolFree(h->base.hm, node);
}

/*
*
*删除所有节点
*
*/
/*static __INLINE__ void hashmap_inter_del_all(myhashmap_t * h)
{
	size_t i = 0;

	assert(h->base.hash_array && h->base.hash_array_size);

	for(i = 0; i < h->base.hash_array_size; i ++)
	{
		myhash_node_t * tmp = h->base.hash_array[i];
		while(tmp)
		{
			myhash_node_t * t = tmp;
			tmp = tmp->next;

			hashmap_inter_destroy_node(h, (myhashmap_node_t *)t);
		}
	}

	memset(h->base.hash_array, 0, __hash_array_size(h->base.hash_array_size));
	h->base.element_count = 0;
}*/


/*
*
*构造映射
*
*/
HMYHASHMAP MyHashMapConstruct(HMYMEMPOOL hm, MYHASH_FUN hash_fun, MYHASH_KEYEQUAL_FUN keyequal_fun, size_t hash_size,
								myobj_ops * key_ops,
								myobj_ops * data_ops)
{
	myhashmap_t * hht = NULL;

	if(NULL == hash_fun || NULL == keyequal_fun)
		return NULL;

	hht = (myhashmap_t *)MyMemPoolMalloc(hm, sizeof(*hht));
	if(NULL == hht)
		return NULL;

	if(key_ops)
	{
		hht->key_construct = key_ops->construct;
		hht->key_destruct = key_ops->destruct;
		hht->key_copy = key_ops->copy;
	}
	else
	{
		hht->key_construct = NULL;
		hht->key_destruct = NULL;
		hht->key_copy = NULL;
	}

	if(data_ops)
	{
		hht->data_construct = data_ops->construct;
		hht->data_destruct = data_ops->destruct;
		hht->data_copy = data_ops->copy;
	}
	else
	{
		hht->data_construct = NULL;
		hht->data_destruct = NULL;
		hht->data_copy = NULL;
	}

	if(0 == hashtable_inter_construct((myhash_t *)hht, hm, hash_fun, keyequal_fun, hash_size))
		return (HMYHASHMAP)hht;

	if(hht)
		MyMemPoolFree(hm, hht);

	return NULL;
}

/*
*
*析构映射
*
*/
void MyHashMapDestruct(HMYHASHMAP hhashmap)
{
	myhashmap_t * h = (myhashmap_t *)hhashmap;
	if(NULL == h)
		return;

	if(NULL != h->base.hash_array && h->base.hash_array_size)
		hash_inter_del_all(&(h->base));

	MyMemPoolFree(h->base.hm, h->base.hash_array);
	MyMemPoolFree(h->base.hm, h);
}

/*
*
*插入元素
*
*/
HMYHASHMAP_ITER MyHashMapInsertUnique(HMYHASHMAP hhm, const void * key, size_t key_size, const void * data, size_t data_size)
{
	myhashmap_node_t * node = NULL;
	myhashmap_t * h = (myhashmap_t *)hhm;

	if(NULL == h)
		return NULL;

	//查找,如果找到返回null
	if(hash_inter_search((myhash_t *)h, key))
		return NULL;

	//找不到添加
	node = hashmap_inter_create_node(h, key, key_size, data, data_size);
	if(NULL == node)
		return NULL;

	if(hash_inter_add_with_resize((myhash_t *)h, (myhash_node_t *)node) == 0)
		return (HMYHASHMAP_ITER)node;

	MyMemPoolFree(h->base.hm, node);
	return NULL;
}

/*
*
*删除一条记录,根据key,
*成功删除返回0, 否则返回-1
*
*/
int MyHashMapDelKey(HMYHASHMAP hhm, const void * key)
{
	myhashmap_t * h = (myhashmap_t *)hhm;
	size_t hash_key = 0;
	myhash_node_t * node = NULL;
	myhash_node_t * prev = NULL;

	if(NULL == h || NULL == h->base.keyequal_fun || NULL == h->base.hash_fun || NULL == h->base.hash_array || 0 == h->base.hash_array_size)
		return -1;

	hash_key = hash_inter_calhash(&(h->base), key) % h->base.hash_array_size;

	//计算哈希
	prev = node = h->base.hash_array[hash_key];

	//线性查找
	while(node)
	{
		if(hash_inter_equal(&(h->base), node->key, key))
			break;

		prev = node;
		node = node->next;
	}

	if(NULL == node)
		return -1;
	
	hash_inter_outlink_node(&(h->base), prev, node, NULL, NULL);
	hashmap_inter_destroy_node(h, (myhashmap_node_t *)node);

	return 0;
}

/*
*
*删除一条记录,根据迭代器
*成功删除返回0, 否则返回-1
*
*/
int MyHashMapDelIter(HMYHASHMAP hhm, HMYHASHMAP_ITER iter)
{
	myhashmap_t * h = (myhashmap_t *)hhm;
	myhash_node_t * node = (myhash_node_t *)iter;
	myhash_node_t * prev = NULL;
	size_t hash_key = 0;

	if(NULL == h || NULL == node || NULL == h->base.hash_array || 0 == h->base.hash_array_size)
		return -1;

	//取出hash key并判断其合法
	hash_key = node->hash_key;
	if(hash_key >= h->base.hash_array_size)
		return -1;

	prev = h->base.hash_array[hash_key];
	while(prev)
	{
		if(node == prev->next)
			break;

		prev = prev->next;
	}

	if(NULL == prev)
		return -1;

	hash_inter_outlink_node(&(h->base), prev, node, NULL, NULL);
	hashmap_inter_destroy_node(h, (myhashmap_node_t *)node);

	return 0;
}
















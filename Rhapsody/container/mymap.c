/*
*
* mymap.c 映射 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/


#include "mymap.h"


typedef struct __mymap_node_t
{
	myrbtree_node_t base;

	size_t key_size;
	size_t data_size;
}mymap_node_t;

typedef struct __mymap_t
{
	myrbtree_t base;

	MYOBJ_CONSTRUCT key_construct;
	MYOBJ_DESTRUCT key_destruct;
	MYOBJ_CONSTRUCT data_construct;
	MYOBJ_DESTRUCT data_destruct;
	MYOBJ_COPY key_copy;
	MYOBJ_COPY data_copy;
}mymap_t;


/*
*
*创建一个节点
*
*/
static __INLINE__ mymap_node_t * map_inter_create_node(mymap_t * m, const void * key, const size_t key_size, const void * data, const size_t data_size)
{
	mymap_node_t * node_new = NULL;

	size_t key_alignment = 0;
	size_t data_alignment = 0;

	assert(m);

	if(key_size)
		key_alignment = CAL_SYS_ALIGMENT(key_size);
	else
		key_alignment = 0;

	if(data_size)
		data_alignment = CAL_SYS_ALIGMENT(data_size);
	else
		data_alignment = 0;

	node_new = (mymap_node_t *)MyMemPoolMalloc(m->base.hm, sizeof(*node_new) + key_size + key_alignment + data_size + data_alignment);

	if(NULL == node_new)
		return NULL;

	memset(&(node_new->base), 0, sizeof(node_new->base));

	if(0 == key_size || NULL == key)
	{
		node_new->base.key = (void *)key;
	}
	else
	{
		node_new->base.key = (char *)node_new + sizeof(*node_new);

		//如果有键的构造函数
		if(m->key_construct)
			m->key_construct(node_new->base.key, key_size, NULL, 0);

		//如果有键的拷贝构造函数
		if(m->key_copy)
			m->key_copy(node_new->base.key, key_size, key, key_size);
		else
			memcpy(node_new->base.key, key, key_size);
		*((char *)(node_new->base.key) + key_size) = 0;
	}
	node_new->key_size = key_size;


	if(0 == data_size || NULL == data)
	{
		node_new->base.data = (void *)data;
	}
	else
	{
		node_new->base.data = (char *)node_new + sizeof(*node_new) + key_size + key_alignment;

		//如果有值的构造函数
		if(m->data_construct)
			m->data_construct(node_new->base.data, data_size, NULL, 0);

		//如果有值的拷贝构造函数
		if(m->data_copy)
			m->data_copy(node_new->base.data, data_size, data, data_size);
		else
			memcpy(node_new->base.data, data, data_size);
		*((char *)(node_new->base.data) + data_size) = 0;
	}
	node_new->data_size = data_size;


	return node_new;
}

/*
*
*销毁一个节点
*
*/
static __INLINE__ void map_inter_destroy_node(mymap_t * m, mymap_node_t * node)
{
	assert(m && node);

	//如果存在键析构函数
	if(m->key_destruct && node->key_size)
		m->key_destruct(node->base.key, node->key_size);

	//如果存在值析构函数
	if(m->data_destruct && node->data_size)
		m->data_destruct(node->base.data, node->data_size);

	MyMemPoolFree(m->base.hm, node);
}


/*
*
*创建映射
*
*/
static __INLINE__ void map_inter_del(mymap_t * m, mymap_node_t * node)
{
	assert(m && node);

	//旋转平衡红黑树
	node = (mymap_node_t *)rbtree_inter_rebalance_for_erase(&(m->base.root), (myrbtree_node_t *)node);

	if(NULL == node)
		return;

	//销毁node
	map_inter_destroy_node(m, node);
}

/*
*
*删除所有节点
*
*/
static __INLINE__ void map_inter_erase(mymap_t * m, mymap_node_t * node)
{
	assert(node);

	while(node)
	{
		mymap_node_t * y = (mymap_node_t *)node->base.left;

		if(node->base.right)
			map_inter_erase(m, (mymap_node_t *)node->base.right);

		map_inter_destroy_node(m, node);

		node = y;
	}
}


/*
*
*创建映射
*
*/
HMYMAP MyMapRealConstruct(HMYMEMPOOL hm, myrbtree_compare compare, 
	myobj_ops * key_ops,
	myobj_ops * data_ops)	
{
	mymap_t * m = NULL;

	m = (mymap_t *)MyMemPoolMalloc(hm, sizeof(*m));

	if(NULL == m)
		return NULL;

	memset(m, 0, sizeof(*m));

	m->base.compare = compare;
	m->base.hm = hm;

	if(key_ops)
	{
		m->key_construct = key_ops->construct;
		m->key_destruct = key_ops->destruct;
		m->key_copy = key_ops->copy;
	}
	else
	{
		m->key_construct = NULL;
		m->key_destruct = NULL;
		m->key_copy = NULL;
	}

	if(data_ops)
	{
		m->data_construct = data_ops->construct;
		m->data_destruct = data_ops->destruct;
		m->data_copy = data_ops->copy;
	}
	else
	{
		m->data_construct = NULL;
		m->data_destruct = NULL;
		m->data_copy = NULL;
	}

	return (HMYMAP)m;
}

/*
*
*映射的析构
*
*/
void MyMapDestruct(HMYMAP hmap)
{
	mymap_t * m = (mymap_t *)hmap;

	if(NULL == m)
		return;

	//遍历树,释放每个节点
	if(m->base.root)
		map_inter_erase(m, (mymap_node_t *)m->base.root);

	MyMemPoolFree(m->base.hm, m);
}

/*
*
*往rb树中插入一个节点
*
*/
HMYMAP_ITER MyMapInsertUnique(HMYMAP hmap, const void * key, const size_t key_size, const void * data, const size_t data_size)
{
	mymap_t * m = (mymap_t *)hmap;

	myrbtree_node_t * parent = NULL;
	mymap_node_t * node_new = NULL;

	if(NULL == m)
        return NULL;

	node_new = (mymap_node_t *)rbtree_inter_searchex((myrbtree_t *)m, m->base.root, key, &parent);
	if(node_new != NULL)
		return (HMYMAP_ITER)node_new;

	node_new = map_inter_create_node(m, key, key_size, data, data_size);
	if(NULL == node_new)
		return NULL;

	rbtree_inter_insert((myrbtree_t *)m, &(m->base.root), (myrbtree_node_t *)node_new, parent);

	return (HMYMAP_ITER)node_new;
}

/*
*
*获取节点的值
*
*/
void * MyMapGetIterData(const HMYMAP_ITER it, size_t * data_size)
{
	mymap_node_t * node = (mymap_node_t *)it;
	if(NULL == node)
		return NULL;

	if(data_size)
		*data_size = node->data_size;

	return node->base.data;
}

/*
*
*设置节点的值
*
*/
/*int MyMapSetIterData(HMYMAP_ITER it, const void * data, size_t data_size)
{
	assert(0);
	return -1;
}*/

/*
*
*根据键删除
*
*/
int MyMapDelKey(HMYMAP hmap, const void * key)
{
	mymap_t * m = (mymap_t *)hmap;
	mymap_node_t * node = NULL;

	if(NULL == m)
		return -1;

	node = (mymap_node_t *)rbtree_inter_search((myrbtree_t *)m, m->base.root, key);

	if(NULL == node)
		return -1;

	map_inter_del(m, node);

	return 0;
}

/*
*
*根据迭代器删除
*
*/
void MyMapDelIter(HMYMAP hmap, HMYMAP_ITER it)
{
	mymap_t * m = (mymap_t *)hmap;
	mymap_node_t * node = (mymap_node_t *)it;

	if(NULL == m || NULL == node)
		return;

	assert(rbtree_inter_search((myrbtree_t *)m, m->base.root, node->base.key));

	map_inter_del(m, node);
}

/*
*
*删除所有节点
*
*/
void MyMapClear(HMYMAP hmap)
{
	mymap_t * m = (mymap_t *)hmap;

	if(NULL == m)
		return;

	if(NULL == m->base.root)
		return;

	//遍历树,释放每个节点
	map_inter_erase(m, (mymap_node_t *)m->base.root);
	m->base.root = NULL;
}

















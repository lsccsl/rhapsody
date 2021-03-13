/*
*
* mylistex.h 链表 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/


#include "mylistex.h"


typedef struct __mylist_ex_t_
{
	mylist_t base;

	myobj_ops data_ops;
}mylist_ex_t;

typedef struct __mylist_ex_node_t_
{
	mylist_node_t base;

	size_t user_data_size;
}mylist_ex_node_t;


static __INLINE__ mylist_ex_node_t * list_ex_inter_create_node(mylist_ex_t * lst, const void * data, size_t data_size)
{
	mylist_ex_node_t * node = NULL;

	assert(lst);

	if(NULL == data || 0 == data_size)
	{
		node = (mylist_ex_node_t *)MyMemPoolMalloc(lst->base.hm, sizeof(*node));

		assert(node);
		memset(node, 0, sizeof(*node));

		node->base.userdata = (void *)data;

		return node;
	}

	node = (mylist_ex_node_t *)MyMemPoolMalloc(lst->base.hm, sizeof(*node) + data_size);
	assert(node);
	memset(node, 0, sizeof(*node));

	node->base.userdata = (char *)node + sizeof(*node);
	node->user_data_size = data_size;

	if(lst->data_ops.construct)
		lst->data_ops.construct(node->base.userdata, data_size, NULL, 0);

	if(lst->data_ops.copy)
		lst->data_ops.copy(node->base.userdata, data_size, data, data_size);
	else
		memcpy(node->base.userdata, data, data_size);

	return node;
}

static __INLINE__ mylist_ex_node_t * list_ex_inter_erase(mylist_ex_t * lst, mylist_ex_node_t * node)
{
	mylist_node_t * next = NULL;

	assert(lst && node);
	assert(node->base.prev && node->base.next);

	next = node->base.prev->next = node->base.next;
	node->base.next->prev = node->base.prev;

	if(lst->data_ops.destruct && node->user_data_size)
		lst->data_ops.destruct(node->base.userdata, node->user_data_size);

	MyMemPoolFree(lst->base.hm, node);

	return (mylist_ex_node_t *)next;
}

/*static __INLINE__ int list_ex_pop(mylist_ex_t * lst, mylist_ex_node_t * node, void * data, size_t data_size)
{
	assert(lst && node);

	if(0 == node->user_data_size || 0 == node->base.userdata)
	{
		if(data_size < sizeof(node->base.userdata))
			return -1;

		memcpy(data, &node->base.userdata, sizeof(node->base.userdata));
		return 0;
	}

	if(lst->data_ops.copy)
		lst->data_ops.copy(data, data_size, node->base.userdata, node->user_data_size);
	else
		memcpy(data, node->base.userdata, (data_size < node->user_data_size) ? data_size : node->user_data_size);

	list_ex_inter_erase(lst, node);

	return 0;
}*/

static __INLINE__ void list_ex_erase_all(mylist_ex_t * lst)
{
	mylist_ex_node_t * node = NULL;
	mylist_ex_node_t * end = NULL;

	assert(lst);

	node = (mylist_ex_node_t *)list_inter_begin((mylist_t *)lst);
	end = (mylist_ex_node_t *)list_inter_end((mylist_t *)lst);

	assert(node && end);

	while(node != end)
	{
		node = list_ex_inter_erase(lst, node);
	}
}


/*
*
*mylistex的构造
*
*/
HMYLIST_EX MyListExConstruct(HMYMEMPOOL hm, myobj_ops * data_ops)
{
	mylist_ex_t * lst = NULL;

	lst = (mylist_ex_t *)MyMemPoolMalloc(hm, sizeof(*lst));

	if(NULL == lst)
		return NULL;

	memset(lst, 0, sizeof(*lst));

	lst->base.hm = hm;

	if(data_ops)
	{
		lst->data_ops.construct = data_ops->construct;
		lst->data_ops.destruct = data_ops->destruct;
		lst->data_ops.copy = data_ops->copy;
	}

	//创建第一节点
	lst->base.head = (mylist_node_t *)list_ex_inter_create_node(lst, NULL, 0);
	if(NULL == lst->base.head)
		goto err_;

	lst->base.head->next = (struct __mylist_node_t *)lst->base.head;
	lst->base.head->prev = (struct __mylist_node_t * )lst->base.head;

	return (HMYLIST_EX)lst;

err_:

	MyMemPoolFree(hm, lst);

	return NULL;
}

/*
*
*mylistex的构造
*
*/
void MyListExDestruct(HMYLIST_EX hlist_ex)
{
	mylist_ex_t * lst = (mylist_ex_t *)hlist_ex;
	if(NULL == lst)
		return;

	list_ex_erase_all(lst);

	assert(lst->base.head);

	list_ex_inter_erase(lst, (mylist_ex_node_t *)(lst->base.head));

	MyMemPoolFree(lst->base.hm, lst);
}

/*
*
*添加一节点到至链表尾
*
*/
HMYLIST_EX_ITER MyListExAddTail(HMYLIST_EX hlist, const void * userdata, size_t data_size)
{
	mylist_ex_t * lst = (mylist_ex_t *)hlist;
	mylist_ex_node_t * node = NULL;
	if(NULL == lst)
		return NULL;

	node = list_ex_inter_create_node(lst, userdata, data_size);

	list_inter_link_before((mylist_node_t *)node, list_inter_end((mylist_t *)lst));

	return (HMYLIST_EX_ITER)node;
}

/*
*
*添加一节点至链表头
*
*/
HMYLIST_EX_ITER MyListExAddHead(HMYLIST_EX hlist, const void * userdata, size_t data_size)
{
	mylist_ex_t * lst = (mylist_ex_t *)hlist;
	mylist_ex_node_t * node = NULL;
	if(NULL == lst)
		return NULL;

	node = list_ex_inter_create_node(lst, userdata, data_size);

	list_inter_link_before((mylist_node_t *)node, list_inter_begin((mylist_t *)lst));

	return (HMYLIST_EX_ITER)node;
}

/*
*
*删除一节点，返回用户数据
*
*/
HMYLIST_EX_ITER MyListExErase(HMYLIST_EX hlist, HMYLIST_ITER iter)
{
	mylist_ex_node_t * node = (mylist_ex_node_t *)iter;
	mylist_ex_t * lst = (mylist_ex_t *)hlist;

	if(NULL == lst || NULL == node)
		return NULL;

	return (HMYLIST_EX_ITER)list_ex_inter_erase(lst, node);
}

/*
*
*删除所有节点
*
*/
void MyListExEraseAll(HMYLIST_EX hlist)
{
	mylist_ex_t * lst = (mylist_ex_t *)hlist;

	if(NULL == lst)
		return;

	list_ex_erase_all(lst);
}

/*
*
*弹出头节点
*
*/
/*int MyListExPopHead(HMYLIST_EX hlist, void * data, size_t data_size)
{
	mylist_ex_t * lst = (mylist_ex_t *)hlist;

	if(NULL == lst)
		return -1;

	if(list_inter_empty((mylist_t *)lst))
		return -1;

	return list_ex_pop(lst, (mylist_ex_node_t *)list_inter_begin((mylist_t *)lst), data, data_size);
}*/

/*
*
*弹出尾节点
*
*/
/*int MyListExPopTail(HMYLIST_EX hlist, void * data, size_t data_size)
{
	mylist_ex_t * lst = (mylist_ex_t *)hlist;

	if(NULL == lst)
		return -1;

	if(list_inter_empty((mylist_t *)lst))
		return -1;

	return list_ex_pop(lst, (mylist_ex_node_t *)(list_inter_end((mylist_t *)lst)->prev), data, data_size);
}*/






















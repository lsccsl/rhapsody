/*
*
* mylist.h 链表 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include "mylist.h"
#include "myutility.h"


typedef struct __mylist_node_t
{
	struct __mylist_node_t * prev;
	struct __mylist_node_t * next;

	//用户数据
	void * userdata;
}mylist_node_t;

typedef struct __mylist_t
{
	mylist_node_t * head;

	//内存池句柄
	HMYMEMPOOL hm;
}mylist_t;


static __INLINE__ void * list_inter_malloc(mylist_t * lst, size_t size)
{
	assert(lst && size);

	return MyMemPoolMalloc(lst->hm, size);
}

static __INLINE__ void list_inter_free(mylist_t * lst, void * ptr)
{
	assert(lst && ptr);

	MyMemPoolFree(lst->hm, ptr);
}

static __INLINE__ mylist_node_t * list_inter_create_node(mylist_t * lst)
{
	mylist_node_t * node = NULL;

	assert(lst);

	node = (mylist_node_t *)list_inter_malloc(lst, sizeof(*node));
	assert(node);

	return node;
}

static __INLINE__ int list_inter_empty(mylist_t * lst)
{
	assert(lst);
	assert(lst->head);

	return (lst->head->next == lst->head)?1:0;
}

static __INLINE__ mylist_node_t * list_inter_begin(mylist_t * lst)
{
	assert(lst);
	assert(lst->head);
	assert(lst->head->next);

	return (mylist_node_t *)lst->head->next;
}

static __INLINE__ mylist_node_t * list_inter_end(mylist_t * lst)
{
	assert(lst);
	assert(lst->head);

	return lst->head;
}

static __INLINE__ mylist_node_t * list_inter_erase(mylist_t * lst, mylist_node_t * node)
{
	mylist_node_t * next = NULL;

	assert(lst && node);
	assert(node->prev && node->next);

	next = node->prev->next = node->next;
	node->next->prev = node->prev;

	list_inter_free(lst, node);

	return next;
}

static __INLINE__ void list_inter_link_before(mylist_node_t * node_to_link, mylist_node_t * node)
{
	assert(node_to_link && node);

	node_to_link->next = node;
	node_to_link->prev = node->prev;

	assert(node_to_link->next && node_to_link->prev);

	node->prev = node_to_link;
	node_to_link->prev->next = node_to_link;
}


/*
*
*构造链表
*
*/
HMYLIST MyListConstruct(HMYMEMPOOL hm)
{
	mylist_t * lst = NULL;

	lst = (mylist_t *)MyMemPoolMalloc(hm, sizeof(*lst));

	if(NULL == lst)
		return NULL;

	memset(lst, 0, sizeof(*lst));

	lst->hm = hm;

	//创建第一节点
	lst->head = list_inter_create_node(lst);
	if(NULL == lst->head)
		goto err_;

	memset(lst->head, 0, sizeof(*(lst->head)));
	lst->head->next = (struct __mylist_node_t *)lst->head;
	lst->head->prev = (struct __mylist_node_t * )lst->head;

	return (HMYLIST)lst;

err_:

	list_inter_free(lst, lst);

	return NULL;
}

/*
*
*销毁链表
*
*/
int MyListDestruct(HMYLIST hlist)
{
	mylist_node_t * node = NULL;

	mylist_t * lst = (mylist_t *)hlist;
	if(NULL == lst)
		return -1;

	//依次销毁各个节点
	node = list_inter_begin(lst);
	while(node != list_inter_end(lst))
	{
		mylist_node_t * temp = node;

		assert(node);
		node = (mylist_node_t *)node->next;
		list_inter_free(lst, temp);
	}

	list_inter_free(lst, lst->head);

	//销毁链表对象
	list_inter_free(lst, lst);

	return 0;
}

/*
*
*添加一节点到至链表尾
*
*/
HMYLIST_ITER MyListAddTail(HMYLIST hlist, const void * userdata)
{
	mylist_node_t * node = NULL;

	mylist_t * lst = (mylist_t *)hlist;
	if(NULL == lst)
		return NULL;

	node = list_inter_create_node(lst);
	memset(node, 0, sizeof(*node));

	node->userdata = (void *)userdata;

	list_inter_link_before(node, list_inter_end(lst));
	//node->next = list_inter_end(lst);
	//node->prev = list_inter_end(lst)->prev;

	//assert(node->next && node->prev);

	//list_inter_end(lst)->prev = node;
	//node->prev->next = node;

	return (HMYLIST_ITER)node;
}

/*
*
*添加一节点至链表头
*
*/
HMYLIST_ITER MyListAddHead(HMYLIST hlist, const void * userdata)
{
	mylist_node_t * node = NULL;

	mylist_t * lst = (mylist_t *)hlist;
	if(NULL == lst)
		return NULL;

	node = list_inter_create_node(lst);
	memset(node, 0, sizeof(*node));

	node->userdata = (void *)userdata;

	list_inter_link_before(node, list_inter_begin(lst));
	//node->next = list_inter_begin(lst);
	//node->prev = list_inter_begin(lst)->prev;

	//assert(node->next && node->prev);

	//list_inter_begin(lst)->prev = node;
	//node->prev->next = node;

	return (HMYLIST_ITER)node;
}

/*
*
*删除一节点，返回用户数据
*
*/
HMYLIST_ITER MyListErase(HMYLIST hlist, HMYLIST_ITER iter)
{
	mylist_node_t * node = (mylist_node_t *)iter;
	mylist_t * lst = (mylist_t *)hlist;

	if(NULL == lst || NULL == node)
		return NULL;

	return (HMYLIST_ITER)list_inter_erase(lst, node);
}

/*
*
*删除所有节点
*
*/
void MyListEraseAll(HMYLIST hlist)
{
	mylist_node_t * node = NULL;
	mylist_node_t * end = NULL;
	mylist_t * lst = (mylist_t *)hlist;

	if(NULL == lst)
		return;

	node = list_inter_begin(lst);
	end = list_inter_end(lst);

	assert(node && end);

	while(node != end)
	{
		node = list_inter_erase(lst, node);
	}
}

/*
*
*获取头结点
*
*/
HMYLIST_ITER MyListGetHead(HMYLIST hlist)
{
	mylist_t * lst = (mylist_t *)hlist;
	if(NULL == lst)
		return NULL;

	return (HMYLIST_ITER)list_inter_begin(lst);
}

/*
*
*获取尾结点
*
*/
HMYLIST_ITER MyListGetTail(HMYLIST hlist)
{
	mylist_t * lst = (mylist_t *)hlist;
	if(NULL == lst)
		return NULL;

	return (HMYLIST_ITER)list_inter_end(lst);
}

/*
*
*获取下一节点
*
*/
HMYLIST_ITER MyListGetNext(HMYLIST hlist, HMYLIST_ITER iter)
{
	mylist_t * lst = (mylist_t *)hlist;
	mylist_node_t * node = (mylist_node_t *)iter;
	
	if(NULL == lst || NULL == node)
		return NULL;

	return (HMYLIST_ITER)node->next;
}

/*
*
*获取上一节点
*
*/
HMYLIST_ITER MyListGetPrev(HMYLIST hlist, HMYLIST_ITER iter)
{
	mylist_t * lst = (mylist_t *)hlist;
	mylist_node_t * node = (mylist_node_t *)iter;
	
	if(NULL == lst || NULL == node)
		return NULL;

	return (HMYLIST_ITER)node->prev;
}

/*
*
*获取节点的用户数据
*
*/
void * MyListGetIterData(HMYLIST_ITER iter)
{
	mylist_node_t * node = (mylist_node_t *)iter;

	if(NULL == node)
		return NULL;

	return node->userdata;
}

/*
*
*链表是否为空
*
*/
int MyListIsEmpty(HMYLIST hlist)
{
	mylist_t * lst = (mylist_t *)hlist;
	if(NULL == lst)
		return 1;

	return list_inter_empty(lst);
}

/*
*
*弹出头节点
*
*/
void * MyListPopHead(HMYLIST hlist)
{
	mylist_t * lst = (mylist_t *)hlist;
	void * data = NULL;

	if(NULL == lst)
		return NULL;

	if(list_inter_empty(lst))
		return NULL;

	data = list_inter_begin(lst)->userdata;

	list_inter_erase(lst, list_inter_begin(lst));

	return data;
}

/*
*
*弹出尾节点
*
*/
void * MyListPopTail(HMYLIST hlist)
{
	mylist_t * lst = (mylist_t *)hlist;
	mylist_node_t * node = NULL;
	void * data = NULL;

	if(NULL == lst)
		return NULL;

	if(list_inter_empty(lst))
		return NULL;

	node = list_inter_end(lst)->prev;

	data = node->userdata;

	list_inter_erase(lst, node);

	return data;
}

/*
*
*取出元素个数
*
*/
int MyListGetCount(HMYLIST hlist)
{
	mylist_t * lst = (mylist_t *)hlist;
	mylist_node_t * node = NULL;
	int ret = 0;

	if(NULL == lst)
		return 0;
	
	node = list_inter_begin(lst);

	while(node != list_inter_end(lst))
	{
		ret ++;
		node = node->next;
	}

	return ret;
}


#include "mylistex.c"


















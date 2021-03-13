/*
*
*MyStringSetEx.c from boost 字符串查找集合字符串查找集合-只需要"一次"字符串比较即可完成从无限个数的字符串集合里提取所需的字符串 lin shao chuan
*
*/


#include "MyStringSetEx.h"


#define STRING_SET_EX_INVAL_DATA (void *)-1


typedef struct __mystringset_node_t_ex
{
	myrbtree_node_t base;

	struct __mystringset_node_t_ex * middle;
}mystringset_node_t_ex;

typedef struct __mystringset_t_ex
{
	myrbtree_t base;
}mystringset_t_ex;


/*
*
*1 表示 key1 比 key2 大
*0 表示 key1 比 key2 小 
*
*/
static __INLINE__ int mystringset_ex_compare(const void * key1, const void * key2)
{
	/*return key1 > key2;*/
	return (char)key1 - (char)key2;
}

static __INLINE__ mystringset_node_t_ex * stringset_ex_create_node(mystringset_t_ex * sst, const char key, const void * data)
{
	mystringset_node_t_ex * node_new = NULL;

	assert(sst);

	node_new = (mystringset_node_t_ex *)MyMemPoolMalloc(sst->base.hm, sizeof(*node_new));

	if(NULL == node_new)
		return NULL;

	memset(node_new, 0, sizeof(*node_new));
	node_new->base.key = (void *)key;
	node_new->base.data = (void *)data;

	return node_new;
}

static __INLINE__ mystringset_node_t_ex * stringset_ex_inter_search(mystringset_t_ex * sst, const char * key)
{
	mystringset_node_t_ex * current;
	char * p = (char *)key;

	assert(sst && p && *p);

	current = (mystringset_node_t_ex *)sst->base.root;

	for(;current;)
	{
		current = (mystringset_node_t_ex *)rbtree_inter_search(&sst->base, (myrbtree_node_t *)current, (void *)*p);

		if(NULL == current)
			return NULL;

		p ++;

		if(*p)
		{
			current = current->middle;
			continue;
		}

		if(STRING_SET_EX_INVAL_DATA == current->base.data)
			return NULL;

		return current;
	}

	return NULL;
}


/*
*
*初始化字符串集合
*
*/
HMYSTRING_SET_EX MyStringSetExConstruct(HMYMEMPOOL hm)
{
	mystringset_t_ex * sst = NULL;

	sst = (mystringset_t_ex *)MyMemPoolMalloc(hm, sizeof(*sst));

	if(NULL == sst)
		return NULL;

	sst->base.compare = mystringset_ex_compare;
	sst->base.hm = hm;
	sst->base.root = NULL;

	return (HMYSTRING_SET_EX )sst;
}

/*
*
*反初始化字符串集合
*
*/
extern void MyStringSetExDestruct(HMYSTRING_SET_EX hss_ex);

/*
*
*反初始化字符串集合
*
*/
int MyStringSetExAdd(HMYSTRING_SET_EX hss_ex, const char * key, const void * data)
{
	mystringset_node_t_ex ** current;
	char * p = (char *)key;

	mystringset_t_ex * sst = (mystringset_t_ex *)hss_ex;

	if(NULL == sst || NULL == p || 0 == *p)
		return -1;

	current = (mystringset_node_t_ex **)&(sst->base.root);

	for(;;)
	{
		mystringset_node_t_ex * temp = NULL;
		if(&current)
		{
			myrbtree_node_t * parent = NULL;

			//搜索
			temp = (mystringset_node_t_ex  *)rbtree_inter_searchex(&sst->base, (myrbtree_node_t *)*current, (void *)(*p), &parent);

			if(NULL == temp)
			{
				temp = stringset_ex_create_node(sst, *p, STRING_SET_EX_INVAL_DATA);
				rbtree_inter_insert(&sst->base, (myrbtree_node_t **)current, (myrbtree_node_t *)temp, parent);
			}
		}
		else
		{
			temp = stringset_ex_create_node(sst, *p, STRING_SET_EX_INVAL_DATA);
			rbtree_inter_insert(&sst->base, (myrbtree_node_t **)current, (myrbtree_node_t *)temp, NULL);
		}

		current = &temp->middle;

		p ++;

		if(0 == *p)
		{
			if(temp->base.data != STRING_SET_EX_INVAL_DATA)
				return -1;

			temp->base.data = (void *)data;
			return 0;
		}
	}
}

/*
*
*反初始化字符串集合
*
*/
/*int MyStringSetExDel(HMYSTRING_SET_EX hss_ex, const char * key, void ** data)
{
	assert(0);
	//mystringset_node_t_ex * node;
	//mystringset_t_ex * sst = (mystringset_t_ex *)hss_ex;

	////顺次破坏每个节点
	//if(NULL == sst || NULL == key || 0 == *key)
	//	return -1;

	//node = stringset_ex_inter_search(sst, key);
	//if(NULL == node)
	//	return -1;

	//while(node)
	//{
	//	if(NULL == node->middle && NULL == node->base.left && NULL == node->base.right)
	//	mystringset_node_t_ex * temp = node->base.parent;
	//	rbtree_inter_del(&sst->base, &(sst->base.root), 
	//}

	return 0;
}*/

/*
*
*反初始化字符串集合
*
*/
int MyStringSetExSearch(HMYSTRING_SET_EX hss_ex, const char * key, void ** data)
{
	mystringset_node_t_ex * node;

	mystringset_t_ex * sst = (mystringset_t_ex *)hss_ex;

	if(NULL == sst || NULL == key || 0 == *key)
		return -1;

	node = stringset_ex_inter_search(sst, key);

	if(NULL == node)
		return -1;

	if(data)
		*data = node->base.data;

	return 0;
}















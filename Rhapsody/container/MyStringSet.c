/*
*
*MyStringSet.c from boost 字符串查找集合字符串查找集合-只需要"一次"字符串比较即可完成从无限个数的字符串集合里提取所需的字符串 lin shao chuan
*
*/


#include "MyStringSet.h"
#include <assert.h>
#include "mymap.h"
#include "myutility.h"


#define INVAL_DATA ((void *)-1)

typedef struct __mystringset_data_t_
{
	 HMYMAP hmap;
	 void * data;
}mystringset_data_t;

typedef struct __mystringset_t_
{
	 HMYMAP hmap;
	 HMYMEMPOOL hm;
}mystringset_t;


/*
*
*1 表示 key1 比 key2 大
*0 表示 key1 比 key2 小 
*
*/
static __INLINE__ int mystring_set_compare(const void * key1, const void * key2)
{
	return (char)key1 - (char)key2;
}

static __INLINE__ void data_destruct(void * data, size_t data_size)
{
	mystringset_data_t * dt = (mystringset_data_t *)data;
	assert(data_size == sizeof(*dt));

	if(dt->hmap)
		MyMapDestruct(dt->hmap);
}

static myobj_ops data_ops = {NULL, data_destruct, NULL};


/*
*
*创建字符串集合
*
*/
HMYSTRING_SET MyStringSetConstruct(HMYMEMPOOL hm)
{
	mystringset_t * ss = MyMemPoolMalloc(hm, sizeof(*ss));
	if(NULL == ss)
		return NULL;

	ss->hm = hm;

	ss->hmap = MyMapRealConstruct(hm, mystring_set_compare, NULL, &data_ops);

	return (HMYSTRING_SET)ss;
}

/*
*
*创建字符串集合
*
*/
extern void MyStringSetDestruct(HMYSTRING_SET hss);

/*
*
*创建字符串集合
*
*/
int MyStringSetAdd(HMYSTRING_SET hss, char * first, const char * last, const void * data, size_t data_size)
{
	HMYMAP current = NULL;

	mystringset_t * ss = (mystringset_t *)hss;
	if(NULL == ss || NULL == ss->hmap || last == first || NULL == first || NULL == last)
		return -1;

	MY_UNUSED_ARG(data_size);

	current = ss->hmap;

	for(;current;)
	{
		mystringset_data_t * t = NULL;

		HMYMAP_ITER it =
			MyMapSearch(current, (void *)(*first));

		if(it)
		{
			first ++;

			t = MyMapGetIterData(it, NULL);

			assert(t && t->hmap);

			current = t->hmap;

			if(last != first)
				continue;

			if(INVAL_DATA != t->data)
				return -1;

			t->data = (void *)data;
			return 0;
		}
		else
		{
			mystringset_data_t t = {NULL, INVAL_DATA};

			t.hmap = MyMapRealConstruct(ss->hm, mystring_set_compare, NULL, &data_ops);
			if(last == first)
				t.data = (void *)data;

			MyMapInsertUnique(current, (void *)(*first), 0, (void *)&t, sizeof(t));

			if(last == first)
				return 0;

			first ++;

			current = t.hmap;
		}
	}

	return -1;
}

/*
*
*删除字符串集合
*
*/
/*int MyStringSetDel(HMYSTRING_SET hss, char * first, const char * last, void ** data, size_t * data_size)
{
	//删除需要记录"父节点"
	return -1;
}*/

/*
*
*查找字符串集合
*
*/
int MyStringSetSearch(HMYSTRING_SET hss, char * first, const char * last, void ** data, size_t * data_size)
{
	HMYMAP current = NULL;

	mystringset_t * ss = (mystringset_t *)hss;
	if(NULL == ss || NULL == ss->hmap)
		return -1;

	MY_UNUSED_ARG(data_size);

	current = ss->hmap;

	for(;current;)
	{
		mystringset_data_t * t = NULL;
		//HMYMAP hmap = NULL;

		HMYMAP_ITER it =
			MyMapSearch(current, (void *)(*first));

		first ++;

		if(NULL == it)
			return -1;

		//ckey = (char)MyMapGetIterKey(it);

		t = MyMapGetIterData(it, NULL);

		assert(t && t->hmap);

		current = t->hmap;

		if(last >= first)
			continue;

		if(INVAL_DATA == t->data)
			return -1;

		if(data)
			*data = t->data;

		return 0;
	}

	return -1;
}

















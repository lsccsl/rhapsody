/*
*
* myvector.h vector 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/

#include "myvector.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "myutility.h"
#include "__vector_inter.h"


/*static __INLINE__ int vector_inter_resize(myvector_t * vt, size_t size)
{
	myvector_element ** el_array = NULL;

	assert(vt);

	if(size <= vt->el_array_size)
		return 0;

	el_array = (myvector_element **)MyMemPoolMalloc(vt->hm, size * sizeof(myvector_element*));
	if(NULL == el_array)
		return -1;

	memcpy(el_array, vt->el_array, vt->el_array_size * sizeof(myvector_element*));
	MyMemPoolFree(vt->hm, vt->el_array);
	vt->el_array_size = size;
	vt->el_array = el_array;

	return 0;
}*/

/*static __INLINE__ myvector_element * vector_inter_getindex(myvector_t * vt, size_t index)
{
	assert(vt && vt->el_array);

	if(index >= vt->el_count)
		return NULL;

	return vt->el_array[index];
}*/

/*static __INLINE__ int vector_inter_init_array(myvector_t * vt, int size)
{
#define DEFAULT_VECTOR_SIZE 32

	assert(vt);

	if(size > 0)
		vt->el_array_size = size;
	else
		vt->el_array_size = DEFAULT_VECTOR_SIZE;

	//分配vector的存储空间
	vt->el_array = (myvector_element **)MyMemPoolMalloc(vt->hm, vt->el_array_size * sizeof(myvector_element*));
	if(NULL == vt->el_array)
		return -1;

	return 0;

#undef DEFAULT_VECTOR_SIZE
}*/

/*static __INLINE__ void vector_inter_clear(myvector_t * vt)
{
	size_t i = 0;

	assert(vt && vt->el_array && vt->el_array_size);

	if(vt->data_ops.destruct)
	{
		for(i = 0; i< vt->el_count && i < vt->el_array_size; i ++)
		{
			if(NULL == vt->el_array[i])
				continue;

			if(vt->el_array[i]->data_size)
				vt->data_ops.destruct(vt->el_array[i]->data, vt->el_array[i]->data_size);
			MyMemPoolFree(vt->hm, vt->el_array[i]);
		}
	}
	else
	{
		for(i = 0; i< vt->el_count && i < vt->el_array_size; i ++)
		{
			if(NULL == vt->el_array[i])
				continue;

			MyMemPoolFree(vt->hm, vt->el_array[i]);
		}
	}

	vt->el_count = 0;
}*/

/*static __INLINE__ void vector_inter_free_array(myvector_t * vt)
{
	assert(vt && vt->el_array && vt->el_array_size);

	vector_inter_clear(vt);

	MyMemPoolFree(vt->hm, vt->el_array);
}*/

/*static __INLINE__ void vector_inter_destroy(myvector_t * vt)
{
	if(NULL == vt)
		return;
    
	vector_inter_free_array(vt);
	MyMemPoolFree(vt->hm, vt);
}*/

/*static __INLINE__ void vector_inter_destroy_element(myvector_t * vt, myvector_element * el)
{
	assert(vt && el);

	if(vt->data_ops.destruct && el->data_size && el->data)
		vt->data_ops.destruct(el->data, el->data_size);

	MyMemPoolFree(vt->hm, el);
}*/

/*static __INLINE__ void vector_inter_del(myvector_t * vt, size_t index)
{
	myvector_element * el = NULL;

	assert(vt && index < vt->el_count && vt->el_array && vt->el_array_size);

	el = vt->el_array[index];

	if(index < vt->el_count - 1)
		memcpy(vt->el_array + index, vt->el_array + index + 1, (vt->el_count - index - 1)*sizeof(myvector_element*));

	vt->el_count --;

	if(NULL == el)
		return;

	__vector_inter_destroy_element(vt, el);
}*/

/*static __INLINE__ myvector_element * vector_inter_create_element(myvector_t * vt, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;

	assert(vt);

	if(data_size && data)
	{
		el = (myvector_element *)MyMemPoolMalloc(vt->hm, sizeof(myvector_element) + data_size + CAL_SYS_ALIGMENT(data_size));
		if(NULL == el)
			return NULL;

		el->data = (char*)el + sizeof(*el);
		el->data_size = data_size;

		if(vt->data_ops.construct)
			vt->data_ops.construct(el->data, el->data_size, NULL, 0);

		if(vt->data_ops.copy)
			vt->data_ops.copy(el->data, el->data_size, data, data_size);
		else
			memcpy(el->data, data, el->data_size);
		*((char *)el->data + data_size) = 0;
	}
	else
	{
		el = (myvector_element *)MyMemPoolMalloc(vt->hm, sizeof(myvector_element));
		if(NULL == el)
			return NULL;

		el->data = (void *)data;
		el->data_size = 0;
	}

	return el;
}*/

/*static __INLINE__ myvector_element * vector_inter_add(myvector_t * vt, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;

	assert(vt && vt->el_array && vt->el_array_size);

	if(vt->el_count >= vt->el_array_size)
		vector_inter_resize(vt, vt->el_array_size * 2);//重构vector

	el = vector_inter_create_element(vt, data, data_size);
	if(NULL == el)
		return NULL;

	vt->el_array[vt->el_count] = el;

	vt->el_count ++;

	return el;
}*/

/*
* 计算第二个子节点的坐标
*
* 0 - 1 2  2(i + 1) - 1 2(i + 1)
* 1 - 3 4
* 2 - 5 6
*/
#define HEAP_SECOND_CHILD_INDEX(x) (2 * ((x) + 1))

/*
*计算父节点的坐标
*/
#define HEAP_PARENT_INDEX(x) (x - 1)/2

/*
*
*推入一个节点,
*
*/
static __INLINE__ void vector_inter_push(myvector_t * vt, size_t hole_index, const size_t top_index, const myvector_element * el_v)
{
	size_t parent_index = HEAP_PARENT_INDEX(hole_index);

	assert(vt && vt->el_array && vt->el_array_size && hole_index < vt->el_count && top_index < vt->el_count && vt->compare);

	while(hole_index > top_index && vt->compare(el_v->data, vt->el_array[parent_index]->data, vt->context) > 0)
	{
		vt->el_array[hole_index] = vt->el_array[parent_index];
		hole_index = parent_index;
		parent_index = HEAP_PARENT_INDEX(hole_index);
	}

	vt->el_array[hole_index] = (myvector_element *)el_v;
}

/*
*
*调整堆
*
*/
static __INLINE__ void vector_inter_adjust(myvector_t * vt, size_t hole_index, const size_t last, const myvector_element * el_v)
{
	size_t top_index = hole_index;
	size_t second_child = HEAP_SECOND_CHILD_INDEX(hole_index);

	assert(vt && vt->el_array && vt->el_array_size >= last && vt->compare);

	while(second_child < last)
	{
		if(vt->compare(vt->el_array[second_child - 1]->data, vt->el_array[second_child]->data, vt->context) > 0)
			second_child -= 1;

		vt->el_array[hole_index] = vt->el_array[second_child];
		hole_index = second_child;
		second_child = HEAP_SECOND_CHILD_INDEX(hole_index);
	}

	if(last == second_child)
	{
		second_child -= 1;
		vt->el_array[hole_index] = vt->el_array[second_child];
		hole_index = second_child;
	}

	vector_inter_push(vt, hole_index, top_index, el_v);
}

/*
*
*生成堆
*
*/
static __INLINE__ void vector_inter_make_heap(myvector_t * vt)
{
	size_t parent_index = 0;
	size_t len = 0;

	assert(vt && vt->el_array && vt->el_array_size && vt->compare);

	if(1 >= vt->el_count)
		return;

	parent_index = HEAP_PARENT_INDEX(vt->el_count - 1);
	len = vt->el_count;

	while(1)
	{
		vector_inter_adjust(vt, parent_index, len, vt->el_array[parent_index]);

		if(0 == parent_index)
			return;

		parent_index -= 1;
	}
}

/*
*
*弹出堆中最大的元素,放在数组的最未尾
*
*/
static __INLINE__ void vector_inter_pop(myvector_t * vt, size_t last)
{
	myvector_element * el_v = NULL;

	assert(vt && vt->compare && vt->el_array && vt->el_array_size && last < vt->el_count);

	if(0 == vt->el_count)
		return;

	//头一个节点为空洞，将头一个节点的值存至尾节点
	el_v = vt->el_array[last];
	vt->el_array[last] = vt->el_array[0];
	vector_inter_adjust(vt, 0, last, el_v);
}


/*
*
*构造vector
*
*/
HMYVECTOR MyVectorConstruct(HMYMEMPOOL hm, size_t size, myobj_ops * data_ops, ALG_COMPARE compare)
{
	return (HMYVECTOR)__vector_inter_create(hm, size, data_ops, compare);
	/*myvector_t * vt = (myvector_t *)MyMemPoolMalloc(hm, sizeof(*vt));
	if(NULL == vt)
		return NULL;

	memset(vt, 0, sizeof(*vt));
	vt->hm = hm;
	vt->compare = compare;

	if(data_ops)
	{
		vt->data_ops.construct = data_ops->construct;
		vt->data_ops.copy = data_ops->copy;
		vt->data_ops.destruct = data_ops->destruct;
	}

	if(0 == __vector_inter_init_array(vt, size))
		return (HMYVECTOR)vt;

	__vector_inter_destroy(vt);

	return NULL;*/
}

/*
*
*析构vector
*
*/
void MyVectorDestruct(HMYVECTOR vt)
{
	if(NULL == vt)
		return;

	__vector_inter_destroy(vt);
}

/*
*
*添加节点(在未尾添加)
*
*/
HMYVECTOR_ITER MyVectorAdd(HMYVECTOR vt, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;

	if(NULL == vt)
		return NULL;

	el = __vector_inter_add(vt, data, data_size);

	return (HMYVECTOR_ITER)el;
}

/*
*
*删除指定的节点
*
*/
int MyVectorDel(HMYVECTOR vt, size_t index)
{
	if(NULL == vt || index >= vt->el_count || NULL == vt->el_array || 0 == vt->el_array_size)
		return -1;

	__vector_inter_del(vt, index);

	return 0;
}

/*
*
*删除指定的节点
*
*/
void MyVectorClear(HMYVECTOR vt)
{
	if(NULL == vt || 0 >= vt->el_count || NULL == vt->el_array || 0 == vt->el_array_size)
		return;

	__vector_inter_clear(vt);
}

/*
*
*获取指定的节点
*
*/
HMYVECTOR_ITER MyVectorGetIndex(HMYVECTOR vt, size_t index)
{
	if(NULL == vt || NULL == vt->el_array)
		return NULL;

	return (HMYVECTOR_ITER)__vector_inter_getindex(vt, index);
}

/*
*
*获取迭代器的数据
*
*/
void * MyVectorGetIterData(HMYVECTOR_ITER el)
{
	if(NULL == el)
		return NULL;

	return __vector_inter_get_iter_data(el);
}

/*
*
*获取迭代器的数据长度
*
*/
size_t MyVectorGetIterDataSize(HMYVECTOR_ITER el)
{
	if(NULL == el)
		return 0;

	return __vector_inter_get_iter_datasize(el);
}

/*
*
*获取指定的节点数据
*
*/
void * MyVectorGetIndexData(HMYVECTOR vt, size_t index, size_t * data_size)
{
	myvector_element * el = NULL;

	if(NULL == vt || NULL == vt->el_array)
		return NULL;

	el = __vector_inter_getindex(vt, index);
	if(NULL == el)
		return NULL;

	if(data_size)
		*data_size = el->data_size;

	return el->data;
}

/*
*
*获取头结点
*
*/
HMYVECTOR_ITER MyVectorGetHead(HMYVECTOR vt)
{
	if(NULL == vt || NULL == vt->el_array || 0 == vt->el_count || 0 == vt->el_array_size)
		return NULL;

	return __vector_get_head(vt);
}

/*
*
*获取尾结点
*
*/
HMYVECTOR_ITER MyVectorGetTail(HMYVECTOR vt)
{
	if(NULL == vt || NULL == vt->el_array || 0 == vt->el_count || 0 == vt->el_array_size)
		return NULL;

	return __vector_get_tail(vt);
}

/*
*
*获取尾结点
*
*/
int MyVectorGetTailData(HMYVECTOR vt, void ** data, size_t * data_size)
{
	if(NULL == vt || NULL == vt->el_array || 0 == vt->el_count || 0 == vt->el_array_size)
		return -1;

	if(data)
		*data = vt->el_array[vt->el_count - 1]->data;
	if(data_size)
		*data_size = vt->el_array[vt->el_count - 1]->data_size;

	return 0;
}

/*
*
*获取节点个数
*
*/
size_t MyVectorGetCount(HMYVECTOR vt)
{
	if(NULL == vt || NULL == vt->el_array)
		return 0;

	return __vector_inter_get_count(vt);
}

/*
*
*获取vector的容量
*
*/
size_t MyVectorGetSize(HMYVECTOR vt)
{
	if(NULL == vt || NULL == vt->el_array)
		return 0;

	return vt->el_array_size;
}

/*
*
*重新设置vector的大小
*
*/
int MyVectorResize(HMYVECTOR vt, int vector_size)
{
	if(NULL == vt)
		return -1;

	return __vector_inter_resize(vt, vector_size);
}

/*
*
*交换两个vector元素
*
*/
int MyVectorSwitch(HMYVECTOR vt, const size_t index1, const size_t index2)
{
	myvector_element * el = NULL;
	if(NULL == vt || NULL == vt->el_array || index1 >= vt->el_count || index2 >= vt->el_count || index1 == index2)
		return -1;

	el = vt->el_array[index1];
	vt->el_array[index1] = vt->el_array[index2];
	vt->el_array[index2] = el;

	return 0;
}

/*
*
*利用vector生成一个堆
*
*/
int MyVectorHeapMake(HMYVECTOR vt)
{
	if(NULL == vt || NULL == vt->compare || NULL == vt->el_array || 0 == vt->el_array_size)
		return -1;

	vector_inter_make_heap(vt);

	return 0;
}

/*
*
*堆排序算法
*
*/
int MyVectorHeapSort(HMYVECTOR vt)
{
	size_t last_index = 0;
	if(NULL == vt || NULL == vt->compare || NULL == vt->el_array || 0 == vt->el_array_size)
		return -1;

	if(1 >= vt->el_count)
		return 0;

	last_index = vt->el_count - 1;

	while(last_index)
	{
		vector_inter_pop(vt, last_index);

		last_index -= 1;
	}

	return 0;
}

/*
*
*往堆中压入一个元素
*
*/
int MyVectorHeapPush(HMYVECTOR vt, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;
	if(NULL == vt || NULL == vt->el_array || 0 == vt->el_array_size || NULL == vt->compare)
		return -1;

	el = __vector_inter_add(vt, data, data_size);
	if(NULL == el)
		return -1;

	vector_inter_push(vt, vt->el_count - 1, 0, el);

	return 0;
}

/*
*
*从堆中弹出一个元素
*
*/
int MyVectorHeapPop(HMYVECTOR vt)
{
	if(NULL == vt || NULL == vt->el_array || 0 == vt->el_array_size)
		return -1;

	if(1 >= vt->el_count)
		return 0;

	vector_inter_pop(vt, vt->el_count - 1);

	return 0;
}


/*
*
*检查一个堆是否合法
*
*/
void MyVectorHeapExamin(HMYVECTOR vt)
{
	size_t parent_index = 0;
	size_t len = 0;
	size_t second_child = 0;

	assert(NULL != vt && NULL != vt->el_array && 0 != vt->el_array_size && NULL != vt->compare);

	len = vt->el_count;

	second_child = HEAP_SECOND_CHILD_INDEX(parent_index);
	while(second_child < len)
	{
		assert(vt->compare(vt->el_array[parent_index]->data, vt->el_array[second_child]->data, vt->context) >= 0);
		assert(vt->compare(vt->el_array[parent_index]->data, vt->el_array[second_child - 1]->data, vt->context) >= 0);

		parent_index += 1;
		second_child = HEAP_SECOND_CHILD_INDEX(parent_index);
	}
	if(second_child == len)
	{
		assert(vt->compare(vt->el_array[parent_index]->data, vt->el_array[second_child - 1]->data, vt->context) >= 0);
	}
}

/*
*
*检查一个数组是否正确地被排序
*
*/
void MyVectorHeapExaminSortOK(HMYVECTOR vt)
{
	size_t i = 0;
	assert(NULL != vt && NULL != vt->el_array && 0 != vt->el_array_size && NULL != vt->compare);

	for(i = 1; i < vt->el_count ; i ++)
	{
		assert(vt->compare(vt->el_array[i]->data, vt->el_array[i - 1]->data, vt->context) >= 0);
	}
}

/*
*
*打印出vector
*
*/
void MyVectorPrint(HMYVECTOR vt)
{
	size_t i = 0;
	if(NULL == vt || NULL == vt->el_array || 0 == vt->el_array_size)
		return;

	for(i = 0; i < vt->el_count; i ++)
	{
		if(0 == i % 16)
			printf("\n");
#ifdef _MBCSV6
		printf("%d ", (long)vt->el_array[i]->data);
#else
#ifdef WIN32
		printf("%d ", (long long)vt->el_array[i]->data);
#else
		printf("%d ", (long)vt->el_array[i]->data);
#endif
#endif
	}

	printf("total:%d\r\n", vt->el_count);
}


#include "myheap.c"
























/**
 *
 * @file MyHeapAlg.c 堆相关的算法
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "MyAlgorithm.h"

#include <assert.h>
#include <stdlib.h>

#include "__def_fun_def.h"
#include "mymempool.h"
#include "myutility.h"


/**
 * @brief 计算第二个子节点的坐标
 *
 * 0 - 1 2  2(i + 1) - 1 2(i + 1)
 * 1 - 3 4
 * 2 - 5 6
 */
#define HEAP_SECOND_CHILD_INDEX(x) (2 * ((x) + 1))

/**
 * @brief 计算父节点的坐标
 */
#define HEAP_PARENT_INDEX(x) (assert(x), ((x) - 1)/2)


static __INLINE__ int heap_push(void * buf, size_t step_size,
					 size_t hole_index, size_t top_index, const void * value, size_t value_size,
					 ALG_COMPARE compare, ALG_COPY copier,
					 const void * context)
{
	size_t parent = 0;

	assert(buf && value && compare && copier);
	assert(value_size <= step_size);

	/*
	* 自下(hole_index)而上(top_index)寻找value合适的放入位置
	*/
	while(hole_index > top_index)
	{
		parent = HEAP_PARENT_INDEX(hole_index);

		/*
		* value大于parent,则把parent拉到hole_index的位置
		*/
		if(compare(value, GET_INDEX_PTR(buf, parent, step_size), context) > 0)
		{
			copier(GET_INDEX_PTR(buf, hole_index, step_size), GET_INDEX_PTR(buf, parent, step_size), step_size, context);

			hole_index = parent;
		}
		else /* 否则退出循环 */
			break;
	}

	copier(GET_INDEX_PTR(buf, hole_index, step_size), value, (step_size < value_size) ? step_size : value_size, context);

	return 0;
}

static __INLINE__ int adjust_heap(void * buf, size_t len, size_t step_size, 
					   size_t hole_index, const void * value, size_t value_size,
					   ALG_COMPARE compare, ALG_COPY copier,
					   const void * context)
{
	size_t top_index = hole_index;
	size_t second_index = HEAP_SECOND_CHILD_INDEX(hole_index);

	assert(buf && value && compare && copier);

	/*
	* 自上(hole_index)而下地调整堆
	*/
	while(second_index < len)
	{
		/*
		* 取出小的一个节点往上拉
		*/
		if(compare(GET_INDEX_PTR(buf, second_index - 1, step_size), GET_INDEX_PTR(buf, second_index, step_size), context) > 0)
			second_index --;

		/*
		* 将second_index里的数据拷到hole_index里
		*/
		copier(GET_INDEX_PTR(buf, hole_index, step_size), GET_INDEX_PTR(buf, second_index, step_size), step_size, context);

		hole_index = second_index;
		second_index = HEAP_SECOND_CHILD_INDEX(hole_index);
	}

	/*
	* 处理只有左孩子,没有右孩子的边界情况
	*/
	if(second_index == len)
	{
		/*
		* 直接将左孩子往上拉
		*/
		copier(GET_INDEX_PTR(buf, hole_index, step_size), GET_INDEX_PTR(buf, second_index - 1, step_size), step_size, context);

		hole_index = second_index - 1;
	}

	heap_push(buf, step_size, hole_index, top_index, value, value_size, compare, copier, context);

	return 0;
}

static __INLINE__ int heap_pop(void * buf, size_t len, size_t step_size,					
					ALG_COMPARE compare, ALG_COPY copier,
					const void * context, void * swap_buf, const size_t swap_buf_size)
{
	assert(buf && compare && copier && swap_buf && swap_buf_size >= step_size);

	copier(swap_buf, GET_INDEX_PTR(buf, len - 1, step_size), step_size, context);
	copier(GET_INDEX_PTR(buf, len - 1, step_size), GET_INDEX_PTR(buf, 0, step_size), step_size, context);

	adjust_heap(buf, len - 1, step_size, 0, swap_buf, swap_buf_size, compare, copier, context);

	return 0;
}

static __INLINE__ int __make_heap__(void * buf, size_t len, size_t step_size,
					  ALG_COMPARE compare, ALG_COPY copier,
					  const void * context, void * swap_buf, const size_t swap_buf_size)
{
	size_t parent = HEAP_PARENT_INDEX((len - 1));

	assert(buf && compare && copier);
	assert(swap_buf && swap_buf_size >= step_size && len >= 2);

	while(1)
	{
		copier(swap_buf, GET_INDEX_PTR(buf, parent, step_size), step_size, context);
		adjust_heap(buf, len, step_size, parent, swap_buf, swap_buf_size, compare, copier, context);
		if(0 == parent)
			return 0;
		parent --;
	}
}


/**
 *
 * @brief 将一个堆排序
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的数组中元素的个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param compare:比较回调函数(不可为空)
 * @param copier:拷贝回调函数(可为空)
 * @param context:用户自定义的上下文数据(可为空)
 * @param swap_buf:生成堆过程中用到的临时空间(可为null)
 * @param swap_buf_size:swap_buf的大小
 *
 */
int MyAlgHeapSort(void * buf, size_t len, size_t step_size,
				  ALG_COMPARE compare, ALG_COPY copier,
				  const void * context, void * swap_buf, size_t swap_buf_size)
{
	int need_free_swap = 0;
	if(NULL == buf || NULL == compare)
		return -1;

	if(NULL == copier)
		copier = __def_alg_copy_;

	if(NULL == swap_buf || swap_buf_size < step_size)
	{
		swap_buf = MyMemPoolMalloc(NULL, step_size);
		swap_buf_size = step_size;

		need_free_swap = 1;
	}

	/*
	* 依次pop即完成了排序
	*/
	while(len >= 2)
	{
		heap_pop(buf, len --, step_size, compare, copier, context, swap_buf, swap_buf_size);
	}

	if(need_free_swap)
		MyMemPoolFree(NULL, swap_buf);

	return 0;
}

/**
 *
 * @brief 生成堆
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的数组中元素的个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param compare:比较回调函数
 * @param copier:拷贝回调函数
 * @param context:用户自定义的上下文数据
 * @param swap_buf:生成堆过程中用到的临时空间(可为null)
 * @param swap_buf_size:swap_buf的大小
 *
 */
int MyAlgMakeHeap(void * buf, size_t len, size_t step_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context, void * swap_buf, size_t swap_buf_size)
{
	if(NULL == buf || NULL == compare)
		return -1;

	if(len <= 1)
		return 0;

	if(NULL == copier)
		copier = __def_alg_copy_;

	if(swap_buf && swap_buf_size >= step_size)
	{
		__make_heap__(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);

		return 0;
	}

	swap_buf = MyMemPoolMalloc(NULL, step_size);
	swap_buf_size = step_size;

	__make_heap__(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);

	MyMemPoolFree(NULL, swap_buf);

	return 0;
}

/**
 *
 * @brief 堆排序
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的数组中元素的个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param compare:比较回调函数
 * @param copier:拷贝回调函数
 * @param context:用户自定义的上下文数据
 * @param swap_buf:生成堆过程中用到的临时空间(可为null)
 * @param swap_buf_size:swap_buf的大小
 *
 */
int MyAlgHeapMakeAndSort(void * buf, size_t len, size_t step_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context, void * swap_buf, size_t swap_buf_size)
{
	MyAlgMakeHeap(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);
	MyAlgHeapSort(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);

	return 0;
}

/**
 *
 * @brief 从堆中弹出一个元素
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:数组中元素的个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param compare:比较回调函数
 * @param copier:拷贝回调函数
 * @param context:用户自定义的上下文数据
 * @param swap_buf:生成堆过程中用到的临时空间(可为null)
 * @param swap_buf_size:swap_buf的大小
 *
 */
int MyAlgHeapPop(void * buf, size_t len, size_t step_size,
						ALG_COMPARE compare, ALG_COPY copier,
						const void * context, void * swap_buf, size_t swap_buf_size)
{
	if(NULL == buf || NULL == compare)
		return -1;

	if(NULL == copier)
		copier = __def_alg_copy_;

	if(swap_buf && swap_buf_size < step_size)
	{
		heap_pop(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);

		return 0;
	}

	swap_buf = MyMemPoolMalloc(NULL, step_size);
	swap_buf_size = step_size;

	heap_pop(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);

	MyMemPoolFree(NULL, swap_buf);

	return 0;
}

/**
 *
 * @brief 推一个元素至堆中
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:数组中的元素个数
 * @param step_size:每个元素的大小
 * @param value:要加入的值
 * @param value_size:value的大小
 * @param context:用户自定义的上下文数据
 *
 */
int MyAlgHeapPush(void * buf, size_t len, size_t step_size,
						 const void * value, size_t value_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context)
{
	if(NULL == buf || NULL == value || NULL == compare || value_size > step_size || 0 == len)
		return -1;

	if(NULL == copier)
		copier = __def_alg_copy_;

	heap_push(buf, step_size, len - 1, 0, value, value_size, compare, copier, context);

	return 0;
}

/**
 *
 * @brief 调整一个堆
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的元素个数
 * @param step_size:每个元素的大小
 * @param hole_index:空洞节点的索引(0表示第一个节点)
 * @param value:值
 * @param value_size:值的大小
 * @param compare:比较回调(不可为空)
 * @param copier:如何拷贝(可为空)
 * @param context:用户自定义的上下文数据
 *
 */
/*int MyAlgAdjustHeap(void * buf, size_t len, size_t step_size, 
						   size_t hole_index, const void * value, size_t value_size,
						   ALG_COMPARE compare, ALG_COPY copier,
						   const void * context)
{
	if(NULL == buf || NULL == value || NULL == compare)
		return -1;

	if(NULL == copier)
		copier = __def_alg_copy_;

	adjust_heap(buf, len, step_size, hole_index, value, value_size, compare, copier, context);

	return 0;
}*/

/**
 *
 * @brief 检查一个序列是否为一个堆
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的元素个数
 * @param step_size:每个元素的大小
 * @param compare:比较回调(不可为空)
 * @param context:用户自定义的上下文数据
 *
 */
int MyAlgExaminHeap(void * buf, size_t len, size_t step_size,
					ALG_COMPARE compare,
					const void * context)
{
	size_t parent_index = 0;
	size_t second_child = 0;

	if(NULL == buf || NULL == compare)
		return -1;

	if(len <= 1)
		return 0;

	second_child = HEAP_SECOND_CHILD_INDEX(parent_index);
	while(second_child < len)
	{
		/*
		* 只要子节点不大于父节点,都是合法的
		*/
		assert(compare(GET_INDEX_PTR(buf, second_child, step_size), GET_INDEX_PTR(buf, parent_index, step_size), context) <= 0);
		assert(compare(GET_INDEX_PTR(buf, second_child - 1, step_size), GET_INDEX_PTR(buf, parent_index, step_size), context) <= 0);

		parent_index += 1;
		second_child = HEAP_SECOND_CHILD_INDEX(parent_index);
	}

	if(second_child == len)
	{
		assert(compare(GET_INDEX_PTR(buf, second_child - 1, step_size), GET_INDEX_PTR(buf, parent_index, step_size), context) <= 0);
	}

	return 0;
}

/**
 *
 * @brief 检查一个序列是否已经正确地被排序
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的元素个数
 * @param step_size:每个元素的大小
 * @param compare:比较回调(不可为空)
 * @param context:用户自定义的上下文数据
 *
 */
int MyAlgSortOK(void * buf, size_t len, size_t step_size,
				ALG_COMPARE compare,
				const void * context)
{
	size_t i = 0;

	if(NULL == buf || NULL == compare)
		return -1;

	if(len <= 1)
		return 0;

	for(i = 1; i < len; i ++)
	{
		/*
		* 只要前一个不大于后一个,就说明被正确排序了
		*/
		if(compare(GET_INDEX_PTR(buf, i - 1, step_size), GET_INDEX_PTR(buf, i, step_size), context) > 0)
		{
			int a = 0;
			int b = a;
		}
		assert(compare(GET_INDEX_PTR(buf, i - 1, step_size), GET_INDEX_PTR(buf, i, step_size), context) <= 0);
	}

	return 0;
}

















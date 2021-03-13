/**
 *
 * @file MyBinarySearch1.c 二分查找
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "MyAlgorithm.h"

#include <assert.h>

#include "myutility.h"


/**
 * @brief 在一个有序序列里做二分查找
 */
static int __alg_binary_search(const void * buf, size_t len, size_t step_size,
							   const void * key,
							   ALG_COMPARE compare, size_t * pindex,
							   const void * context)
{
	size_t hi = len;
	size_t lo = 0;

	assert(buf && len && step_size && compare);

	while(hi > lo)
	{
		size_t mid = (hi + lo) / 2;
		int ret = compare(key, GET_INDEX_PTR(buf, mid, step_size), context);

		if(ret > 0)
		{
			lo = mid + 1;
			continue;
		}
		else if(ret < 0)
		{
			hi = mid;
			continue;
		}
		else
		{
			if(pindex)
				*pindex = mid;
			return 0;
		}
	}

	assert(hi == lo);

	if(pindex)
		*pindex = lo;

	return -1;
}


/**
 *
 * @brief 在一个有序序列里做二分查找
 *
 * @param buf:有序序列数据缓冲区起始地址
 * @param len:有序序列中的元素个数
 * @param step_size:每个元素的大小
 * @param key:要查找的关键字
 * @param compare:比较回调(不可为空)
 * @param pindex:如果找到,返回元素的位置,
 *               如果找不到,则返回元素应添加的位置,例如:
 *               1 3 4 6 7 --- 源序列,查找2,
 *               pindex将被置成[1],即表示2应添加到1与3之间
 *
 * @retval 0:成功
 * @retval 其它:失败
 *
 */
int MyBinarySearch1(const void * buf, size_t len, size_t step_size,
					const void * key,
					ALG_COMPARE compare, size_t * pindex,
					const void * context)
{
	if(NULL == buf || 0 == len || 0 == step_size || NULL == compare)
		return -1;

	return __alg_binary_search(buf, len, step_size, key, compare, pindex, context);
}


///**
// * @brief
// *  > 0  表示 key1 大于 key2
// *  == 0 表示 key1 等于 key2
// *  < 0  表示 key1 小于 key2
// *
// * @param context:用户自定义的上下文数据
// */
//typedef int (*BINSEARCH_COMPARE)(const void * key, unsigned int key_sz, 
//								 const void * data2, 
//								 const void * context, unsigned int context_sz);

/**
 * @brief 在一个有序序列里做二分查找
 *
 * @param buf:有序序列数据缓冲区起始地址
 * @param len:有序序列中的元素个数
 * @param step_size:每个元素的大小
 * @param key:要查找的关键字
 * @param key_sz:关键字缓冲的长度
 * @param compare:比较回调(不可为空)
 * @param pindex:如果找到,返回元素的位置,
 *               如果找不到,则返回元素应添加的位置,例如:
 *               1 3 4 6 7 --- 源序列,查找2,
 *               pindex将被置成[1],即表示2应添加到1与3之间
 * @param context:上下文信息
 * @param context_sz:上下文信息缓冲的长度
 *
 * @retval 0:成功
 * @retval 其它:失败
 */
int MyBinarySearch(const void * buf, unsigned int len, unsigned int step_size,
				   const void * key, unsigned int key_sz,
				   BINSEARCH_COMPARE compare, unsigned int * pindex,
				   const void * context, unsigned int context_sz)
{
	unsigned int hi = len;
	unsigned int lo = 0;

	if(NULL == buf || 0 == len || 0 == step_size || NULL == compare)
		return -1;

	while(hi > lo)
	{
		unsigned int mid = (hi + lo) / 2;
		int ret = compare(key, key_sz, GET_INDEX_PTR(buf, mid, step_size), context, context_sz);

		if(ret > 0)
		{
			lo = mid + 1;
			continue;
		}
		else if(ret < 0)
		{
			hi = mid;
			continue;
		}
		else
		{
			if(pindex)
				*pindex = mid;
			return 0;
		}
	}

	assert(hi == lo);

	if(pindex)
		*pindex = lo;

	return -1;
}



















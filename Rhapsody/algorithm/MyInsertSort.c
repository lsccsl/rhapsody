/**
 *
 * @file MyInsertSort.c 各种算法函数声明
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "MyAlgorithm.h"
#include <stdlib.h>
#include <assert.h>
#include "__def_fun_def.h"
#include "mymempool.h"


/**
 *
 * @brief:插入式排序
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:缓冲区的元素的个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param compare:比较回调(不可为空)
 * @param swaper:如何交换两个元素的数据(可为空)
 * @param move:如何移动一数据(类似于memmove动作,可为空)
 * @param copier:如何拷贝数据(可为空)
 * @param context:用户自定义的上下文数据(可为空)
 * @param swap_buf:在排序过程中用于保存临时数据,大小为一个元素的大小即可
 * @param swap_buf_size:swap_buf的实际大小
 *
 */
int MyAlgInsertSort(void * buf, size_t len, size_t step_size,
					ALG_COMPARE compare, ALG_SWAP swaper, ALG_MOVE move, ALG_COPY copier,
					const void * context, void * swap_buf, size_t swap_buf_size)
{
	size_t i = 0;
	size_t k = 0;
	unsigned char * start = (unsigned char *)buf;

	unsigned char * temp = NULL;
	int need_release = 0;
	if(swap_buf && swap_buf_size >= step_size)
		temp = (unsigned char *)swap_buf;
	else
	{
		temp = (unsigned char *)MyMemPoolMalloc(NULL, step_size);
		need_release = 1;
	}

	assert(start && compare && temp);

	if(NULL == swaper)
		swaper = __def_alg_swap_;

	if(NULL == move)
		move = __def_alg_move_;

	if(NULL == copier)
		copier = __def_alg_copy_;

	for(i = 1; i < len; i ++)
	{
		unsigned char * current = (start + i * step_size);

		/* 
		* 将要插入的数据保存到temp 
		*/
		copier(temp, current, step_size, context);

		/*
		* 如果要加入的值比第一个元素还小(第一个在有序序列中总是最小的).直接做后移拷贝
		*/
		if(compare(start, current, context) > 0)
		{
			move(start + step_size, start, i * step_size, context);
			copier(start, temp, step_size, context);
			continue;
		}

		/* 
		* 否则顺序比较,插入合适的位置 
		*/
		k = i - 1;
		while(compare(start + k * step_size, current, context) > 0)
		{
			k --;
		}

		/* 
		* 计算出正确的插入位置
		*/
		k += 1;

		/*
		* 找到了k位置为正确的插入位置,所有数据往后移1个step_size
		*/
		move(start + (k + 1) * step_size,
			start + k * step_size, (i - k) * step_size, context);

		/*
		* 保存要插入的数据到k位置
		*/
		copier(start + k * step_size, temp, step_size, context);
	}

	if(need_release)
		MyMemPoolFree(NULL, temp);

	return 0;
}













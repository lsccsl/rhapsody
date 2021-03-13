/**
 *
 * @file def_fun_def.c 各种回调函数的默认定义
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "__def_fun_def.h"
#include <string.h>
#include "myutility.h"


/**
 *
 * @brief 描述如何将src里的数据拷贝给dst
 *
 * @param context:用户自定义的上下文数据
 *
 */
int __def_alg_copy_(void * dst,
					const void * src, const size_t size_copy,
					const void * context)
{
	memcpy(dst, src, size_copy);

	MY_UNUSED_ARG(context);

	return 0;
}

/**
 *
 * @brief 描述如何将src起始的一段数据拷贝给dst起始的一段空间
 *
 * @param context:用户自定义的上下文数据
 *
 */
int __def_alg_move_(void * dst,
					const void * src, const size_t size_move,
					const void * context)
{
	memmove(dst, src, size_move);

	MY_UNUSED_ARG(context);

	return 0;
}

/**
 *
 * @brief 描述如何将data1与data2的数据互换
 *
 * @param context:用户自定义的上下文数据
 *
 */
int __def_alg_swap_(void * data1,
					void * data2, const size_t data_size,
					const void * context)
{
#define SWAP(x, y) do{\
		x = x ^ y;\
		y = x ^ y;\
		x = x ^ y;\
}while(0)

	size_t i = 0;
	size_t loop = data_size / 4;
	size_t arith_compliment = data_size % 4;

	MY_UNUSED_ARG(context);

	for(i = 0; i < loop; i ++)
	{
		SWAP(*((unsigned int *)data1 + i), *((unsigned int *)data2 + i));
	}

	if(3 == arith_compliment)
	{
		SWAP(*((unsigned short *)((unsigned int *)data1 + i)), *((unsigned short *)((unsigned int *)data2 + i)));
		SWAP(*((unsigned char *)data1 + data_size - 1), *((unsigned char *)data2 + data_size - 1));
	}

	if(2 == arith_compliment)
	{
		SWAP(*((unsigned short *)((unsigned int *)data1 + i)), *((unsigned short *)((unsigned int *)data2 + i)));
	}

	if(1 == arith_compliment)
	{
		SWAP(*((unsigned char *)data1 + data_size - 1), *((unsigned char *)data2 + data_size - 1));
	}

	return 0;
}













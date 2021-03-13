#ifndef __MYUTILITY_H__
#define __MYUTILITY_H__


#include <assert.h>

#include "myerr_def.h"


/**
 * 不使用的参数
 */
#define MY_UNUSED_ARG(x) do{}while(&x == 0)

/**
 * @brief 计算索引i的指针值,x为起始,s为指针下走的跨度
 */
#define GET_INDEX_PTR(x, i, s) (/*assert(0 == (s % 4)),*/ ((unsigned char *)(x) + (i) * (s)))

/**
 * @brief 计算对齐
 */
#define CAL_ALIGMENT(x, ali) ((ali) - (x) % (ali))

/**
 * @brief unsgined int转成大码,存入一个数组
 */
#define uint_to_big_endian(_uint_val, _dst_array, _dst_array_size) \
	do{\
		assert((_dst_array_size) >= 4);\
		(_dst_array)[0] = ((_uint_val)>>24) & 0xff;\
		(_dst_array)[1] = ((_uint_val)>>16) & 0xff;\
		(_dst_array)[2] = ((_uint_val)>>8) & 0xff;\
		(_dst_array)[3] = (_uint_val) & 0xff;\
	}while(0)

/**
 * @brief 将一个大码形式保存的无符号整形数转存到一个unsigned int 变量里
 */
#define array_to_uint_as_big_endian(_src_array, _src_array_size, _uint_val) \
	do{\
		assert((_src_array_size) >= 4);\
		_uint_val = ((_src_array)[0]<<24) | ((_src_array)[1]<<16) | ((_src_array)[2]<<8) | (_src_array)[3];\
	}while(0)

/**
 * @brief 将一个2字节的值存入数组
 */
#define put_2byte(_val, _dst_array, _dst_array_sz) \
	do{\
		assert((_dst_array_sz) >= 2);\
		(_dst_array)[0] = (_val)>>8; \
		(_dst_array)[1] = (_val); \
	}while(0)

/**
 * @brief 从一个数组里取出一个2字节的值
 */
#define get_2byte(_src_array, _src_array_sz, _val) \
	do{\
		(_val) = ((_src_array)[0]<<8) | (_src_array)[1];\
	}while(0)


#endif












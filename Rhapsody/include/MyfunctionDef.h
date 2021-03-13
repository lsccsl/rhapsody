/**
 *
 * @file MyfunctionDef.h 各种回调函数的原型声明
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __MYFUNCTIONDEF_H__
#define __MYFUNCTIONDEF_H__


#include <stdlib.h>


/**
 * @brief
 *  > 0  表示 key1 大于 key2
 *  == 0 表示 key1 等于 key2
 *  < 0  表示 key1 小于 key2
 *
 * @param context:用户自定义的上下文数据
 */
typedef int (*ALG_COMPARE)(const void * data1, const void * data2, const void * context);

/**
 *
 * 描述如何将src里的数据拷贝给dst
 *
 * @param context:用户自定义的上下文数据
 *
 */
typedef int (*ALG_COPY)(void * dst,
						const void * src, const size_t size_copy,
						const void * context);

/**
 *
 * 描述如何将src起始的一段数据拷贝给dst起始的一段空间
 *
 * @param context:用户自定义的上下文数据
 *
 */
typedef int (*ALG_MOVE)(void * dst, 
						const void * src, const size_t size_move,
						const void * context);

/**
 *
 * 描述如何将data1与data2的数据互相
 *
 * @param context:用户自定义的上下文数据
 *
 */
typedef int (*ALG_SWAP)(void * data1,
						void * data2, const size_t data_size,
						const void * context);


#endif














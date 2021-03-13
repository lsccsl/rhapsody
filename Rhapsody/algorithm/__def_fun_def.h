/**
 *
 * @file def_fun_def.h 各种回调函数的默认定义
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __DEF_FUN_DEF_H__
#define __DEF_FUN_DEF_H__


#include <stdlib.h>


/**
 *
 * @brief 描述如何将src里的数据拷贝给dst
 *
 * @param context:用户自定义的上下文数据
 *
 */
extern int __def_alg_copy_(void * dst,
					const void * src, const size_t size_copy,
					const void * context);

/**
 *
 * @brief 描述如何将src起始的一段数据拷贝给dst起始的一段空间
 *
 * @param context:用户自定义的上下文数据
 *
 */
extern int __def_alg_move_(void * dst,
					const void * src, const size_t size_move,
					const void * context);

/**
 *
 * @brief 描述如何将data1与data2的数据互换
 *
 * @param context:用户自定义的上下文数据
 *
 */
extern int __def_alg_swap_(void * data1, 
					void * data2, const size_t data_size,
					const void * context);

#endif













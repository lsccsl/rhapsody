/**
 *
 * @file __vector_inter.h vector的内部接口定义
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef ____VECTOR_INTER_H__
#define ____VECTOR_INTER_H__


#include <string.h>

#include "myobj.h"
#include "MyAlgorithm.h"
#include "mymempool.h"


typedef struct __myvector_element_
{
	/*
	* data缓冲区的实际大小
	*/
	size_t buf_size;

	/*
	* 用户数据所使用的大小
	*/
	size_t data_size;

	/*
	* 用户数据缓冲区的起始地址
	*/
	void * data;
}myvector_element;

typedef struct __myvector_t_
{
	myvector_element ** el_array;
	size_t el_array_size;
	size_t el_count;

	myobj_ops data_ops;

	/*
	* 比较回调与相应的用户上下文数据
	*/
	ALG_COMPARE compare;
	/* 预留 */
	void * context;

	HMYMEMPOOL hm;
}myvector_t;


/**
 * 获取第一个节点
 */
#define __vector_get_head(vt) (assert(vt && \
	((myvector_t *)(vt))->el_array && \
	((myvector_t *)(vt))->el_count && \
	((myvector_t *)(vt))->el_array_size), \
	((myvector_t *)(vt))->el_array[0])

/**
 * 获取第一个节点
 */
#define __vector_get_head_data(vt) (__vector_get_head(vt)->data)


/**
 * 获取最后一个节点
 */
#define __vector_get_tail(vt) (assert(vt && \
	((myvector_t *)(vt))->el_array && \
	((myvector_t *)(vt))->el_count && \
	((myvector_t *)(vt))->el_array_size), \
	((myvector_t *)vt)->el_array[((myvector_t *)(vt))->el_count - 1])

/**
 * 获取最后一个节点的值
 */
#define __vector_get_tail_data(vt) (__vector_get_tail(vt)->data)


/**
 * 获取迭代器的数据域
 */
#define __vector_inter_get_iter_data(el) (assert(el), ((myvector_element *)(el))->data)

/**
 * 获取迭代器的数据域的长度
 */
#define __vector_inter_get_iter_datasize(el) (assert(el), ((myvector_element *)(el))->data_size)

/**
 * 获取vector里元素的个数
 */
#define __vector_inter_get_count(vt) (assert(vt && ((myvector_t *)(vt))->el_array), ((myvector_t *)(vt))->el_count)

/**
 * 获取数组的起始地址
 */
#define __vector_inter_get_array(vt) (assert(vt && ((myvector_t *)(vt))->el_array), ((myvector_t *)(vt))->el_array)

/**
 * 获取数组中每个元素的大小
 */
#define __vector_inter_get_array_step_size(vt) (assert(vt), sizeof((vt)->el_array[0]))

/**
 * 获取指定位置元素
 */
#define __vector_inter_getindex(vt, index) (assert(vt && ((myvector_t *)(vt))->el_array),\
	(index >= ((myvector_t *)(vt))->el_count) ? NULL : ((myvector_t *)(vt))->el_array[index])

/**
 * 获取指定位置元素
 */
#define __vector_inter_get_index_data(vt, index) (assert(__vector_inter_get_count((vt))), (__vector_inter_getindex((vt), (index)))->data)

/**
 * 销毁一个节点
 */
#define __vector_inter_destroy_element(vt, el) do{\
	assert((vt) && (el));\
	\
	if((vt)->data_ops.destruct && (el)->data_size && (el)->data)\
		(vt)->data_ops.destruct((el)->data, (el)->data_size);\
	\
	MyMemPoolFree((vt)->hm, (el));\
	}while(0)

/**
 * 销毁vector
 */
#define __vector_inter_destroy(vt) do{\
		if(NULL != (vt))\
			__vector_inter_free_array((vt));\
		MyMemPoolFree((vt)->hm, (vt));\
	}while(0)

/**
 * 释放数组
 */
#define __vector_inter_free_array(vt) do{\
	assert(vt && vt->el_array && vt->el_array_size);\
	\
	__vector_inter_clear(vt);\
	\
	MyMemPoolFree(vt->hm, vt->el_array);\
	}while(0)

/**
 * 删除index处的节点
 */
#define __vector_inter_del(vt, index) do{\
		myvector_element * el = NULL;\
		\
		assert((vt) && index < (vt)->el_count && (vt)->el_array && (vt)->el_array_size);\
		\
		el = (vt)->el_array[(index)];\
		\
		if(index < (vt)->el_count - 1)\
			memcpy((vt)->el_array + (index), (vt)->el_array + (index) + 1, ((vt)->el_count - (index) - 1)*sizeof(myvector_element*));\
		\
		(vt)->el_count --;\
		\
		if(NULL != el)\
			__vector_inter_destroy_element((vt), el);\
	}while(0)

#define __vector_inter_get_temp_it(it, data_size, data) myvector_element __el__ = {0, data_size, data};HMYVECTOR_ITER it = &__el__

/**
 * 创建vector
 */
extern myvector_t * __vector_inter_create(HMYMEMPOOL hm, size_t size, myobj_ops * data_ops, ALG_COMPARE compare);

/**
 * 初始化数组
 */
extern int __vector_inter_init_array(myvector_t * vt, size_t size);

/**
 * 清除数组中的元素
 */
extern void __vector_inter_clear(myvector_t * vt);

/**
 * 往数组里添加元素
 */
extern myvector_element * __vector_inter_add(myvector_t * vt, const void * data, const size_t data_size);

/**
 * 往数组里添加元素,在index之后
 */
extern int __vector_inter_insert_at(myvector_t * vt, size_t index, const void * data, const size_t data_size);

/**
 * 重构数组
 */
extern int __vector_inter_resize(myvector_t * vt, size_t size);

/**
 * 创建数组元素
 */
extern myvector_element * __vector_inter_create_element(myvector_t * vt, const void * data, const size_t data_size);

/**
 * 更新某个位置的数据
 */
extern void * __vector_inter_update_index(myvector_t * vt, size_t index, const void * data, const size_t data_size);

/**
 * 移动某区间元素至另外一个数组的未尾
 * @param vt_dst:目标数组
 * @param vt_src:源数组
 * @param index_begin:要移动的源数组元素起始下标
 * @param range:要移动的元素个数
 */
extern void __vector_move_range_to_end(myvector_t * vt_dst, myvector_t * vt_src, size_t index_begin, size_t range);

/**
 * @brief 移动某区间元素至另外一个数组指定下标之前
 * @param vt_dst:目标数组
 * @param dst_index:移动至目标数组的下标之后
 * @param vt_src:源数组
 * @param index_begin:要移动的源数组元素起始下标
 * @param range:要移动的元素个数
 */
extern void __vector_move_range_to_index(myvector_t * vt_dst, size_t dst_index, myvector_t * vt_src, size_t index_begin, size_t range);

///**
// * 从vector里取出,但不释放节点的空间
// */
//extern myvector_element * __vector_inter_out(myvector_t * vt, size_t index);
//
///**
// * 添加一个element
// */
//extern myvector_element * __vector_inter_add_el(myvector_t * vt, myvector_element * el);


#endif














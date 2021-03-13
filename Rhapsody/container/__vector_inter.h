/**
 *
 * @file __vector_inter.h vector���ڲ��ӿڶ���
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
	* data��������ʵ�ʴ�С
	*/
	size_t buf_size;

	/*
	* �û�������ʹ�õĴ�С
	*/
	size_t data_size;

	/*
	* �û����ݻ���������ʼ��ַ
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
	* �Ƚϻص�����Ӧ���û�����������
	*/
	ALG_COMPARE compare;
	/* Ԥ�� */
	void * context;

	HMYMEMPOOL hm;
}myvector_t;


/**
 * ��ȡ��һ���ڵ�
 */
#define __vector_get_head(vt) (assert(vt && \
	((myvector_t *)(vt))->el_array && \
	((myvector_t *)(vt))->el_count && \
	((myvector_t *)(vt))->el_array_size), \
	((myvector_t *)(vt))->el_array[0])

/**
 * ��ȡ��һ���ڵ�
 */
#define __vector_get_head_data(vt) (__vector_get_head(vt)->data)


/**
 * ��ȡ���һ���ڵ�
 */
#define __vector_get_tail(vt) (assert(vt && \
	((myvector_t *)(vt))->el_array && \
	((myvector_t *)(vt))->el_count && \
	((myvector_t *)(vt))->el_array_size), \
	((myvector_t *)vt)->el_array[((myvector_t *)(vt))->el_count - 1])

/**
 * ��ȡ���һ���ڵ��ֵ
 */
#define __vector_get_tail_data(vt) (__vector_get_tail(vt)->data)


/**
 * ��ȡ��������������
 */
#define __vector_inter_get_iter_data(el) (assert(el), ((myvector_element *)(el))->data)

/**
 * ��ȡ��������������ĳ���
 */
#define __vector_inter_get_iter_datasize(el) (assert(el), ((myvector_element *)(el))->data_size)

/**
 * ��ȡvector��Ԫ�صĸ���
 */
#define __vector_inter_get_count(vt) (assert(vt && ((myvector_t *)(vt))->el_array), ((myvector_t *)(vt))->el_count)

/**
 * ��ȡ�������ʼ��ַ
 */
#define __vector_inter_get_array(vt) (assert(vt && ((myvector_t *)(vt))->el_array), ((myvector_t *)(vt))->el_array)

/**
 * ��ȡ������ÿ��Ԫ�صĴ�С
 */
#define __vector_inter_get_array_step_size(vt) (assert(vt), sizeof((vt)->el_array[0]))

/**
 * ��ȡָ��λ��Ԫ��
 */
#define __vector_inter_getindex(vt, index) (assert(vt && ((myvector_t *)(vt))->el_array),\
	(index >= ((myvector_t *)(vt))->el_count) ? NULL : ((myvector_t *)(vt))->el_array[index])

/**
 * ��ȡָ��λ��Ԫ��
 */
#define __vector_inter_get_index_data(vt, index) (assert(__vector_inter_get_count((vt))), (__vector_inter_getindex((vt), (index)))->data)

/**
 * ����һ���ڵ�
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
 * ����vector
 */
#define __vector_inter_destroy(vt) do{\
		if(NULL != (vt))\
			__vector_inter_free_array((vt));\
		MyMemPoolFree((vt)->hm, (vt));\
	}while(0)

/**
 * �ͷ�����
 */
#define __vector_inter_free_array(vt) do{\
	assert(vt && vt->el_array && vt->el_array_size);\
	\
	__vector_inter_clear(vt);\
	\
	MyMemPoolFree(vt->hm, vt->el_array);\
	}while(0)

/**
 * ɾ��index���Ľڵ�
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
 * ����vector
 */
extern myvector_t * __vector_inter_create(HMYMEMPOOL hm, size_t size, myobj_ops * data_ops, ALG_COMPARE compare);

/**
 * ��ʼ������
 */
extern int __vector_inter_init_array(myvector_t * vt, size_t size);

/**
 * ��������е�Ԫ��
 */
extern void __vector_inter_clear(myvector_t * vt);

/**
 * �����������Ԫ��
 */
extern myvector_element * __vector_inter_add(myvector_t * vt, const void * data, const size_t data_size);

/**
 * �����������Ԫ��,��index֮��
 */
extern int __vector_inter_insert_at(myvector_t * vt, size_t index, const void * data, const size_t data_size);

/**
 * �ع�����
 */
extern int __vector_inter_resize(myvector_t * vt, size_t size);

/**
 * ��������Ԫ��
 */
extern myvector_element * __vector_inter_create_element(myvector_t * vt, const void * data, const size_t data_size);

/**
 * ����ĳ��λ�õ�����
 */
extern void * __vector_inter_update_index(myvector_t * vt, size_t index, const void * data, const size_t data_size);

/**
 * �ƶ�ĳ����Ԫ��������һ�������δβ
 * @param vt_dst:Ŀ������
 * @param vt_src:Դ����
 * @param index_begin:Ҫ�ƶ���Դ����Ԫ����ʼ�±�
 * @param range:Ҫ�ƶ���Ԫ�ظ���
 */
extern void __vector_move_range_to_end(myvector_t * vt_dst, myvector_t * vt_src, size_t index_begin, size_t range);

/**
 * @brief �ƶ�ĳ����Ԫ��������һ������ָ���±�֮ǰ
 * @param vt_dst:Ŀ������
 * @param dst_index:�ƶ���Ŀ��������±�֮��
 * @param vt_src:Դ����
 * @param index_begin:Ҫ�ƶ���Դ����Ԫ����ʼ�±�
 * @param range:Ҫ�ƶ���Ԫ�ظ���
 */
extern void __vector_move_range_to_index(myvector_t * vt_dst, size_t dst_index, myvector_t * vt_src, size_t index_begin, size_t range);

///**
// * ��vector��ȡ��,�����ͷŽڵ�Ŀռ�
// */
//extern myvector_element * __vector_inter_out(myvector_t * vt, size_t index);
//
///**
// * ���һ��element
// */
//extern myvector_element * __vector_inter_add_el(myvector_t * vt, myvector_element * el);


#endif














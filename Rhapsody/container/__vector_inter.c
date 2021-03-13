/**
 *
 * @file __vector_inter.c vector���ڲ��ӿ�
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "__vector_inter.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>


/**
 * ��ȡ������ÿ��Ԫ�صĴ�С
 */
#define __vector_inter_add_to_end(vt, el) do{\
		(vt)->el_array[(vt)->el_count] = (el);\
		(vt)->el_count ++;\
	}while(0)

/**
 * �����������Ԫ��
 */
extern myvector_t * __vector_inter_create(HMYMEMPOOL hm, size_t size, myobj_ops * data_ops, ALG_COMPARE compare)
{
	myvector_t * vt = (myvector_t *)MyMemPoolMalloc(hm, sizeof(*vt));
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
		return vt;

	__vector_inter_destroy(vt);

	return NULL;
}

/**
 * ��ʼ������
 */
int __vector_inter_init_array(myvector_t * vt, size_t size)
{
#define DEFAULT_VECTOR_SIZE 32

	assert(vt);

	if(size > 0)
		vt->el_array_size = size;
	else
		vt->el_array_size = DEFAULT_VECTOR_SIZE;

	//����vector�Ĵ洢�ռ�
	vt->el_array = (myvector_element **)MyMemPoolMalloc(vt->hm, vt->el_array_size * sizeof(myvector_element*));
	if(NULL == vt->el_array)
		return -1;

	return 0;

#undef DEFAULT_VECTOR_SIZE
}

/**
 * ��������е�Ԫ��
 */
void __vector_inter_clear(myvector_t * vt)
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
}

/**
 * �����������Ԫ��
 */
myvector_element * __vector_inter_add(myvector_t * vt, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;

	assert(vt && vt->el_array && vt->el_array_size);

	if(vt->el_count >= vt->el_array_size)
	{
		/* �ع�vector */
		if(__vector_inter_resize(vt, (vt->el_array_size + 1) * 2))
			return NULL;
	}

	el = __vector_inter_create_element(vt, data, data_size);
	if(NULL == el)
		return NULL;

	__vector_inter_add_to_end(vt, el);

	return el;
}

/**
 * �����������Ԫ��,��index֮��
 */
int __vector_inter_insert_at(myvector_t * vt, size_t index, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;

	assert(vt && vt->el_array && vt->el_array_size);
	assert(index <= vt->el_count);

	if(vt->el_count >= vt->el_array_size)
	{
		/* �ع�vector */
		if(__vector_inter_resize(vt, (vt->el_array_size + 1) * 2))
			return -1;
	}

	el = __vector_inter_create_element(vt, data, data_size);
	if(NULL == el)
		return -1;

	if(index < vt->el_count)
		memmove((vt)->el_array + (index) + 1, (vt)->el_array + (index), ((vt)->el_count - (index))*sizeof(myvector_element*));
	else
		index = vt->el_count;

	(vt)->el_array[index] = el;

	vt->el_count ++;

	return 0;
}

/**
 * �ع�����
 */
int __vector_inter_resize(myvector_t * vt, size_t size)
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
}

/**
 * ��������Ԫ��
 */
myvector_element * __vector_inter_create_element(myvector_t * vt, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;

	assert(vt);

	if(data_size && data)
	{
		el = (myvector_element *)MyMemPoolMalloc(vt->hm, sizeof(myvector_element) + data_size + CAL_SYS_ALIGMENT(data_size));
		if(NULL == el)
			return NULL;

		el->data = (char*)el + sizeof(*el);
		el->buf_size = el->data_size = data_size;

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
		el->buf_size = el->data_size = 0;
	}

	return el;
}

/**
 * ����ĳ��λ�õ�����
 */
void * __vector_inter_update_index(myvector_t * vt, size_t index, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;

	assert(vt->el_array && vt->el_array_size);
	assert(index < vt->el_count);

	el = vt->el_array[index];

	assert(el);

	if(0 == el->data_size && 0 == data_size)
	{
		void * ret_data = el->data;
		el->data = (void *)data;
		return ret_data;
	}

	if(el->buf_size < data_size || 0 == data_size)
	{
		__vector_inter_destroy_element(vt, el);
		el = __vector_inter_create_element(vt, data, data_size);
		vt->el_array[index] = el;
	}
	else
	{
		if(vt->data_ops.destruct)
			vt->data_ops.destruct(el->data, el->data_size);

		if(vt->data_ops.copy)
			vt->data_ops.copy(el->data, el->buf_size, data, data_size);
		else
			memcpy(el->data, data, data_size);

		el->data_size = data_size;
	}

	return NULL;
}

/**
 * �ƶ�ĳ����Ԫ��������һ�������δβ
 * @param vt_dst:Ŀ������
 * @param vt_src:Դ����
 * @param index_begin:Ҫ�ƶ���Դ����Ԫ����ʼ�±�
 * @param range:Ҫ�ƶ���Ԫ�ظ���
 */
void __vector_move_range_to_end(myvector_t * vt_dst, myvector_t * vt_src, size_t index_begin, size_t range)
{
	assert(vt_dst && vt_dst->el_array && vt_dst->el_array_size);
	assert(vt_src && vt_src->el_array && vt_src->el_array_size);

	assert(vt_src->el_count > index_begin && vt_src->el_count >= (index_begin + range));

	/*
	* ȷ����ͬһ�ֶ���
	*/
	assert(vt_src->data_ops.construct == vt_dst->data_ops.construct);
	assert(vt_src->data_ops.destruct == vt_dst->data_ops.destruct);
	assert(vt_src->data_ops.copy == vt_dst->data_ops.copy);

	if(vt_dst->el_array_size < vt_dst->el_count + range)
	{
		if(__vector_inter_resize(vt_dst, (vt_dst->el_count + range) * 2))
			return;
	}

	if(vt_src->hm != vt_dst->hm)
	{
		/*
		* ���ʹ�õ��ǲ�ͬ���ڴ��
		*/
		size_t i;
		for(i = index_begin; i < index_begin + range; i ++)
		{
			myvector_element * node = __vector_inter_getindex(vt_src, i);

			__vector_inter_add(vt_dst, node->data, node->data_size);

			/*
			* MyMemPoolFree(vt_src->hm, node);
			*/
		}

		for(i = index_begin; i < index_begin + range; i ++)
			MyMemPoolFree(vt_src->hm, __vector_inter_getindex(vt_src, i));
	}
	else
	{
		/*
		* ���ʹ�õ���ͬһ���ڴ��
		*/
		memcpy(vt_dst->el_array + vt_dst->el_count, 
			vt_src->el_array + index_begin, 
			range * sizeof(myvector_element*));
	}

	if(vt_src->el_count - index_begin - range)
		memcpy(vt_src->el_array + index_begin,
			vt_src->el_array + index_begin + range,
			(vt_src->el_count - index_begin - range)*sizeof(myvector_element*));

	vt_dst->el_count += range;
	vt_src->el_count -= range;
}

/**
 * @brief �ƶ�ĳ����Ԫ��������һ������ָ���±�֮ǰ
 * @param vt_dst:Ŀ������
 * @param dst_index:�ƶ���Ŀ��������±�֮��
 * @param vt_src:Դ����
 * @param index_begin:Ҫ�ƶ���Դ����Ԫ����ʼ�±�
 * @param range:Ҫ�ƶ���Ԫ�ظ���
 */
void __vector_move_range_to_index(myvector_t * vt_dst, size_t dst_index, myvector_t * vt_src, size_t index_begin, size_t range)
{
	assert(vt_dst && vt_dst->el_array && vt_dst->el_array_size);
	assert(vt_src && vt_src->el_array && vt_src->el_array_size);

	assert(vt_src->el_count > index_begin && vt_src->el_count >= (index_begin + range));
	assert(vt_dst->el_count >= dst_index);

	/*
	* ȷ����ͬһ�ֶ���
	*/
	assert(vt_src->data_ops.construct == vt_dst->data_ops.construct);
	assert(vt_src->data_ops.destruct == vt_dst->data_ops.destruct);
	assert(vt_src->data_ops.copy == vt_dst->data_ops.copy);

	/*
	* ���Ŀ�����鲻����,�ع�Ŀ������
	*/
	if(vt_dst->el_array_size < vt_dst->el_count + range)
	{
		if(__vector_inter_resize(vt_dst, (vt_dst->el_count + range) * 2))
			return;
	}

	if(vt_src->hm != vt_dst->hm)
	{
		/*
		* ���ʹ�õ��ǲ�ͬ���ڴ��
		*/
		size_t i;
		for(i = index_begin; i < index_begin + range; i ++)
		{
			myvector_element * node = __vector_inter_getindex(vt_src, i);

			__vector_inter_insert_at(vt_dst, dst_index + i, node->data, node->data_size);

			/*
			* MyMemPoolFree(vt_src->hm, node);
			*/
		}

		for(i = index_begin; i < index_begin + range; i ++)
			MyMemPoolFree(vt_src->hm, __vector_inter_getindex(vt_src, i));
	}
	else
	{
		/*
		* ���ʹ�õ���ͬһ���ڴ��
		* Ŀ���������Ԫ������Ų,Ȼ�󿽱�����
		*/
		memmove(vt_dst->el_array + dst_index + range, vt_dst->el_array + dst_index, 
			(vt_dst->el_count - dst_index) * sizeof(myvector_element *));

		memcpy(vt_dst->el_array + dst_index, 
			vt_src->el_array + index_begin, 
			range * sizeof(myvector_element*));
	}

	if(vt_src->el_count - index_begin - range)
		memcpy(vt_src->el_array + index_begin,
			vt_src->el_array + index_begin + range,
			(vt_src->el_count - index_begin - range)*sizeof(myvector_element*));

	vt_dst->el_count += range;
	vt_src->el_count -= range;
}

///**
// * ��vector��ȡ��,�����ͷŽڵ�Ŀռ�
// */
//myvector_element * __vector_inter_out(myvector_t * vt, size_t index)
//{
//	myvector_element * el = NULL;
//
//	assert(vt && index < (vt)->el_count && (vt)->el_array && (vt)->el_array_size);
//
//	el = (vt)->el_array[index];
//
//	if(index < (vt)->el_count - 1)
//		memcpy((vt)->el_array + index, (vt)->el_array + index + 1, ((vt)->el_count - index - 1)*sizeof(myvector_element*));
//
//	(vt)->el_count --;
//
//	return el;
//}
//
///**
// * ���һ��element
// */
//void __vector_inter_add_el(myvector_t * vt, const myvector_element * el)
//{
//	assert(vt && vt->el_array && vt->el_array_size);
//
//	/*
//	* ȷ����ͬһ�ֶ���
//	*/
//	assert(vt->data_ops.construct == el->vt->data_ops.construct);
//	assert(vt->data_ops.destruct == el->vt->data_ops.destruct);
//	assert(vt->data_ops.copy == el->vt->data_ops.copy);
//
//	if(vt->hm == el->vt->hm)
//	{
//		/*
//		* ���ʹ�õ���ͬһ���ڴ��
//		*/
//		if(vt->el_count >= vt->el_array_size)
//		{
//			/* �ع�vector */
//			if(__vector_inter_resize(vt, (vt->el_array_size + 1) * 2))
//				return;
//		}
//
//		__vector_inter_add_to_end(vt, el);
//	}
//	else
//	{
//		/*
//		* ���ʹ�õ��ǲ�ͬ���ڴ��
//		*/
//		__vector_inter_add(vt, el->data, el->data_size);
//	}
//}



















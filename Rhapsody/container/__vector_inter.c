/**
 *
 * @file __vector_inter.c vector的内部接口
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "__vector_inter.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>


/**
 * 获取数组中每个元素的大小
 */
#define __vector_inter_add_to_end(vt, el) do{\
		(vt)->el_array[(vt)->el_count] = (el);\
		(vt)->el_count ++;\
	}while(0)

/**
 * 往数组里添加元素
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
 * 初始化数组
 */
int __vector_inter_init_array(myvector_t * vt, size_t size)
{
#define DEFAULT_VECTOR_SIZE 32

	assert(vt);

	if(size > 0)
		vt->el_array_size = size;
	else
		vt->el_array_size = DEFAULT_VECTOR_SIZE;

	//分配vector的存储空间
	vt->el_array = (myvector_element **)MyMemPoolMalloc(vt->hm, vt->el_array_size * sizeof(myvector_element*));
	if(NULL == vt->el_array)
		return -1;

	return 0;

#undef DEFAULT_VECTOR_SIZE
}

/**
 * 清除数组中的元素
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
 * 往数组里添加元素
 */
myvector_element * __vector_inter_add(myvector_t * vt, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;

	assert(vt && vt->el_array && vt->el_array_size);

	if(vt->el_count >= vt->el_array_size)
	{
		/* 重构vector */
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
 * 往数组里添加元素,在index之后
 */
int __vector_inter_insert_at(myvector_t * vt, size_t index, const void * data, const size_t data_size)
{
	myvector_element * el = NULL;

	assert(vt && vt->el_array && vt->el_array_size);
	assert(index <= vt->el_count);

	if(vt->el_count >= vt->el_array_size)
	{
		/* 重构vector */
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
 * 重构数组
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
 * 创建数组元素
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
 * 更新某个位置的数据
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
 * 移动某区间元素至另外一个数组的未尾
 * @param vt_dst:目标数组
 * @param vt_src:源数组
 * @param index_begin:要移动的源数组元素起始下标
 * @param range:要移动的元素个数
 */
void __vector_move_range_to_end(myvector_t * vt_dst, myvector_t * vt_src, size_t index_begin, size_t range)
{
	assert(vt_dst && vt_dst->el_array && vt_dst->el_array_size);
	assert(vt_src && vt_src->el_array && vt_src->el_array_size);

	assert(vt_src->el_count > index_begin && vt_src->el_count >= (index_begin + range));

	/*
	* 确保是同一种对象
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
		* 如果使用的是不同的内存池
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
		* 如果使用的是同一个内存池
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
 * @brief 移动某区间元素至另外一个数组指定下标之前
 * @param vt_dst:目标数组
 * @param dst_index:移动至目标数组的下标之后
 * @param vt_src:源数组
 * @param index_begin:要移动的源数组元素起始下标
 * @param range:要移动的元素个数
 */
void __vector_move_range_to_index(myvector_t * vt_dst, size_t dst_index, myvector_t * vt_src, size_t index_begin, size_t range)
{
	assert(vt_dst && vt_dst->el_array && vt_dst->el_array_size);
	assert(vt_src && vt_src->el_array && vt_src->el_array_size);

	assert(vt_src->el_count > index_begin && vt_src->el_count >= (index_begin + range));
	assert(vt_dst->el_count >= dst_index);

	/*
	* 确保是同一种对象
	*/
	assert(vt_src->data_ops.construct == vt_dst->data_ops.construct);
	assert(vt_src->data_ops.destruct == vt_dst->data_ops.destruct);
	assert(vt_src->data_ops.copy == vt_dst->data_ops.copy);

	/*
	* 如果目标数组不够大,重构目标数组
	*/
	if(vt_dst->el_array_size < vt_dst->el_count + range)
	{
		if(__vector_inter_resize(vt_dst, (vt_dst->el_count + range) * 2))
			return;
	}

	if(vt_src->hm != vt_dst->hm)
	{
		/*
		* 如果使用的是不同的内存池
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
		* 如果使用的是同一个内存池
		* 目标数组里的元素往后挪,然后拷贝即可
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
// * 从vector里取出,但不释放节点的空间
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
// * 添加一个element
// */
//void __vector_inter_add_el(myvector_t * vt, const myvector_element * el)
//{
//	assert(vt && vt->el_array && vt->el_array_size);
//
//	/*
//	* 确保是同一种对象
//	*/
//	assert(vt->data_ops.construct == el->vt->data_ops.construct);
//	assert(vt->data_ops.destruct == el->vt->data_ops.destruct);
//	assert(vt->data_ops.copy == el->vt->data_ops.copy);
//
//	if(vt->hm == el->vt->hm)
//	{
//		/*
//		* 如果使用的是同一个内存池
//		*/
//		if(vt->el_count >= vt->el_array_size)
//		{
//			/* 重构vector */
//			if(__vector_inter_resize(vt, (vt->el_array_size + 1) * 2))
//				return;
//		}
//
//		__vector_inter_add_to_end(vt, el);
//	}
//	else
//	{
//		/*
//		* 如果使用的是不同的内存池
//		*/
//		__vector_inter_add(vt, el->data, el->data_size);
//	}
//}



















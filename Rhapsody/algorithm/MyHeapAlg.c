/**
 *
 * @file MyHeapAlg.c ����ص��㷨
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "MyAlgorithm.h"

#include <assert.h>
#include <stdlib.h>

#include "__def_fun_def.h"
#include "mymempool.h"
#include "myutility.h"


/**
 * @brief ����ڶ����ӽڵ������
 *
 * 0 - 1 2  2(i + 1) - 1 2(i + 1)
 * 1 - 3 4
 * 2 - 5 6
 */
#define HEAP_SECOND_CHILD_INDEX(x) (2 * ((x) + 1))

/**
 * @brief ���㸸�ڵ������
 */
#define HEAP_PARENT_INDEX(x) (assert(x), ((x) - 1)/2)


static __INLINE__ int heap_push(void * buf, size_t step_size,
					 size_t hole_index, size_t top_index, const void * value, size_t value_size,
					 ALG_COMPARE compare, ALG_COPY copier,
					 const void * context)
{
	size_t parent = 0;

	assert(buf && value && compare && copier);
	assert(value_size <= step_size);

	/*
	* ����(hole_index)����(top_index)Ѱ��value���ʵķ���λ��
	*/
	while(hole_index > top_index)
	{
		parent = HEAP_PARENT_INDEX(hole_index);

		/*
		* value����parent,���parent����hole_index��λ��
		*/
		if(compare(value, GET_INDEX_PTR(buf, parent, step_size), context) > 0)
		{
			copier(GET_INDEX_PTR(buf, hole_index, step_size), GET_INDEX_PTR(buf, parent, step_size), step_size, context);

			hole_index = parent;
		}
		else /* �����˳�ѭ�� */
			break;
	}

	copier(GET_INDEX_PTR(buf, hole_index, step_size), value, (step_size < value_size) ? step_size : value_size, context);

	return 0;
}

static __INLINE__ int adjust_heap(void * buf, size_t len, size_t step_size, 
					   size_t hole_index, const void * value, size_t value_size,
					   ALG_COMPARE compare, ALG_COPY copier,
					   const void * context)
{
	size_t top_index = hole_index;
	size_t second_index = HEAP_SECOND_CHILD_INDEX(hole_index);

	assert(buf && value && compare && copier);

	/*
	* ����(hole_index)���µص�����
	*/
	while(second_index < len)
	{
		/*
		* ȡ��С��һ���ڵ�������
		*/
		if(compare(GET_INDEX_PTR(buf, second_index - 1, step_size), GET_INDEX_PTR(buf, second_index, step_size), context) > 0)
			second_index --;

		/*
		* ��second_index������ݿ���hole_index��
		*/
		copier(GET_INDEX_PTR(buf, hole_index, step_size), GET_INDEX_PTR(buf, second_index, step_size), step_size, context);

		hole_index = second_index;
		second_index = HEAP_SECOND_CHILD_INDEX(hole_index);
	}

	/*
	* ����ֻ������,û���Һ��ӵı߽����
	*/
	if(second_index == len)
	{
		/*
		* ֱ�ӽ�����������
		*/
		copier(GET_INDEX_PTR(buf, hole_index, step_size), GET_INDEX_PTR(buf, second_index - 1, step_size), step_size, context);

		hole_index = second_index - 1;
	}

	heap_push(buf, step_size, hole_index, top_index, value, value_size, compare, copier, context);

	return 0;
}

static __INLINE__ int heap_pop(void * buf, size_t len, size_t step_size,					
					ALG_COMPARE compare, ALG_COPY copier,
					const void * context, void * swap_buf, const size_t swap_buf_size)
{
	assert(buf && compare && copier && swap_buf && swap_buf_size >= step_size);

	copier(swap_buf, GET_INDEX_PTR(buf, len - 1, step_size), step_size, context);
	copier(GET_INDEX_PTR(buf, len - 1, step_size), GET_INDEX_PTR(buf, 0, step_size), step_size, context);

	adjust_heap(buf, len - 1, step_size, 0, swap_buf, swap_buf_size, compare, copier, context);

	return 0;
}

static __INLINE__ int __make_heap__(void * buf, size_t len, size_t step_size,
					  ALG_COMPARE compare, ALG_COPY copier,
					  const void * context, void * swap_buf, const size_t swap_buf_size)
{
	size_t parent = HEAP_PARENT_INDEX((len - 1));

	assert(buf && compare && copier);
	assert(swap_buf && swap_buf_size >= step_size && len >= 2);

	while(1)
	{
		copier(swap_buf, GET_INDEX_PTR(buf, parent, step_size), step_size, context);
		adjust_heap(buf, len, step_size, parent, swap_buf, swap_buf_size, compare, copier, context);
		if(0 == parent)
			return 0;
		parent --;
	}
}


/**
 *
 * @brief ��һ��������
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����������Ԫ�صĸ���
 * @param step_size:ָ�����ߵĿ��(1��ʾһ�ֽ�)
 * @param compare:�Ƚϻص�����(����Ϊ��)
 * @param copier:�����ص�����(��Ϊ��)
 * @param context:�û��Զ��������������(��Ϊ��)
 * @param swap_buf:���ɶѹ������õ�����ʱ�ռ�(��Ϊnull)
 * @param swap_buf_size:swap_buf�Ĵ�С
 *
 */
int MyAlgHeapSort(void * buf, size_t len, size_t step_size,
				  ALG_COMPARE compare, ALG_COPY copier,
				  const void * context, void * swap_buf, size_t swap_buf_size)
{
	int need_free_swap = 0;
	if(NULL == buf || NULL == compare)
		return -1;

	if(NULL == copier)
		copier = __def_alg_copy_;

	if(NULL == swap_buf || swap_buf_size < step_size)
	{
		swap_buf = MyMemPoolMalloc(NULL, step_size);
		swap_buf_size = step_size;

		need_free_swap = 1;
	}

	/*
	* ����pop�����������
	*/
	while(len >= 2)
	{
		heap_pop(buf, len --, step_size, compare, copier, context, swap_buf, swap_buf_size);
	}

	if(need_free_swap)
		MyMemPoolFree(NULL, swap_buf);

	return 0;
}

/**
 *
 * @brief ���ɶ�
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����������Ԫ�صĸ���
 * @param step_size:ָ�����ߵĿ��(1��ʾһ�ֽ�)
 * @param compare:�Ƚϻص�����
 * @param copier:�����ص�����
 * @param context:�û��Զ��������������
 * @param swap_buf:���ɶѹ������õ�����ʱ�ռ�(��Ϊnull)
 * @param swap_buf_size:swap_buf�Ĵ�С
 *
 */
int MyAlgMakeHeap(void * buf, size_t len, size_t step_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context, void * swap_buf, size_t swap_buf_size)
{
	if(NULL == buf || NULL == compare)
		return -1;

	if(len <= 1)
		return 0;

	if(NULL == copier)
		copier = __def_alg_copy_;

	if(swap_buf && swap_buf_size >= step_size)
	{
		__make_heap__(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);

		return 0;
	}

	swap_buf = MyMemPoolMalloc(NULL, step_size);
	swap_buf_size = step_size;

	__make_heap__(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);

	MyMemPoolFree(NULL, swap_buf);

	return 0;
}

/**
 *
 * @brief ������
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����������Ԫ�صĸ���
 * @param step_size:ָ�����ߵĿ��(1��ʾһ�ֽ�)
 * @param compare:�Ƚϻص�����
 * @param copier:�����ص�����
 * @param context:�û��Զ��������������
 * @param swap_buf:���ɶѹ������õ�����ʱ�ռ�(��Ϊnull)
 * @param swap_buf_size:swap_buf�Ĵ�С
 *
 */
int MyAlgHeapMakeAndSort(void * buf, size_t len, size_t step_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context, void * swap_buf, size_t swap_buf_size)
{
	MyAlgMakeHeap(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);
	MyAlgHeapSort(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);

	return 0;
}

/**
 *
 * @brief �Ӷ��е���һ��Ԫ��
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:������Ԫ�صĸ���
 * @param step_size:ָ�����ߵĿ��(1��ʾһ�ֽ�)
 * @param compare:�Ƚϻص�����
 * @param copier:�����ص�����
 * @param context:�û��Զ��������������
 * @param swap_buf:���ɶѹ������õ�����ʱ�ռ�(��Ϊnull)
 * @param swap_buf_size:swap_buf�Ĵ�С
 *
 */
int MyAlgHeapPop(void * buf, size_t len, size_t step_size,
						ALG_COMPARE compare, ALG_COPY copier,
						const void * context, void * swap_buf, size_t swap_buf_size)
{
	if(NULL == buf || NULL == compare)
		return -1;

	if(NULL == copier)
		copier = __def_alg_copy_;

	if(swap_buf && swap_buf_size < step_size)
	{
		heap_pop(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);

		return 0;
	}

	swap_buf = MyMemPoolMalloc(NULL, step_size);
	swap_buf_size = step_size;

	heap_pop(buf, len, step_size, compare, copier, context, swap_buf, swap_buf_size);

	MyMemPoolFree(NULL, swap_buf);

	return 0;
}

/**
 *
 * @brief ��һ��Ԫ��������
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:�����е�Ԫ�ظ���
 * @param step_size:ÿ��Ԫ�صĴ�С
 * @param value:Ҫ�����ֵ
 * @param value_size:value�Ĵ�С
 * @param context:�û��Զ��������������
 *
 */
int MyAlgHeapPush(void * buf, size_t len, size_t step_size,
						 const void * value, size_t value_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context)
{
	if(NULL == buf || NULL == value || NULL == compare || value_size > step_size || 0 == len)
		return -1;

	if(NULL == copier)
		copier = __def_alg_copy_;

	heap_push(buf, step_size, len - 1, 0, value, value_size, compare, copier, context);

	return 0;
}

/**
 *
 * @brief ����һ����
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����Ԫ�ظ���
 * @param step_size:ÿ��Ԫ�صĴ�С
 * @param hole_index:�ն��ڵ������(0��ʾ��һ���ڵ�)
 * @param value:ֵ
 * @param value_size:ֵ�Ĵ�С
 * @param compare:�Ƚϻص�(����Ϊ��)
 * @param copier:��ο���(��Ϊ��)
 * @param context:�û��Զ��������������
 *
 */
/*int MyAlgAdjustHeap(void * buf, size_t len, size_t step_size, 
						   size_t hole_index, const void * value, size_t value_size,
						   ALG_COMPARE compare, ALG_COPY copier,
						   const void * context)
{
	if(NULL == buf || NULL == value || NULL == compare)
		return -1;

	if(NULL == copier)
		copier = __def_alg_copy_;

	adjust_heap(buf, len, step_size, hole_index, value, value_size, compare, copier, context);

	return 0;
}*/

/**
 *
 * @brief ���һ�������Ƿ�Ϊһ����
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����Ԫ�ظ���
 * @param step_size:ÿ��Ԫ�صĴ�С
 * @param compare:�Ƚϻص�(����Ϊ��)
 * @param context:�û��Զ��������������
 *
 */
int MyAlgExaminHeap(void * buf, size_t len, size_t step_size,
					ALG_COMPARE compare,
					const void * context)
{
	size_t parent_index = 0;
	size_t second_child = 0;

	if(NULL == buf || NULL == compare)
		return -1;

	if(len <= 1)
		return 0;

	second_child = HEAP_SECOND_CHILD_INDEX(parent_index);
	while(second_child < len)
	{
		/*
		* ֻҪ�ӽڵ㲻���ڸ��ڵ�,���ǺϷ���
		*/
		assert(compare(GET_INDEX_PTR(buf, second_child, step_size), GET_INDEX_PTR(buf, parent_index, step_size), context) <= 0);
		assert(compare(GET_INDEX_PTR(buf, second_child - 1, step_size), GET_INDEX_PTR(buf, parent_index, step_size), context) <= 0);

		parent_index += 1;
		second_child = HEAP_SECOND_CHILD_INDEX(parent_index);
	}

	if(second_child == len)
	{
		assert(compare(GET_INDEX_PTR(buf, second_child - 1, step_size), GET_INDEX_PTR(buf, parent_index, step_size), context) <= 0);
	}

	return 0;
}

/**
 *
 * @brief ���һ�������Ƿ��Ѿ���ȷ�ر�����
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����Ԫ�ظ���
 * @param step_size:ÿ��Ԫ�صĴ�С
 * @param compare:�Ƚϻص�(����Ϊ��)
 * @param context:�û��Զ��������������
 *
 */
int MyAlgSortOK(void * buf, size_t len, size_t step_size,
				ALG_COMPARE compare,
				const void * context)
{
	size_t i = 0;

	if(NULL == buf || NULL == compare)
		return -1;

	if(len <= 1)
		return 0;

	for(i = 1; i < len; i ++)
	{
		/*
		* ֻҪǰһ�������ں�һ��,��˵������ȷ������
		*/
		if(compare(GET_INDEX_PTR(buf, i - 1, step_size), GET_INDEX_PTR(buf, i, step_size), context) > 0)
		{
			int a = 0;
			int b = a;
		}
		assert(compare(GET_INDEX_PTR(buf, i - 1, step_size), GET_INDEX_PTR(buf, i, step_size), context) <= 0);
	}

	return 0;
}

















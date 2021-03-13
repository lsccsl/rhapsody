/**
 *
 * @file MyQuickSort.c ��������
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "MyAlgorithm.h"
#include <assert.h>
#include "mymempool.h"
#include "myutility.h"
#include "__def_fun_def.h"


#define THRESHOLD 16


typedef struct __quick_sort_cal_session_
{
	/*
	* ��¼��������
	*/
	size_t start;
	size_t end;

	/*
	* ��¼��ǰʣ��ĵݹ����
	*/
	size_t depth_limit;
}quick_sort_cal_session;

typedef struct __quick_sort_stack_t_
{
	/*
	* ջ����ʼ��ַ���С
	*/
	quick_sort_cal_session * s;
	size_t s_size;

	/*
	* ջ��ָ��
	*/
	size_t top;
}quick_sort_stack_t;


/**
 * @brief ����ջ
 */
static __INLINE__ quick_sort_stack_t * quick_sort_stack_create(size_t dep_limit)
{
	quick_sort_stack_t * s = (quick_sort_stack_t *)MyMemPoolMalloc(NULL, sizeof(*s));

	s->s_size = (dep_limit + 1) * 2;
	s->s = (quick_sort_cal_session *)MyMemPoolMalloc(NULL, s->s_size * sizeof(quick_sort_cal_session));
	s->top = 0;

	return s;
}

/**
 * @brief ����ջ
 */
static __INLINE__ void quick_sort_stack_destroy(quick_sort_stack_t * s)
{
	assert(s);

	MyMemPoolFree(NULL, s->s);

	MyMemPoolFree(NULL, s);
}

/**
 * @brief ��ջ
 */
static __INLINE__ void quick_sort_stack_push(quick_sort_stack_t * s, quick_sort_cal_session * cs)
{
	assert(s && cs && s->top < s->s_size);

	s->s[s->top] = *cs;
	s->top ++;
}

/**
 * @brief ��ջ
 */
static __INLINE__ void quick_sort_stack_pop(quick_sort_stack_t * s, quick_sort_cal_session * cs)
{
	assert(s && cs);

	*cs = s->s[s->top - 1];

	s->top --;
}

/**
 * @brief �ж�ջ�Ƿ�Ϊ��
 */
static __INLINE__ int quick_sort_stack_empty(quick_sort_stack_t * s)
{
	assert(s);

	return (s->top == 0);
}


/**
 * @brief ����2�Ķ��� ���ڼ����������ʱ�����ݹ����,��ֹ���ֶ񻯵�����
 */
static __INLINE__ size_t __lg__(size_t v)
{
	size_t ret = 0;
	for(; v > 1; v >>= 1)
		ret ++;

	return ret;
}

/**
 * @brief ������ֵ
 */
static __INLINE__ void * __median__(void * a, void * b, void * c,
									ALG_COMPARE compare, const void * context)
{
	assert(a && b && c && compare);

	if(compare(a ,b, context) > 0)
	{
		if(compare(a, c, context) > 0)
			return a;
		else
			return c;
	}
	else
	{
		if(compare(b, c, context) > 0)
			return b;
		else
			return c;
	}
}

/**
 * @brief ����value_patition��ֵ�ĳ��ָ��
 */
static __INLINE__ size_t _partition_(void * buf, size_t start, size_t end, size_t step_size,
								   ALG_COMPARE compare, ALG_SWAP swaper,
								   const void * value_partition,
								   const void * context)
{
	assert(buf && (start < end) && compare && swaper && value_partition);

	while(1)
	{
		while(compare(value_partition, GET_INDEX_PTR(buf, start, step_size), context) > 0)
			start ++;

		while(compare(GET_INDEX_PTR(buf, end, step_size), value_partition, context) > 0)
			end --;

		if(start >= end)
			return start;

		swaper(GET_INDEX_PTR(buf, start, step_size), GET_INDEX_PTR(buf, end, step_size), step_size, context);

		start ++;
	}
}

/**
 * @brief ����len��С����ݹ����,��ֹ���ַ����񻯵�����ݹ���ÿ�������
 *
 * @todo ���lenС��һ���ٽ�ֵ(����Ϊ16)���ò���ʽ����
 */
static __INLINE__ int quick_sort(void * buf, size_t len, size_t step_size,
					  ALG_COMPARE compare, ALG_SWAP swaper, ALG_MOVE move, ALG_COPY copier,
					  const void * context, void * swap_buf, size_t swap_buf_size)
{
	quick_sort_cal_session cs = {0};
	quick_sort_stack_t * _stack = NULL;

	/*
	* ����ݹ����
	*/
	size_t dep_limit = __lg__(len);

	assert(buf && compare && swap_buf && swap_buf_size >= step_size);
	assert(swaper && move && copier);

	/*
	* ����ջ
	*/
	_stack = quick_sort_stack_create(dep_limit);

	/*
	* ��ʼ��Ϣ��ջ
	*/
	cs.depth_limit = dep_limit;
	cs.start = 0;
	cs.end = len - 1;
	quick_sort_stack_push(_stack, &cs);

	while(!quick_sort_stack_empty(_stack))
	{
		void * middle = NULL;
		size_t cut = 0;
		size_t start = 0;
		size_t end = 0;

		/*��ջ*/
		quick_sort_stack_pop(_stack, &cs);

		start = cs.start;
		end = cs.end;
		assert(start < len && end < len && end >= start);

		/**
		 * �����ǰ����Ĵ�СС��һ��ֵ(��16) ֱ���ò���ʽ���������һ�����������
		 */
		if(end - start <= THRESHOLD)
		{
			MyAlgInsertSort(GET_INDEX_PTR(buf, start, step_size), end - start + 1, step_size,
				compare, swaper, move, copier, 
				context, swap_buf, swap_buf_size);
			continue;
		}

		/*
		* ����ݹ����Ϊ��,�Դ�������ж�����,��ֹ���ַ�����
		*/
		if(0 == cs.depth_limit)
		{
			MyAlgHeapMakeAndSort(GET_INDEX_PTR(buf, start, step_size), end - start + 1, step_size,
				compare, copier, context,
				swap_buf, swap_buf_size);
			continue;
		}

		/*
		* �ҳ���ֵ
		*/
		middle = __median__(GET_INDEX_PTR(buf, start, step_size),
							GET_INDEX_PTR(buf, end, step_size),
							GET_INDEX_PTR(buf, (start + end + 1) / 2, step_size), compare, context);
		copier(swap_buf, middle, step_size, context);

		cut = _partition_(buf, start, end, step_size, compare, swaper, swap_buf, context);

		cs.depth_limit --;
		cs.end = cut - 1;
		if(cs.end > cs.start)
			quick_sort_stack_push(_stack, &cs);

		cs.start = cut;
		cs.end = end;
		if(cs.end > cs.start)
			quick_sort_stack_push(_stack, &cs);
	}
	
	quick_sort_stack_destroy(_stack);

	return 0;
}


/**
 *
 * @brief ��������
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param buf_size:Ҫ�ŵ����е�Ԫ�ظ���
 * @param step_size:ָ�����ߵĿ��(1��ʾһ�ֽ�)
 * @param context:�û��Զ��������������
 *
 */
int MyAlgQuickSort(void * buf, size_t buf_size, size_t step_size,
				   ALG_COMPARE compare, ALG_SWAP swaper, ALG_MOVE move, ALG_COPY copier,
				   const void * context, void * swap_buf, size_t swap_buf_size)
{
	if(NULL == buf || NULL == compare)
		return -1;

	if(NULL == move)
		move = __def_alg_move_;

	if(NULL == swaper)
		swaper = __def_alg_swap_;

	if(NULL == copier)
		copier = __def_alg_copy_;

	if(swap_buf && swap_buf_size >= step_size)
	{
		quick_sort(buf, buf_size, step_size, compare, swaper, move, copier, context, swap_buf, swap_buf_size);
		return 0;
	}

	swap_buf = MyMemPoolMalloc(NULL, step_size);
	swap_buf_size = step_size;

	quick_sort(buf, buf_size, step_size, compare, swaper, move, copier, context, swap_buf, swap_buf_size);
	
	MyMemPoolFree(NULL, swap_buf);

	return 0;
}


















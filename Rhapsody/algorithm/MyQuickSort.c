/**
 *
 * @file MyQuickSort.c 快速排序
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
	* 记录处理区间
	*/
	size_t start;
	size_t end;

	/*
	* 记录当前剩余的递归次数
	*/
	size_t depth_limit;
}quick_sort_cal_session;

typedef struct __quick_sort_stack_t_
{
	/*
	* 栈的起始地址与大小
	*/
	quick_sort_cal_session * s;
	size_t s_size;

	/*
	* 栈顶指针
	*/
	size_t top;
}quick_sort_stack_t;


/**
 * @brief 创建栈
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
 * @brief 销毁栈
 */
static __INLINE__ void quick_sort_stack_destroy(quick_sort_stack_t * s)
{
	assert(s);

	MyMemPoolFree(NULL, s->s);

	MyMemPoolFree(NULL, s);
}

/**
 * @brief 入栈
 */
static __INLINE__ void quick_sort_stack_push(quick_sort_stack_t * s, quick_sort_cal_session * cs)
{
	assert(s && cs && s->top < s->s_size);

	s->s[s->top] = *cs;
	s->top ++;
}

/**
 * @brief 出栈
 */
static __INLINE__ void quick_sort_stack_pop(quick_sort_stack_t * s, quick_sort_cal_session * cs)
{
	assert(s && cs);

	*cs = s->s[s->top - 1];

	s->top --;
}

/**
 * @brief 判断栈是否为空
 */
static __INLINE__ int quick_sort_stack_empty(quick_sort_stack_t * s)
{
	assert(s);

	return (s->top == 0);
}


/**
 * @brief 计算2的对数 用于计算快速排序时的最大递归次数,防止出现恶化的现象
 */
static __INLINE__ size_t __lg__(size_t v)
{
	size_t ret = 0;
	for(; v > 1; v >>= 1)
		ret ++;

	return ret;
}

/**
 * @brief 三点中值
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
 * @brief 根据value_patition的值的出分割点
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
 * @brief 根据len大小计算递归次数,防止出现分区恶化的情况递归调用快速排序
 *
 * @todo 如果len小于一个临界值(假设为16)采用插入式排序
 */
static __INLINE__ int quick_sort(void * buf, size_t len, size_t step_size,
					  ALG_COMPARE compare, ALG_SWAP swaper, ALG_MOVE move, ALG_COPY copier,
					  const void * context, void * swap_buf, size_t swap_buf_size)
{
	quick_sort_cal_session cs = {0};
	quick_sort_stack_t * _stack = NULL;

	/*
	* 计算递归次数
	*/
	size_t dep_limit = __lg__(len);

	assert(buf && compare && swap_buf && swap_buf_size >= step_size);
	assert(swaper && move && copier);

	/*
	* 创建栈
	*/
	_stack = quick_sort_stack_create(dep_limit);

	/*
	* 初始信息入栈
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

		/*出栈*/
		quick_sort_stack_pop(_stack, &cs);

		start = cs.start;
		end = cs.end;
		assert(start < len && end < len && end >= start);

		/**
		 * 如果当前区间的大小小于一个值(如16) 直接用插入式排序完成这一子区间的排序
		 */
		if(end - start <= THRESHOLD)
		{
			MyAlgInsertSort(GET_INDEX_PTR(buf, start, step_size), end - start + 1, step_size,
				compare, swaper, move, copier, 
				context, swap_buf, swap_buf_size);
			continue;
		}

		/*
		* 如果递归次数为零,对此区间进行堆排序,防止出现分区恶化
		*/
		if(0 == cs.depth_limit)
		{
			MyAlgHeapMakeAndSort(GET_INDEX_PTR(buf, start, step_size), end - start + 1, step_size,
				compare, copier, context,
				swap_buf, swap_buf_size);
			continue;
		}

		/*
		* 找出中值
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
 * @brief 快速排序
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param buf_size:要排的序列的元素个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param context:用户自定义的上下文数据
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


















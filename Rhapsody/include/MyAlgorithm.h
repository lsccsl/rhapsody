/**
 *
 * @file MyAlgorithm.h 各种算法函数声明
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __MYALGORITHM_H__
#define __MYALGORITHM_H__


#include <stdlib.h>
#include "MyfunctionDef.h"


/**
 * @brief:插入式排序
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:缓冲区的元素的个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param compare:比较回调(不可为空)
 * @param swaper:如何交换两个元素的数据(可为空)
 * @param move:如何移动一数据(类似于memmove动作,可为空)
 * @param copier:如何拷贝数据(可为空)
 * @param context:用户自定义的上下文数据(可为空)
 * @param swap_buf:在排序过程中用于保存临时数据,大小为一个元素的大小即可
 * @param swap_buf_size:swap_buf的实际大小
 */
extern int MyAlgInsertSort(void * buf, size_t len, size_t step_size,
						ALG_COMPARE compare, ALG_SWAP swaper, ALG_MOVE move, ALG_COPY copier,
						const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief 快速排序
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排的序列的元素个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param context:用户自定义的上下文数据
 */
extern int MyAlgQuickSort(void * buf, size_t len, size_t step_size,
					   ALG_COMPARE compare, ALG_SWAP swaper, ALG_MOVE move, ALG_COPY copier,
					   const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief 将一个堆排序
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的数组中元素的个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param compare:比较回调函数(不可为空)
 * @param copier:拷贝回调函数(可为空)
 * @param context:用户自定义的上下文数据(可为空)
 * @param swap_buf:生成堆过程中用到的临时空间(可为null)
 * @param swap_buf_size:swap_buf的大小
 */
extern int MyAlgHeapSort(void * buf, size_t len, size_t step_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief 生成堆
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的数组中元素的个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param compare:比较回调函数
 * @param copier:拷贝回调函数
 * @param context:用户自定义的上下文数据
 * @param swap_buf:生成堆过程中用到的临时空间(可为null)
 * @param swap_buf_size:swap_buf的大小
 */
extern int MyAlgMakeHeap(void * buf, size_t len, size_t step_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief 堆排序
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的数组中元素的个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param compare:比较回调函数
 * @param copier:拷贝回调函数
 * @param context:用户自定义的上下文数据
 * @param swap_buf:生成堆过程中用到的临时空间(可为null)
 * @param swap_buf_size:swap_buf的大小
 */
extern int MyAlgHeapMakeAndSort(void * buf, size_t len, size_t step_size,
								ALG_COMPARE compare, ALG_COPY copier,
								const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief 从堆中弹出一个元素
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:数组中元素的个数
 * @param step_size:指针下走的跨度(1表示一字节)
 * @param compare:比较回调函数
 * @param copier:拷贝回调函数
 * @param context:用户自定义的上下文数据
 * @param swap_buf:生成堆过程中用到的临时空间(可为null)
 * @param swap_buf_size:swap_buf的大小
 */
extern int MyAlgHeapPop(void * buf, size_t len, size_t step_size,
						ALG_COMPARE compare, ALG_COPY copier,
						const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief 推一个元素至堆中
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:数组中的元素个数
 * @param step_size:每个元素的大小
 * @param value:要加入的值
 * @param value_size:value的大小
 * @param context:用户自定义的上下文数据
 */
extern int MyAlgHeapPush(void * buf, size_t len, size_t step_size,
						 const void * value, size_t value_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context);

/**
 * @brief 调整一个堆
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的元素个数
 * @param step_size:每个元素的大小
 * @param hole_index:空洞节点的索引(0表示第一个节点)
 * @param value:值
 * @param value_size:值的大小
 * @param compare:比较回调(不可为空)
 * @param copier:如何拷贝(可为空)
 * @param context:用户自定义的上下文数据
 */
/*extern int MyAlgAdjustHeap(void * buf, size_t len, size_t step_size, 
						   size_t hole_index, const void * value, size_t value_size,
						   ALG_COMPARE compare, ALG_COPY copier,
						   const void * context);*/

/**
 * @brief 检查一个序列是否为一个堆
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的元素个数
 * @param step_size:每个元素的大小
 * @param compare:比较回调(不可为空)
 * @param context:用户自定义的上下文数据
 */
extern int MyAlgExaminHeap(void * buf, size_t len, size_t step_size,
						   ALG_COMPARE compare,
						   const void * context);

/**
 * @brief 检查一个序列是否为一个堆
 *
 * @param buf:要排序的数据缓冲区起始地址
 * @param len:要排序的元素个数
 * @param step_size:每个元素的大小
 * @param compare:比较回调(不可为空)
 * @param context:用户自定义的上下文数据
 */
extern int MyAlgSortOK(void * buf, size_t len, size_t step_size,
					   ALG_COMPARE compare,
					   const void * context);

/**
 * @brief 在一个有序序列里做二分查找
 *
 * @param buf:有序序列数据缓冲区起始地址
 * @param len:有序序列中的元素个数
 * @param step_size:每个元素的大小
 * @param key:要查找的关键字
 * @param compare:比较回调(不可为空)
 * @param pindex:如果找到,返回元素的位置,
 *               如果找不到,则返回元素应添加的位置,例如:
 *               1 3 4 6 7 --- 源序列,查找2,
 *               pindex将被置成[1],即表示2应添加到1与3之间
 *
 * @retval 0:成功
 * @retval 其它:失败
 */
extern int MyBinarySearch1(const void * buf, size_t len, size_t step_size,
						  const void * key,
						  ALG_COMPARE compare, size_t * pindex,
						  const void * context);


/**
 * @brief
 *  > 0  表示 key 大于 data
 *  == 0 表示 key 等于 data
 *  < 0  表示 key 小于 data
 *
 * @param key:关键字
 * @param key_sz:关键字的缓冲区大小
 * @param context:用户自定义的上下文数据
 * @param context_sz:用户自定义的上下文数据的长度
 */
typedef int (*BINSEARCH_COMPARE)(const void * key, unsigned int key_sz, 
								 const void * data2, 
								 const void * context, unsigned int context_sz);

/**
 * @brief 在一个有序序列里做二分查找
 *
 * @param buf:有序序列数据缓冲区起始地址
 * @param len:有序序列中的元素个数
 * @param step_size:每个元素的大小
 * @param key:要查找的关键字
 * @param key_sz:关键字缓冲的长度
 * @param compare:比较回调(不可为空)
 * @param pindex:如果找到,返回元素的位置,
 *               如果找不到,则返回元素应添加的位置,例如:
 *               1 3 4 6 7 --- 源序列,查找2,
 *               pindex将被置成[1],即表示2应添加到1与3之间
 * @param context:上下文信息
 * @param context_sz:上下文信息缓冲的长度
 *
 * @retval 0:成功
 * @retval 其它:失败
 */
extern int MyBinarySearch(const void * buf, unsigned int len, unsigned int step_size,
						  const void * key, unsigned int key_sz,
						  BINSEARCH_COMPARE compare, unsigned int * pindex,
						  const void * context, unsigned int context_sz);


#endif
















/**
 *
 * @file mydefmempool.h 内存池 2007-8-25 1:55
 *
 * @author lin shao chuan 
 *
 */
#ifndef __MYDEFMEMPOOL_H__
#define __MYDEFMEMPOOL_H__


#include "mymempool.h"


/**
 * @brief 内存池信息结构,用于MyMemPoolView
 */
typedef struct __rhapsody_info_t_
{
	int blk_count;
}rhapsody_info_t;


/**
 * @brief 创建一个内存池
 */
extern HMYMEMPOOL RhapsodyMemPoolConstruct();


#endif







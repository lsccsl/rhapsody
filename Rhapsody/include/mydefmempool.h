/**
 *
 * @file mydefmempool.h �ڴ�� 2007-8-25 1:55
 *
 * @author lin shao chuan 
 *
 */
#ifndef __MYDEFMEMPOOL_H__
#define __MYDEFMEMPOOL_H__


#include "mymempool.h"


/**
 * @brief �ڴ����Ϣ�ṹ,����MyMemPoolView
 */
typedef struct __rhapsody_info_t_
{
	int blk_count;
}rhapsody_info_t;


/**
 * @brief ����һ���ڴ��
 */
extern HMYMEMPOOL RhapsodyMemPoolConstruct();


#endif







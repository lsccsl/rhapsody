#ifndef __MY_CONFIG_H__
#define __MY_CONFIG_H__


#include "myutility.h"


#define __INLINE__ 

/**
 * 内存泄漏检测 
 */
//#define MEM_LEAK_CHECK 

/**
 * 4字节对齐 
 */
#define SYS_ALIGNMENT 4 

/**
 * @brief 计算对齐到4字节的补数 3->1
 */
#define CAL_SYS_ALIGMENT(x) CAL_ALIGMENT(x, SYS_ALIGNMENT)/*(SYS_ALIGNMENT - x % SYS_ALIGNMENT)*/

/* 类型定义 */
#ifdef WIN32
	typedef __int64 int64;
	typedef unsigned __int64 uint64;
#else
	typedef long long int int64;
	typedef unsigned long long int uint64;
#endif


#endif










#ifndef __MY_CONFIG_H__
#define __MY_CONFIG_H__


#include "myutility.h"


#define __INLINE__ 

/**
 * �ڴ�й©��� 
 */
//#define MEM_LEAK_CHECK 

/**
 * 4�ֽڶ��� 
 */
#define SYS_ALIGNMENT 4 

/**
 * @brief ������뵽4�ֽڵĲ��� 3->1
 */
#define CAL_SYS_ALIGMENT(x) CAL_ALIGMENT(x, SYS_ALIGNMENT)/*(SYS_ALIGNMENT - x % SYS_ALIGNMENT)*/

/* ���Ͷ��� */
#ifdef WIN32
	typedef __int64 int64;
	typedef unsigned __int64 uint64;
#else
	typedef long long int int64;
	typedef unsigned long long int uint64;
#endif


#endif










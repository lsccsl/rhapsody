/*
*
*MyStringSetEx.h from boost 字符串查找集合字符串查找集合-只需要"一次"字符串比较即可完成从无限个数的字符串集合里提取所需的字符串 lin shao chuan
*
*/


#ifndef __MYSTRINGSETEX_H__
#define __MYSTRINGSETEX_H__


#include "myrbtree.h"


typedef struct __handle_mystringset_ex
{
	int unused;
} * HMYSTRING_SET_EX;

typedef struct __handle_mystringset_ex_iter
{
	int unused;
} * HMYSTRING_SET_EX_ITER;


/*
*
*初始化字符串集合
*
*/
extern HMYSTRING_SET_EX MyStringSetExConstruct(HMYMEMPOOL hm);

/*
*
*反初始化字符串集合
*
*/
extern void MyStringSetExDestruct(HMYSTRING_SET_EX hss_ex);

/*
*
*反初始化字符串集合
*
*/
extern int MyStringSetExAdd(HMYSTRING_SET_EX hss_ex, const char * key, const void * data);

/*
*
*反初始化字符串集合
*
*/
extern int MyStringSetExDel(HMYSTRING_SET_EX hss_ex, const char * key, void ** data);

/*
*
*反初始化字符串集合
*
*/
extern int MyStringSetExSearch(HMYSTRING_SET_EX hss_ex, const char * key, void ** data);

#endif



























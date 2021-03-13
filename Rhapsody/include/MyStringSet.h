/*
*
*MyStringSet.h from boost 字符串查找集合字符串查找集合-只需要"一次"字符串比较即可完成从无限个数的字符串集合里提取所需的字符串 lin shao chuan
*
*/


#ifndef __MYSTRINGSET_H__
#define __MYSTRINGSET_H__


#include "mymempool.h"


typedef struct __mystringset_handle_
{int unused;}*HMYSTRING_SET;

typedef struct __mystringset_iter_
{int unused;}*HMYSTRING_SET_ITER;


/*
*
*创建字符串集合
*
*/
extern HMYSTRING_SET MyStringSetConstruct(HMYMEMPOOL hm);

/*
*
*创建字符串集合
*
*/
extern void MyStringSetDestruct(HMYSTRING_SET hss);

/*
*
*创建字符串集合
*
*/
extern int MyStringSetAdd(HMYSTRING_SET hss, char * first, const char * last, const void * data, size_t data_size);

/*
*
*创建字符串集合
*
*/
extern int MyStringSetDel(HMYSTRING_SET hss, char * first, const char * last, void ** data, size_t * data_size);

/*
*
*创建字符串集合
*
*/
extern int MyStringSetSearch(HMYSTRING_SET hss, char * first, const char * last, void ** data, size_t * data_size);


#endif




















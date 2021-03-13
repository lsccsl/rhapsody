/*
*
* string_set.h from boost 字符串查找集合-只需要"一次"字符串比较即可完成从无限个数的字符串集合里提取所需的字符串 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/


#ifndef __STRING_SET_H__
#define __STRING_SET_H__


#include "myobj.h"
#include "mymempool.h"


typedef struct __string_set_handle_
{int unused;}*HSTRING_SET;


/*
*
*创建字符串集
*
*/
extern HSTRING_SET StringSetConstruct(HMYMEMPOOL hm, myobj_ops * data_ops);

/*
*
*创建字符串集
*
*/
extern void StringSetDestruct(HSTRING_SET hss);

/*
*
*添加字符串
*
*/
extern int StringSetAdd(const HSTRING_SET hss, char * first, const char * last, unsigned long data, size_t data_size);

/*
*
*添加字符串
*
*/
extern int StringSetSearch(const HSTRING_SET hss, char * first, const char * last, unsigned long * data, size_t * data_size);

/*
*
*删除一个指定序列
*
*/
extern int StringSetDel(const HSTRING_SET hss, char * first, const char * last, unsigned long * data, size_t * data_size);


#endif














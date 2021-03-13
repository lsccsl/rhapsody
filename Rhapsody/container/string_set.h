/*
*
* string_set.h from boost �ַ������Ҽ���-ֻ��Ҫ"һ��"�ַ����Ƚϼ�����ɴ����޸������ַ�����������ȡ������ַ��� 
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
*�����ַ�����
*
*/
extern HSTRING_SET StringSetConstruct(HMYMEMPOOL hm, myobj_ops * data_ops);

/*
*
*�����ַ�����
*
*/
extern void StringSetDestruct(HSTRING_SET hss);

/*
*
*����ַ���
*
*/
extern int StringSetAdd(const HSTRING_SET hss, char * first, const char * last, unsigned long data, size_t data_size);

/*
*
*����ַ���
*
*/
extern int StringSetSearch(const HSTRING_SET hss, char * first, const char * last, unsigned long * data, size_t * data_size);

/*
*
*ɾ��һ��ָ������
*
*/
extern int StringSetDel(const HSTRING_SET hss, char * first, const char * last, unsigned long * data, size_t * data_size);


#endif














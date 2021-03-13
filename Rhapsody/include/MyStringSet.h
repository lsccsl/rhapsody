/*
*
*MyStringSet.h from boost �ַ������Ҽ����ַ������Ҽ���-ֻ��Ҫ"һ��"�ַ����Ƚϼ�����ɴ����޸������ַ�����������ȡ������ַ��� lin shao chuan
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
*�����ַ�������
*
*/
extern HMYSTRING_SET MyStringSetConstruct(HMYMEMPOOL hm);

/*
*
*�����ַ�������
*
*/
extern void MyStringSetDestruct(HMYSTRING_SET hss);

/*
*
*�����ַ�������
*
*/
extern int MyStringSetAdd(HMYSTRING_SET hss, char * first, const char * last, const void * data, size_t data_size);

/*
*
*�����ַ�������
*
*/
extern int MyStringSetDel(HMYSTRING_SET hss, char * first, const char * last, void ** data, size_t * data_size);

/*
*
*�����ַ�������
*
*/
extern int MyStringSetSearch(HMYSTRING_SET hss, char * first, const char * last, void ** data, size_t * data_size);


#endif




















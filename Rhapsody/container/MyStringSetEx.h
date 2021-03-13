/*
*
*MyStringSetEx.h from boost �ַ������Ҽ����ַ������Ҽ���-ֻ��Ҫ"һ��"�ַ����Ƚϼ�����ɴ����޸������ַ�����������ȡ������ַ��� lin shao chuan
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
*��ʼ���ַ�������
*
*/
extern HMYSTRING_SET_EX MyStringSetExConstruct(HMYMEMPOOL hm);

/*
*
*����ʼ���ַ�������
*
*/
extern void MyStringSetExDestruct(HMYSTRING_SET_EX hss_ex);

/*
*
*����ʼ���ַ�������
*
*/
extern int MyStringSetExAdd(HMYSTRING_SET_EX hss_ex, const char * key, const void * data);

/*
*
*����ʼ���ַ�������
*
*/
extern int MyStringSetExDel(HMYSTRING_SET_EX hss_ex, const char * key, void ** data);

/*
*
*����ʼ���ַ�������
*
*/
extern int MyStringSetExSearch(HMYSTRING_SET_EX hss_ex, const char * key, void ** data);

#endif



























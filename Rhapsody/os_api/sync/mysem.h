/*
*
*mysem.h �ź��� lin shao chuan
*
*/
#ifndef __MYSEM_H__
#define __MYSEM_H__


#include "mymempool.h"


typedef struct __mysem_handle_
{int unused;}*HMYSEM;


/*
*
*�����ź���
*
*/
extern HMYSEM MySemRealConstruct(HMYMEMPOOL hm, int value, const char * pcname);
#define MySemConstruct(__hm_, __value_) MySemRealConstruct(__hm_, __value_, NULL)

/*
*
*�����ź���
*
*/
extern void MySemDestruct(HMYSEM hsem);

/*
*
*�����ź���
*
*/
extern void MySemInCrease(HMYSEM hsem);

/*
*
*�����ź���
*
*/
extern void MySemDeCrease(HMYSEM hsem);

#endif

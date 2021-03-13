/*
*
*mysem.h 信号量 lin shao chuan
*
*/
#ifndef __MYSEM_H__
#define __MYSEM_H__


#include "mymempool.h"


typedef struct __mysem_handle_
{int unused;}*HMYSEM;


/*
*
*创建信号量
*
*/
extern HMYSEM MySemRealConstruct(HMYMEMPOOL hm, int value, const char * pcname);
#define MySemConstruct(__hm_, __value_) MySemRealConstruct(__hm_, __value_, NULL)

/*
*
*销毁信号量
*
*/
extern void MySemDestruct(HMYSEM hsem);

/*
*
*增加信号量
*
*/
extern void MySemInCrease(HMYSEM hsem);

/*
*
*减少信号量
*
*/
extern void MySemDeCrease(HMYSEM hsem);

#endif

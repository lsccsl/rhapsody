/*
*
*mymutex.h ������ lin shao chuan
*
*/
#ifndef __MYMUTEX_H__
#define __MYMUTEX_H__

#include "mymempool.h"


typedef struct __mymutex_handle_
{int unused;}*HMYMUTEX;


/*
*
*����������
*
*/
extern HMYMUTEX MyMutexRealConstruct(HMYMEMPOOL hm, const char * pcname);
#define MyMutexConstruct(__hm) MyMutexRealConstruct(__hm, NULL)

/*
*
*���ٻ�����
*
*/
extern void MyMutexDestruct(HMYMUTEX hmx);

/*
*
*����
*
*/
extern int MyMutexLock(HMYMUTEX hmx);
extern int MyMutexTryLock(HMYMUTEX hmx);

/*
*
*����
*
*/
extern int MyMutexUnLock(HMYMUTEX hmx);


#endif














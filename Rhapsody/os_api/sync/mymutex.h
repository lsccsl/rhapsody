/*
*
*mymutex.h »¥³âËø lin shao chuan
*
*/
#ifndef __MYMUTEX_H__
#define __MYMUTEX_H__

#include "mymempool.h"


typedef struct __mymutex_handle_
{int unused;}*HMYMUTEX;


/*
*
*´´½¨»¥³âËø
*
*/
extern HMYMUTEX MyMutexRealConstruct(HMYMEMPOOL hm, const char * pcname);
#define MyMutexConstruct(__hm) MyMutexRealConstruct(__hm, NULL)

/*
*
*Ëø»Ù»¥³âËø
*
*/
extern void MyMutexDestruct(HMYMUTEX hmx);

/*
*
*¼ÓËø
*
*/
extern int MyMutexLock(HMYMUTEX hmx);
extern int MyMutexTryLock(HMYMUTEX hmx);

/*
*
*½âËø
*
*/
extern int MyMutexUnLock(HMYMUTEX hmx);


#endif














#ifndef __GETTIMEOFDAY_H__
#define __GETTIMEOFDAY_H__

#ifdef WIN32	
	extern void gettimeofday(struct timeval *ptv, void *tzp);
#else
	#include <sys/time.h>
#endif

#endif

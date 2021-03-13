#include "myos.h"

#ifdef WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif

extern void os_msleep(int msec)
{
#ifdef WIN32
	Sleep(msec);
#else
	usleep(msec * 1000);
#endif
}


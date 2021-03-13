#ifdef WIN32
	#include "mythread_win32.c"
#else
	#include "mythread_linux.c"
#endif


#ifdef WIN32
	#include "mymutex_win32.c"
#else
	#include "mymutex_linux.c"
#endif


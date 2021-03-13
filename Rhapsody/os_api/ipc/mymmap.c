#ifdef WIN32
	#include "mymmap_win32.c"
#else
	#include "mymmap_linux.c"
#endif

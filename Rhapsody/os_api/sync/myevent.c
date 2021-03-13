
#ifdef WIN32
	#include "myevent_win32.c"
#else
//	#include "myevent_pipe.c"
	#include "myevent_linux.c"
#endif

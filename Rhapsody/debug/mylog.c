#include "mylog.h"

#include <stdarg.h>
#include <stdio.h>

void log_err(char * fmt, ...)
{
	va_list var;

#ifndef WIN32
	printf("\033[1;31m");
#endif

	va_start(var, fmt);
	vprintf(fmt, var);
	va_end(var);

#ifndef WIN32
	printf("\033[0m");
#endif
}

void log_warn(char * fmt, ...)
{
	va_list var;

#ifndef WIN32
	printf("\033[1;33m");
#endif

	va_start(var, fmt);
	vprintf(fmt, var);
	va_end(var);

#ifndef WIN32
	printf("\033[0m");
#endif
}

void log_info(char * fmt, ...)
{
	va_list var;

#ifndef WIN32
	printf("\033[1;32m");
#endif

	va_start(var, fmt);
	vprintf(fmt, var);
	va_end(var);

#ifndef WIN32
	printf("\033[0m");
#endif
}

void log_debug(char * fmt, ...)
{
	va_list var;

	va_start(var, fmt);
	vprintf(fmt, var);
	va_end(var);

#ifndef WIN32
	printf("\033[0m");
#endif
}








#include <time.h>
#include <winsock2.h>

/*void TimevalToSystemTime(struct timeval *ptv, LPSYSTEMTIME pst)
{
	FILETIME ft;
	if(!ptv || !pst)
		goto Exit;
	TimevalToFileTime(ptv, &ft);
	FileTimeToSystemTime(&ft, pst);
Exit:
	;
}*/

void FileTimeToTimeval(LPFILETIME pft, struct timeval * ptv)
{ /* Note that LONGLONG is a 64-bit value */
	LONGLONG ll;
	if(!pft || !ptv)
		goto Exit;
	ll = ((LONGLONG) pft->dwHighDateTime << 32);
	ll += (LONGLONG) pft->dwLowDateTime;
#ifdef __GNUC__
	ll -= 116444736000000000ll;
#else
	ll -= 116444736000000000;
#endif
	ptv->tv_sec = (long) (ll / 10000000);
	ptv->tv_usec = (long) (ll - ((LONGLONG)(ptv->tv_sec) * 10000000)) / 10;
Exit:;
}

void gettimeofday(struct timeval *ptv, void *tzp)
{
	static int QueryCounter = 2;
	FILETIME CurrentTime;
	if(!ptv)
		goto Exit;

	if(QueryCounter)
	{
		static LARGE_INTEGER Frequency;
		static LARGE_INTEGER Offset;
		static LARGE_INTEGER LastCounter;
		LARGE_INTEGER Time;
		LARGE_INTEGER Counter;
/* HANDLE hThread = GetCurrentThread(); 
int ThreadPrio = GetThreadPriority(hThread);

SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL); */
		GetSystemTimeAsFileTime(&CurrentTime);
		QueryPerformanceCounter(&Counter);
/* SetThreadPriority(hThread, ThreadPrio); */

		if(QueryCounter == 2)
		{
			QueryCounter = 1;
			if(!QueryPerformanceFrequency(&Frequency))
			{
				QueryCounter = 0;
				Frequency.QuadPart = 10000000; /* prevent division by 0 */
			}

/* get time as a large integer */
			Counter.HighPart &= 0x7fl; /* Clear the highest bits to prevent overflows */
			Offset.LowPart = CurrentTime.dwLowDateTime;
			Offset.HighPart = (LONG) CurrentTime.dwHighDateTime;
			Offset.QuadPart -= Counter.QuadPart * 10000000 / Frequency.QuadPart;
		}		

/* Convert counter to a 100 nanoseconds resolution timer value. */ 

		Counter.HighPart &= 0x7fl; /* Clear the highest bits to prevent overflows */
		Counter.QuadPart *= 10000000; /* Because we need time stamp in units of 100 ns */
		Counter.QuadPart /= Frequency.QuadPart; /* counter of 0.1 microseconds */

		if(LastCounter.QuadPart > Counter.QuadPart) 
		{ /* Counter value wrapped */
#ifdef __GNUC__
Offset.QuadPart += (0x7f00000000ll * 10000000ll) / Frequency.QuadPart;
#else
Offset.QuadPart += (0x7f00000000 * 10000000) / Frequency.QuadPart;
#endif
		}
		LastCounter = Counter;

/* Add the in previous call calculated offset */
		Counter.QuadPart += Offset.QuadPart;

/* get time as a large integer */
		Time.LowPart = CurrentTime.dwLowDateTime;
		Time.HighPart = (LONG) CurrentTime.dwHighDateTime;

/* keep time difference within an interval of +- 0.1 seconds
relative to the time function by adjusting the counters offset */

		if(((Time.QuadPart + 1000000) < Counter.QuadPart) ||
			((Time.QuadPart - 1000000) > Counter.QuadPart))
		{ /* Adjust the offset */
			Offset.QuadPart += Time.QuadPart - Counter.QuadPart;
			Counter.QuadPart = Time.QuadPart;
		}

/* Now let's use the adjusted performance counter time for the time stamp */
		CurrentTime.dwLowDateTime = Counter.LowPart;
		CurrentTime.dwHighDateTime = Counter.HighPart;
	}
	else
	{
		GetSystemTimeAsFileTime(&CurrentTime);
	}

	FileTimeToTimeval(&CurrentTime,ptv);

	Exit:;
}/* int gettimeofday(struct timeval *ptv, void *tzp) */
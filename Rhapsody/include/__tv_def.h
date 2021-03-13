#ifndef __TV_DEF_H__
#define __TV_DEF_H__

typedef struct __mytv_t_
{
	long tv_sec;	/* ���� */
	long tv_usec;	/* ΢��/�����֮һ�� */
}mytv_t;

/*
*
*t1�Ƿ�С��t2
*
*/
#define timeval_smaller(t1, t2) \
	( ((t1).tv_sec < (t2).tv_sec) ? 1 : ( (((t1).tv_usec < (t2).tv_usec) && ((t1).tv_sec == (t2).tv_sec)) ? 1:0) )

/*
*
*t1�Ƿ����t2
*
*/
#define timeval_greater(t1, t2) \
	( ((t1).tv_sec > (t2).tv_sec) ? 1 : ( (((t1).tv_usec > (t2).tv_usec) && ((t1).tv_sec == (t2).tv_sec)) ? 1:0) )


/*
*
*t1 += t2
*
*/
#define timeval_add(t1, t2) do{\
	(t1).tv_sec += (t2).tv_sec;\
	(t1).tv_usec += (t2).tv_usec;\
	if((t1).tv_usec >= (1000 * 1000))\
	{\
		(t1).tv_sec += (t1).tv_usec/(1000 * 1000);\
		(t1).tv_usec = (t1).tv_usec % (1000 * 1000);\
	}\
	else if((t1).tv_usec < 0 && (t1).tv_sec >= 1)\
	{\
		(t1).tv_sec -= -1;\
		(t1).tv_usec = 1000 * 1000 + (t1).tv_usec;\
	}\
}while(0)

/*
*
*t1 -= t2
*
*/
#define timeval_minus(t1, t2) do{\
	(t1).tv_sec -= (t2).tv_sec;\
	(t1).tv_usec -= (t2).tv_usec;\
	if((t1).tv_usec >= (1000 * 1000))\
	{\
		(t1).tv_sec += (t1).tv_usec/(1000 * 1000);\
		(t1).tv_usec = (t1).tv_usec % (1000 * 1000);\
	}\
	else if((t1).tv_usec < 0 && (t1).tv_sec >= 1)\
	{\
		(t1).tv_sec -= 1;\
		(t1).tv_usec = 1000 * 1000 + (t1).tv_usec;\
	}\
}while(0)

#endif

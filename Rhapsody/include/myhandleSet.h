/*
*
*myhandleSet.h fd���� lin shao chuan
*
*/
#ifndef __MYHANDLE_SET_H__
#define __MYHANDLE_SET_H__


#include "mymempool.h"
#include "myvector.h"

struct timeval;

struct __myhandleSet_t_;
typedef struct __myhandleSet_t_ * HMYHANDLESET;


enum E_HANDLE_SET_MASK
{
	E_FD_READ = 0x01,
	E_FD_WRITE = 0x02,
	E_FD_EXCEPTION = 0x04,
};


/*
*
*����
*
*/
extern HMYHANDLESET MyHandleSetConstruct(HMYMEMPOOL hm);

/*
*
*����
*
*/
extern void MyHandleSetDestruct(HMYHANDLESET hs);

/*
*
*���һ��fd
*@retval:
	0:�ɹ�
	-1:���ȫ��ʧ��
	-2:��Ӷ�ʧ��
	-3:���дʧ��
	-4:����쳣ʧ��
*
*/
extern int MyHandleSetFdSet(HMYHANDLESET hs, int fd, unsigned long mask);

/*
*
*ɾ����һ��fd
*
*/
extern void MyHandleSetDelFd(HMYHANDLESET hs, int fd);

/*
*
*select
*@param
	timeout:��ʱ��null��ʾ���޵ȴ�
*@retval
	0:��ʾ��ʱ ��-1���ʾ�����¼��ľ���� -1:��ʾʧ��
*
*/
extern int MyHandleSetSelect(HMYHANDLESET hs, struct timeval * timeout);

/*
*
*ȡ�����з����¼���fd,����յ�ǰ�����¼���fd����
*@param
	hvRead:��ʾ�ɶ���fd����
	hvWrite:��ʾ��д��fd����
	hvException:��ʾ�����fd����
*
*/	
extern void MyHandleSetGetAllSignalFd(HMYHANDLESET hs,
	HMYVECTOR read_set,
	HMYVECTOR write_set,
	HMYVECTOR exception_set);

/*
*
*�ж�fd�Ƿ���mask��ָ�����¼�,������Ӧ�ļ�����ɾ��fd
*
*/	
extern unsigned long MyHandleSetIsSet(HMYHANDLESET hs, int fd, unsigned long mask);


#endif


















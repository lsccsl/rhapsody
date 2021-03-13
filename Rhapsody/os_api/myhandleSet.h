/*
*
*myhandleSet.h fd集合 lin shao chuan
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
*创建
*
*/
extern HMYHANDLESET MyHandleSetConstruct(HMYMEMPOOL hm);

/*
*
*销毁
*
*/
extern void MyHandleSetDestruct(HMYHANDLESET hs);

/*
*
*添加一个fd
*@retval:
	0:成功
	-1:添加全部失败
	-2:添加读失败
	-3:添加写失败
	-4:添加异常失败
*
*/
extern int MyHandleSetFdSet(HMYHANDLESET hs, int fd, unsigned long mask);

/*
*
*删除加一个fd
*
*/
extern void MyHandleSetDelFd(HMYHANDLESET hs, int fd);

/*
*
*select
*@param
	timeout:超时，null表示无限等待
*@retval
	0:表示超时 非-1则表示发生事件的句柄数 -1:表示失败
*
*/
extern int MyHandleSetSelect(HMYHANDLESET hs, struct timeval * timeout);

/*
*
*取出所有发生事件的fd,并清空当前发生事件的fd集合
*@param
	hvRead:表示可读的fd集合
	hvWrite:表示可写的fd集合
	hvException:表示出错的fd集合
*
*/	
extern void MyHandleSetGetAllSignalFd(HMYHANDLESET hs,
	HMYVECTOR read_set,
	HMYVECTOR write_set,
	HMYVECTOR exception_set);

/*
*
*判断fd是否发生mask中指定的事件,并从相应的集合中删除fd
*
*/	
extern unsigned long MyHandleSetIsSet(HMYHANDLESET hs, int fd, unsigned long mask);


#endif


















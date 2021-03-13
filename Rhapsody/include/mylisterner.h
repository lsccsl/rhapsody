/*
*
*mylisterner.h 定时器线程 lin shao chuan
*
* 参照ACE的Select_Reactor实现以下功能:
*  监听设备句柄
*  监听消息队列(优先级与无优先级)
*  提供定时器接口
*
* 暂未实现的接口:
*	提供sysv消息队列监听
*	仿照ACE提供suspend resume fd类似的接口
*	删除监听fd的接口
*
*/
#ifndef __MYLISTERNER_H__
#define __MYLISTERNER_H__

#include "mymempool.h"
#include "mytimerheap.h"
#include "myhandleSet.h"


struct __mylisterner_t_;
typedef struct __mylisterner_t_ * HMYLISTERNER;


/*处理有输入事件的回调函数*/
typedef int (*LISTERNER_HANDLE_INPUT)(unsigned long context_data, int fd);

/*处理有输出事件的回调函数*/
typedef int (*LISTERNER_HANDLE_OUTPUT)(unsigned long context_data, int fd);

/*处理有异常事件的回调函数*/
typedef int (*LISTERNER_HANDLE_EXCEPTION)(unsigned long context_data, int fd);

/*处理消息队列里的消息*/
typedef int (*CB_LISTERNER_HANDLE_MSG)(unsigned long context_data, void * msg);

typedef struct __event_handle_t_
{
	LISTERNER_HANDLE_INPUT input;
	LISTERNER_HANDLE_OUTPUT output;
	LISTERNER_HANDLE_EXCEPTION exception;

	/*用户处理事件时的上下文数据*/
	unsigned long context_data;
}event_handle_t;


/*
*
* 构造监听线程
*
*/
extern HMYLISTERNER MyListernerConstruct(HMYMEMPOOL hm, size_t max_msg_count);

/*
*
* 析构监听线程
*
*/
extern void MyListernerDestruct(HMYLISTERNER hlisterner);

/*
*
* 运行监听线程
*
*/
extern void MyListernerRun(HMYLISTERNER hlisterner);

/*
*
* 等待listern线程退出
*
*/
extern void MyListernerWait(HMYLISTERNER hlisterner);

/*
*
* 添加定时器
*
*/
extern HTIMERID MyListernerAddTimer(HMYLISTERNER hlisterner, mytimer_node_t * node);

/*
*
* 删除定时器
*
*/
extern int MyListernerDelTimer(HMYLISTERNER hlisterner, HTIMERID timer_id);

/*
*
* 重置定时器
*
*/
extern HTIMERID MyListernerResetTimer(HMYLISTERNER hlisterner, HTIMERID timer_id, mytimer_node_t * node);

/*
*
* 添加文件扫描符
*
*/
extern int MyListernerAddFD(HMYLISTERNER hlisterner, int fd, enum E_HANDLE_SET_MASK mask, event_handle_t * evt_handle);

/*
*
* 删除文件扫描符
*
*/
extern int MyListernerDelFD(HMYLISTERNER hlisterner, int fd);

/*
*
* 添加一条消息 
*
*/
extern int MyListernerAddMsg(HMYLISTERNER hlisterner, 
							 const void * user_msg, 
							 unsigned long context_data,
							 CB_LISTERNER_HANDLE_MSG handle);

/*
*
* 添加一条带优先级的消息
*
*/
extern int MyListernerAddProrityMsg(HMYLISTERNER hlisterner, 
									int prority, 
									const void * user_msg, 
									unsigned long context_data,
									CB_LISTERNER_HANDLE_MSG handle);

/*
*
* 获取定时器个数
*
*/
int MyListernerPrint(HMYLISTERNER hlisterner, char * pctemp, size_t sz);


#endif
















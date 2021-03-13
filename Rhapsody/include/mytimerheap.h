/*
* mytimerheap.h 定时堆 lin shao chuan
*
* changelog:
* <lsccsl@tom.com 2008 09:17>
*			去除对timeval这个结构体的依赖,防止引入<winsock2.h>之类的头文件.
*			在超时回调函数中,补充定时器id参数
*/

#ifndef __MYTIMERHEAP_H__
#define __MYTIMERHEAP_H__


#include "mymempool.h"
#include "__tv_def.h"

typedef struct __mytimerheap_handle_
{int unused;}*HMYTIMERHEAP;

typedef void * HTIMERID;

/*
*超时触发的回调函数
*@param 
	context_data:触发回调时的用户上下文
	timer_user_data:定时器节点相关的用户数据
	timerid:定时器的id
*/
typedef int (*MY_TIMER_HEAP_TIMEOUT_CB)(unsigned long context_data, 
										unsigned long timer_user_data,
										HTIMERID timerid);

typedef struct __mytimer_node_t_
{
	unsigned long context_data;				/*触发回调时的用户上下文*/
	unsigned long timer_user_data;			/*定时器节点相关的用户数据*/
	MY_TIMER_HEAP_TIMEOUT_CB timeout_cb;	/*定时器回调函数*/

	mytv_t period;							/* 定时超时间隔*/
	mytv_t first_expire;					/* <第一次> 相对超时时间*/

	mytv_t abs_expire;						/* <下一次> 绝对超时时间 此域对用户无用 */
}mytimer_node_t;


/*
*
* 构造定时堆
*
*/
extern HMYTIMERHEAP MyTimerHeapConstruct(HMYMEMPOOL hm);

/*
*
* 析构定时堆
*
*/
extern void MyTimerHeapDestruct(HMYTIMERHEAP hth);

/*
*
* 添加定时器
*
*@param 
	mytimer_node_t * node:定时器节点
*@retval 定时器的id NULL:失败
*
*/
extern HTIMERID MyTimerHeapAdd(HMYTIMERHEAP hth, mytimer_node_t * node);

/*
*
* 删除定时器
*
*/
extern int MyTimerHeapDel(HMYTIMERHEAP hth, HTIMERID timerid);

/*
*
* 重置定时器
*
*@param 
	mytimer_node_t * node:定时器节点
*@retval 定时器的id NULL:失败
*
*/
extern HTIMERID MyTimerHeapReset(HMYTIMERHEAP hth, HTIMERID timerid, mytimer_node_t * node);

/*
*
* 获取头节点,即绝对超时最小(最早超时)的键值
*
*/
extern HTIMERID MyTimeHeapGetEarliestKey(HMYTIMERHEAP hth);

/*
*
* 获取最小超时(绝对时间)
*
*@retval 0:成功 -1:失败
*
*/
extern int MyTimerHeapGetEarliestExpire(HMYTIMERHEAP hth, mytv_t * expire);

/*
*
* 触发所有超时事件
*
*@retval 超时个数
*
*/
extern unsigned MyTimerHeapRunExpire(HMYTIMERHEAP hth, mytv_t * tv_now);

/*
*
* 获取定时器的个数
*
*/
extern size_t MyTimerHeapGetCount(HMYTIMERHEAP hth);

/*
*
* 输出timer heap里的信息
*
*/
extern void MyTimerHeapPrint(HMYTIMERHEAP hth);


#endif




























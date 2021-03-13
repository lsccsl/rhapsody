/**
 *
 * @file mysysvmsg.c sv消息队列 2007-8-24 21:03
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __MYSYSVMSGQ_H__
#define __MYSYSVMSGQ_H__


#include "mymempool.h"


typedef struct __mysvmsgq_handle_
{int unused;}*HMYSVMSGQ;

 
/**
 * @brief 创建消息队列
 * @param key:消息队列的关键字
 * @param notify_pipe_name:管道通知的名称,为null则不打开通知管道
 * @param bcreate:表示创建
 * @param max_size:表示消息队列的大小
 */
extern HMYSVMSGQ MySVMsgQConstruct(HMYMEMPOOL hm, unsigned long key, int bcreate, size_t max_size, const char * notify_pipe_name);

/**
 * @brief 获取消息队列
 * @param qid:消息队列的标识
 * @param notify_pipe_name:管道通知的名称,为null则不打开通知管道
 */
extern HMYSVMSGQ MySVMsgQGet(HMYMEMPOOL hm, int qid, const char * notify_pipe_name);

/**
 * @brief 销毁消息队列
 */
extern void MySVMsgQDestruct(HMYSVMSGQ hmq);

/**
 * @brief 写消息
 */
extern int MySVMsgQWrite(HMYSVMSGQ hmq, const void * buf, size_t buf_len);

/**
 * @brief 读消息
 */
extern size_t MySVMsgQRead(HMYSVMSGQ hmq, void * buf, size_t buf_len, int bblock);

/**
 * @brief 获取通知管道的fd
 */
extern int MySVMsgQGetSelectFd(HMYSVMSGQ hmq);

/**
 * @brief 获取通知管道的fd
 */
extern int MySVMsgQGetID(HMYSVMSGQ hmq);


#endif


















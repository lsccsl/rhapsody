/**
 *
 * @file mysysvmsg.c sv��Ϣ���� 2007-8-24 21:03
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
 * @brief ������Ϣ����
 * @param key:��Ϣ���еĹؼ���
 * @param notify_pipe_name:�ܵ�֪ͨ������,Ϊnull�򲻴�֪ͨ�ܵ�
 * @param bcreate:��ʾ����
 * @param max_size:��ʾ��Ϣ���еĴ�С
 */
extern HMYSVMSGQ MySVMsgQConstruct(HMYMEMPOOL hm, unsigned long key, int bcreate, size_t max_size, const char * notify_pipe_name);

/**
 * @brief ��ȡ��Ϣ����
 * @param qid:��Ϣ���еı�ʶ
 * @param notify_pipe_name:�ܵ�֪ͨ������,Ϊnull�򲻴�֪ͨ�ܵ�
 */
extern HMYSVMSGQ MySVMsgQGet(HMYMEMPOOL hm, int qid, const char * notify_pipe_name);

/**
 * @brief ������Ϣ����
 */
extern void MySVMsgQDestruct(HMYSVMSGQ hmq);

/**
 * @brief д��Ϣ
 */
extern int MySVMsgQWrite(HMYSVMSGQ hmq, const void * buf, size_t buf_len);

/**
 * @brief ����Ϣ
 */
extern size_t MySVMsgQRead(HMYSVMSGQ hmq, void * buf, size_t buf_len, int bblock);

/**
 * @brief ��ȡ֪ͨ�ܵ���fd
 */
extern int MySVMsgQGetSelectFd(HMYSVMSGQ hmq);

/**
 * @brief ��ȡ֪ͨ�ܵ���fd
 */
extern int MySVMsgQGetID(HMYSVMSGQ hmq);


#endif


















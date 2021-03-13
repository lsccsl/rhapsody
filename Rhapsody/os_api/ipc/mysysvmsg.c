/**
 *
 * @file mysysvmsg.c sv消息队列 2007-8-24 21:03
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "mysysvmsg.h"

#include <assert.h>
#include <sys/types.h>
#include <linux/msg.h>
#include <string.h>

#include "mynamepipe.h"
#include "myutility.h"
#include "mylog.h"
#include "os_def.h"


typedef struct __mysvmq_t_
{
	/*
	* 消息队列的键值
	*/
	key_t key;
	
	/*
	* 消息队列的id
	*/
	int qid;
	
	/*
	* 消息队列是否由当前对象创建的
	*/
	int bcreate;
	
	/*
	* 消息写通知管道
	*/
	HMYNAMEPIPE hnotify_pipe;
	
	HMYMEMPOOL hm;
}mysvmq_t;


typedef struct __sysv_notify_tag_
{
	int unused;
}sysv_notify_tag;


static __INLINE__ int msgq_create_inter(key_t key, int bcreate)
{	
	int ret_msgq_id = -1;

	ret_msgq_id = msgget(key, 0);

	/* 不创建,只获取 */
	if(ret_msgq_id >= 0 || !bcreate)
		return ret_msgq_id;	

	/* 创建 */
	ret_msgq_id = msgget(key, IPC_CREAT|0777);	
	if(ret_msgq_id < 0)
	{
		LOG_WARN(("fail create sv msgq %d", key));
		return -1;
	}
	
	return ret_msgq_id;
}

static __INLINE__ void msgq_destroy(mysvmq_t * q)
{
	assert(q);

	if(q->bcreate)
		msgctl(q->qid, IPC_RMID, NULL);

	if(q->hnotify_pipe)
		MyNamePipeDestruct(q->hnotify_pipe);

	MyMemPoolFree(q->hm, q);
}


/**
 * @brief 创建消息队列
 * @param key:消息队列的关键字
 * @param notify_pipe_name:管道通知的名称,为null则不打开通知管道
 */
HMYSVMSGQ MySVMsgQConstruct(HMYMEMPOOL hm, unsigned long key, int bcreate, size_t max_size, const char * notify_pipe_name)
{
	struct msqid_ds * buf = NULL;

	mysvmq_t * q = (mysvmq_t *)MyMemPoolMalloc(hm, sizeof(*q));
	if(NULL == q)
		return NULL;
		
	q->hm = hm;

	q->bcreate = bcreate;
	q->key = key;
	q->qid = msgq_create_inter(q->key, q->bcreate);
	if(-1 == q->qid)
		goto MySVMsgQConstruct_err_;
	
	/*
	* 根据需要要创建通知管道
	*/
	if(notify_pipe_name)
		q->hnotify_pipe = MyNamePipeConstruct(hm, notify_pipe_name, bcreate);
	else
		q->hnotify_pipe = NULL;
	
	/*
	* 不需要设置消息队列的大小
	* 或者只是获取消息队列
	*/
	if(0 == max_size || 0 == bcreate)
		return (HMYSVMSGQ)q;

	/* 需要设置消息队列的大小 */
	buf = (struct msqid_ds *)MyMemPoolMalloc(hm, sizeof(*buf));
	if(-1 != msgctl(q->qid, IPC_STAT, buf))
	{
		buf->msg_qbytes = max_size;
		msgctl(q->qid, IPC_SET, buf);
	}
	MyMemPoolFree(hm, buf);

	return (HMYSVMSGQ)q;
	
MySVMsgQConstruct_err_:
	
	msgq_destroy(q);
	
	return NULL;
}

/**
 * @brief 获取消息队列
 * @param qid:消息队列的标识
 * @param notify_pipe_name:管道通知的名称,为null则不打开通知管道
 */
HMYSVMSGQ MySVMsgQGet(HMYMEMPOOL hm, int qid, const char * notify_pipe_name)
{
	mysvmq_t * q = (mysvmq_t *)MyMemPoolMalloc(hm, sizeof(*q));
	if(NULL == q)
		return NULL;

	q->hm = hm;

	q->bcreate = 0;
	q->key = 0;
	q->qid = qid;

	/*
	* 根据需要要创建通知管道
	*/
	if(notify_pipe_name)
		q->hnotify_pipe = MyNamePipeConstruct(hm, notify_pipe_name, q->bcreate);
	else
		q->hnotify_pipe = NULL;

	return (HMYSVMSGQ)q;
}

/**
 * @brief 销毁消息队列
 */
void MySVMsgQDestruct(HMYSVMSGQ hmq)
{
	mysvmq_t * q = (mysvmq_t *)hmq;
	if(NULL == q)
		return;

	msgq_destroy(q);
}

/**
 * @brief 写消息
 */
int MySVMsgQWrite(HMYSVMSGQ hmq, const void * buf, size_t buf_len)
{
	sysv_notify_tag nt;
	struct msgbuf * sndbuf = NULL;

	mysvmq_t * q = (mysvmq_t *)hmq;
	if(NULL == q || NULL == buf || 0 == buf_len)
		return -1;
		
	sndbuf = (struct msgbuf *)MyMemPoolMalloc(q->hm, sizeof(sndbuf->mtype) + buf_len);
	if(NULL == sndbuf)
		return -1;
	
	sndbuf->mtype = 1;
	memcpy(sndbuf->mtext, buf, buf_len);
	
	if(0 == msgsnd(q->qid, sndbuf, sizeof(sndbuf->mtype) + buf_len, IPC_NOWAIT) && q->hnotify_pipe)
		MyNamePipeWrite(q->hnotify_pipe, &nt, sizeof(nt));

	MyMemPoolFree(q->hm, sndbuf);
	
	return 0;
}

/**
 * @brief 读消息
 */
size_t MySVMsgQRead(HMYSVMSGQ hmq, void * buf, size_t buf_len, int bblock)
{
	ssize_t ret = 0;
	sysv_notify_tag nt;
	int flag = 0;
	struct msgbuf * sndbuf = (struct msgbuf *)buf;

	mysvmq_t * q = (mysvmq_t *)hmq;
	if(NULL == q || NULL == buf || sizeof(sndbuf->mtype) >= buf_len)
		return 0;

	if(!bblock)
		flag = IPC_NOWAIT;

	ret = msgrcv(q->qid, buf, buf_len, 0, flag);
	if(-1 == ret)
		ret = 0;

	if(q->hnotify_pipe)
		MyNamePipeRead(q->hnotify_pipe, &nt, sizeof(nt));

	ret = ret - sizeof(sndbuf->mtype);
	memmove(buf, (char *)buf + sizeof(sndbuf->mtype), ret);
	
	return ret;
}

/**
 * @brief 获取通知管道的fd
 */
int MySVMsgQGetSelectFd(HMYSVMSGQ hmq)
{
	mysvmq_t * q = (mysvmq_t *)hmq;
	if(NULL == q || NULL == q->hnotify_pipe)
		return INVALID_FD;

	return MyNamePipeGetFd(q->hnotify_pipe);
}

/**
 * @brief 获取通知管道的fd
 */
int MySVMsgQGetID(HMYSVMSGQ hmq)
{
	mysvmq_t * q = (mysvmq_t *)hmq;
	if(NULL == q)
		return -1;

	return q->qid;
}




























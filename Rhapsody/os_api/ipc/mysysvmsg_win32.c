/**
 * @file myipcmsgq_win32.c win��ģ��һ����Ϣ���� 2008-8-26 10:24
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  lin shao chuan makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 * see the GNU General Public License  for more detail.
 */
#include "mysysvmsg.h"

#include <stdio.h>
#include <string.h>

#include "mymmap.h"
#include "myevent.h"
#include "mysem.h"

#include "mylog.h"

#define MAX_Q_BUF_SIZE (1024 * 1024)


typedef struct __msgq_info_t_
{
	volatile int qsz;
	volatile int head_pos;
	volatile int tail_pos;
}msgq_info_t;

typedef struct __mysvmq_t_
{
	/*
	* ��Ϣ���еļ�ֵ
	*/
	unsigned long id;
	
	/*
	* �ùܵ���ģ����Ϣ����
	*/
	HUOS_MMAP hrealq;
	char * buf;
	msgq_info_t * qi;
	
	/*
	* ��Ϣ�����Ƿ��ɵ�ǰ���󴴽���
	*/
	int bcreate;	
	
	HMYMEMPOOL hm;

	/* д�¼� */
	HMYEVENT hevt;

	/* ������ */
	HMYSEM protector;
}mysvmq_t;


/**
 * @brief ������Ϣ����
 * @param key:��Ϣ���еĹؼ���
 * @param notify_pipe_name:�ܵ�֪ͨ������,Ϊnull�򲻴�֪ͨ�ܵ�
 * @param bcreate:��ʾ����
 * @param max_size:��ʾ��Ϣ���еĴ�С
 */
HMYSVMSGQ MySVMsgQConstruct(HMYMEMPOOL hm, 
							unsigned long key, 
							int bcreate, 
							size_t max_size, 
							const char * notify_pipe_name)
{
	char ackey[32] = {0};
	char ackey_m[32] = {0};
	char ackey_e[32] = {0};

	mysvmq_t * mq = MyMemPoolMalloc(hm, sizeof(*mq));
	if(NULL == mq)
		return NULL;

	mq->hm = hm;
	mq->buf = NULL;
	mq->qi = NULL;

	_snprintf(ackey, sizeof(ackey) - 1, "%d", key);
	_snprintf(ackey_m, sizeof(ackey_m) - 1, "%dm", key);
	_snprintf(ackey_e, sizeof(ackey_e) - 1, "%de", key);

	mq->protector = MySemRealConstruct(hm, 1, ackey_m);
	mq->hevt = MyEventRealConstruct(hm, 0, ackey_e);

	if(bcreate)
	{
		mq->hrealq = UOS_MMapOpen(hm, ackey, MAX_Q_BUF_SIZE, NULL);
		if(NULL == mq->hrealq)
		{
			mq->hrealq = UOS_MMapCreate(hm, ackey, MAX_Q_BUF_SIZE, NULL);

			mq->qi = UOS_MMapGetBuf(mq->hrealq);
			mq->qi->qsz = MAX_Q_BUF_SIZE - sizeof(*(mq->qi));
			mq->qi->head_pos = 0;
			mq->qi->tail_pos = 0;
		}
		else
		{
			mq->qi = UOS_MMapGetBuf(mq->hrealq);
		}
	}
	else
	{
		mq->hrealq = UOS_MMapOpen(hm, ackey, MAX_Q_BUF_SIZE, NULL);
		mq->qi = UOS_MMapGetBuf(mq->hrealq);
	}

	/* �ӹ����ڴ��л�ö��л�����ָ�� */
	mq->buf = (char *)(mq->qi + 1);

	mq->bcreate = bcreate;
	mq->id = key;

	return (HMYSVMSGQ)mq;
}

/**
 * @brief ��ȡ��Ϣ����
 * @param qid:��Ϣ���еı�ʶ
 * @param notify_pipe_name:�ܵ�֪ͨ������,Ϊnull�򲻴�֪ͨ�ܵ�
 */
HMYSVMSGQ MySVMsgQGet(HMYMEMPOOL hm, int qid, const char * notify_pipe_name)
{
	char ackey[32] = {0};
	char ackey_m[32] = {0};
	char ackey_e[32] = {0};

	mysvmq_t * mq = MyMemPoolMalloc(hm, sizeof(*mq));
	if(NULL == mq)
		return NULL;

	mq->bcreate = 0;
	mq->id = qid;
	mq->hm = hm;

	_snprintf(ackey, sizeof(ackey) - 1, "%d", mq->id);
	_snprintf(ackey_m, sizeof(ackey_m) - 1, "%dm", mq->id);
	_snprintf(ackey_e, sizeof(ackey_e) - 1, "%de", mq->id);

	mq->protector = MySemRealConstruct(hm, 1, ackey_m);
	mq->hevt = MyEventRealConstruct(hm, 0, ackey_e);
	mq->hrealq = UOS_MMapOpen(hm, ackey, MAX_Q_BUF_SIZE, NULL);

	/* �ӹ����ڴ��л�ö��л�����ָ�� */
	mq->qi = UOS_MMapGetBuf(mq->hrealq);
	mq->buf = (char *)(mq->qi + 1);

	return (HMYSVMSGQ)mq;
}

/**
 * @brief ������Ϣ����
 */
void MySVMsgQDestruct(HMYSVMSGQ hmq)
{
	mysvmq_t * mq = (mysvmq_t *)hmq;
	if(NULL == mq)
		return;

	if(mq->hrealq)
		UOS_MMapClose(mq->hrealq);

	MySemDestruct(mq->protector);
	MyEventDestruct(mq->hevt);

	MyMemPoolFree(mq->hm, mq);
}

/**
 * @brief д��Ϣ
 */
int MySVMsgQWrite(HMYSVMSGQ hmq, const void * buf, size_t buf_len)
{
	int ret = 0;

	mysvmq_t * mq = (mysvmq_t *)hmq;
	if(NULL == mq || NULL == mq->hrealq || 0 == buf_len || NULL == buf)
		return -1;

	MySemDeCrease(mq->protector);

	/* ����ʽ 4byte(��ͷ) + nbyte(����) */

	/* �����򻯴���,����ѭ��,�Ժ������Ż� todo... */
	assert(mq->qi->tail_pos >= mq->qi->head_pos);
	
	if((mq->qi->qsz - mq->qi->tail_pos) >= (int)(buf_len + sizeof(size_t)))
	{
		/* �ռ乻 */
		memcpy(&mq->buf[mq->qi->tail_pos], &buf_len, sizeof(size_t));
		memcpy(&mq->buf[mq->qi->tail_pos + sizeof(size_t)], buf, buf_len);

		/* �ı�βָ��λ�� */
		mq->qi->tail_pos = (int)(mq->qi->tail_pos + buf_len + sizeof(size_t));
	}
	else
	{
		/* �������ѭ����ͷ��,������ÿռ�,todo... */
		ret = -1;
	}

	MySemInCrease(mq->protector);

	MyEventSetSignaled(mq->hevt);

	return ret;
}

/**
 * @brief ����Ϣ
 */
size_t MySVMsgQRead(HMYSVMSGQ hmq, void * buf, size_t buf_len, int bblock)
{
	int ret = 0;
	size_t len = 0;

	mysvmq_t * mq = (mysvmq_t *)hmq;
	if(NULL == mq || NULL == mq->hrealq || 0 == buf_len || NULL == buf)
		return -1;
	/* ����ʽ 4byte(��ͷ) + nbyte(����) */

	/* ����4�ֽڰ�ͷ,�ٶ����� */
	MySemDeCrease(mq->protector);
	ret = mq->qi->tail_pos - mq->qi->head_pos;
	MySemInCrease(mq->protector);

	while(0 == ret)
	{
		MyEventWait(mq->hevt, NULL);

		MySemDeCrease(mq->protector);
		ret = mq->qi->tail_pos - mq->qi->head_pos;
		MySemInCrease(mq->protector);
	}

	MySemDeCrease(mq->protector);

	assert(mq->qi->tail_pos >= mq->qi->head_pos);

	memcpy(&len, &mq->buf[mq->qi->head_pos], sizeof(size_t));
	if(0 == len)
	{
		LOG_WARN(("read err"));
		mq->qi->head_pos += sizeof(size_t);
		ret = 0;
		goto MySVMsgQRead_end_;
	}

	if(len > buf_len)
	{
		/* �û�����Ļ����������� */
		memcpy(buf, &mq->buf[mq->qi->head_pos + sizeof(size_t)], buf_len);
		mq->qi->head_pos += (int)(len + sizeof(size_t));
		ret = (int)buf_len;
	}
	else
	{
		memcpy(buf, &mq->buf[mq->qi->head_pos + sizeof(size_t)], len);
		mq->qi->head_pos += (int)(len + sizeof(size_t));
		ret = (int)len;
	}

MySVMsgQRead_end_:

	assert(mq->qi->tail_pos >= mq->qi->head_pos);

	if(mq->qi->head_pos >= mq->qi->tail_pos)
		mq->qi->head_pos = mq->qi->tail_pos = 0;

	MySemInCrease(mq->protector);
	return ret;
}

/**
 * @brief ��ȡ֪ͨ�ܵ���fd
 */
int MySVMsgQGetSelectFd(HMYSVMSGQ hmq);

/**
 * @brief ��ȡ֪ͨ�ܵ���fd
 */
int MySVMsgQGetID(HMYSVMSGQ hmq)
{
	mysvmq_t * mq = (mysvmq_t *)hmq;
	if(NULL == mq || NULL == mq->hrealq)
		return -1;

	return mq->id;
}

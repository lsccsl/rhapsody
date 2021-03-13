/*
*
* ���ü�������̬�����Ļ����������̲߳���ȫ 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/

#ifndef __MYBUFFER_H__
#define __MYBUFFER_H__


#include "mymempool.h"


struct __mybuffer_t_;
typedef struct __mybuffer_t_* HMYBUFFER;


/*
*
*����
*
*/
extern HMYBUFFER MyBufferConstruct(HMYMEMPOOL hm, size_t s);

/*
*
*����
*
*/
extern void MyBufferDestruct(HMYBUFFER hb);

/*
*
*����
*
*/
extern void MyBufferRef(HMYBUFFER hb_src, HMYBUFFER hb_dst);

/*
*
*���
*
*/
extern void MyBufferDeepCopy(HMYBUFFER hb_src, HMYBUFFER hb_dst);

/*
*
*��ȡ������
*
*/
extern void * MyBufferGet(HMYBUFFER hb, size_t * len);

/*
*
*���û�����
*
*/
extern int MyBufferSet(HMYBUFFER hb, const void * b, size_t len);

/**
 * @brief ���������е������ÿ�,ʵ���ǽ����ݳ�������
 */
extern int MyBufferClear(HMYBUFFER hb);

/*
*
*ƴ�ӻ�����
*
*/
extern int MyBufferAppend(HMYBUFFER hb, const void * b, size_t len);

/*
*
*��������������ƴ��
*
*/
extern size_t MyBufferCat(HMYBUFFER hb_src, HMYBUFFER hb_dst);

/*
*
*��ȡ������������
*
*/
extern size_t MyBufferGetCapability(HMYBUFFER hb);

/*
*
*��ȡ�����������ü���
*
*/
extern int MyBufferGetRefCount(HMYBUFFER hb);

/*
*
*�鿴buffer
*
*/
extern void MyBufferLook(HMYBUFFER hb);

#endif














/*
*
* ���ü�������̬�����Ļ����������̲߳���ȫ 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/


#include "mybuffer.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "myutility.h"


typedef struct __ref_buffer_t_
{
	char * pb;         /*���������׵�ַ*/
	size_t capability; /*������������*/
	size_t len;        /*�����ĳ���*/
	int ref;           /*�����õĴ���*/
}ref_buffer_t;

typedef struct __mybuffer_t_
{
	ref_buffer_t * b;
	HMYMEMPOOL hm;
}mybuffer_t;


static __INLINE__ void decrease_ref(mybuffer_t * b)
{
	assert(b && b->b);

	b->b->ref -= 1;

	if(0 < b->b->ref)
	{
		b->b = NULL;
		return;
	}

	if(b->b->pb)
		MyMemPoolFree(b->hm, b->b->pb);

	MyMemPoolFree(b->hm, b->b);

	b->b = NULL;
}

static __INLINE__ void increase_ref(mybuffer_t * b)
{
	assert(b && b->b);

	b->b->ref += 1;
}

static __INLINE__ int resize(mybuffer_t * b, size_t len)
{
	void * temp = NULL;

	assert(b && b->b);

	if(len < b->b->capability)
		return 0;

	temp = MyMemPoolMalloc(b->hm, len);	
	if(NULL == temp)
		return -1;

	memcpy(temp, b->b->pb, b->b->len);

	MyMemPoolFree(b->hm, b->b->pb);

	b->b->capability = len;
	b->b->pb = (char *)temp;

	return 0;
}

static __INLINE__ size_t buffer_inter_append(mybuffer_t * b, const void * buf, size_t len)
{
	assert(b && b->b && buf && len);

	//bug here todo
	if(b->b->pb == buf)
		return 0;

	if((b->b->len + len) <= b->b->capability)
	{
		memcpy((char *)(b->b->pb) + b->b->len, buf, len);
		b->b->len += len;
		return b->b->len;
	}

	if(-1 == resize(b, (b->b->len + len) * 2))
		return b->b->len;

	memcpy((char *)(b->b->pb) + b->b->len, buf, len);
	b->b->len += len;

	return b->b->len;
}


/*
*
*����
*
*/
HMYBUFFER MyBufferConstruct(HMYMEMPOOL hm, size_t s)
{
	mybuffer_t * b = (mybuffer_t *)MyMemPoolMalloc(hm, sizeof(*b));
	if(NULL == b)
		return NULL;

	b->b = (ref_buffer_t *)MyMemPoolMalloc(hm, sizeof(*(b->b)));
	if(NULL == b->b)
		goto MyBufferConstruct_err_;

	b->hm = hm;

	b->b->len = 0;
	b->b->ref = 1;

	if(s)
		b->b->capability = s;
	else
		b->b->capability = 10;

	b->b->pb = (char *)MyMemPoolMalloc(hm, b->b->capability);

	if(NULL == b->b->pb)
		goto MyBufferConstruct_err_;

	return (HMYBUFFER)b;

MyBufferConstruct_err_:

	if(NULL == b)
		return NULL;

	if(b->b)
		decrease_ref(b);

	MyMemPoolFree(b->hm, b);

	return NULL;
}

/*
*
*����
*
*/
void MyBufferDestruct(HMYBUFFER b)
{
	if(NULL == b)
		return;

	if(b->b)
		decrease_ref(b);

	MyMemPoolFree(b->hm, b);
}

/*
*
*����
*
*/
void MyBufferRef(HMYBUFFER b_src, HMYBUFFER b_dst)
{
	if(NULL == b_src || NULL == b_dst || NULL == b_src->b)
		return;

	if(b_dst->b)
		decrease_ref(b_dst);

	increase_ref(b_src);

	b_dst->b = b_src->b;
}

/*
*
*���
*
*/
//void MyBufferDeepCopy(HMYBUFFER hb_src, HMYBUFFER hb_dst)
//{
//	mybuffer_t * b_src = (mybuffer_t *)hb_src;
//	mybuffer_t * b_dst = (mybuffer_t *)hb_dst;
//
//	if(NULL == b_src || NULL == b_dst || NULL == b_src->b || NULL == b_dst->b)
//		return;
//
//	decrease_ref(b_dst);
//
//	memcpy(b_dst->b, b_src->b, sizeof(*(b_dst->b)));
//
//	b_dst->b->pb = MyMemPoolMalloc(b_dst->hm, b_dst->b->capability);
//	memcpy(b_dst->b->pb, b_src->b->pb, b_dst->b->capability);
//}

/*
*
*��ȡ������
*
*/
void * MyBufferGet(HMYBUFFER b, size_t * len)
{
	if(NULL == b)
	{
		if(len)
			*len = 0;
		return NULL;
	}

	if(len)
		*len = b->b->len;

	return b->b->pb;
}

/*
*
*���û�����
*
*/
int MyBufferSet(HMYBUFFER b, const void * buf, size_t len)
{
	void * temp = NULL;
	if(NULL == b || NULL == b->b || NULL == buf || 0 == len)
		return -1;

	//�жϻ������Ƿ񹹴󣬲����������·��仺����
	if(len <= b->b->capability)
	{
		memcpy(b->b->pb, buf, len);
		b->b->len = len;
		return 0;
	}
		
	temp = MyMemPoolMalloc(b->hm, len * 2);	
	if(NULL == temp)
		return -1;

	memcpy(temp, buf, len);

	MyMemPoolFree(b->hm, b->b->pb);

	b->b->pb = (char *)temp;
	b->b->capability = len * 2;
	b->b->len = len;

	return 0;
}

/**
 * @brief ���������е������ÿ�,ʵ���ǽ����ݳ�������
 */
int MyBufferClear(HMYBUFFER hb)
{
	if(NULL == hb)
		return -1;

	hb->b->len = 0;
	return 0;
}

/*
*
*ƴ�ӻ�����
*
*/
int MyBufferAppend(HMYBUFFER b, const void * buf, size_t len)
{
	if(NULL == b || NULL == b->b || NULL == buf || 0 == len)
		return -1;

	buffer_inter_append(b, buf, len);

	return 0;
}

/*
*
*��������������ƴ��
*
*/
size_t MyBufferCat(HMYBUFFER b_src, HMYBUFFER b_dst)
{
	if(NULL == b_src || NULL == b_src->b || NULL == b_dst || 0 == b_dst->b || NULL == b_dst->b->pb || 0 == b_dst->b->len)
		return 0;

	return buffer_inter_append(b_src, b_dst->b->pb, b_dst->b->len);
}

/*
*
*ƴ�ӻ�����
*
*/
size_t MyBufferGetCapability(HMYBUFFER b)
{
	if(NULL == b || NULL == b->b)
		return 0;

	return b->b->capability;
}

/*
*
*��ȡ�����������ü���
*
*/
int MyBufferGetRefCount(HMYBUFFER b)
{
	if(NULL == b || NULL == b->b)
		return -1;

	return b->b->ref;
}

/*
*
*�鿴buffer
*
*/
void MyBufferLook(HMYBUFFER hb)
{
	size_t l = 0;
	size_t c = 0;
	int r = 0;
	void * b = NULL;
	b = MyBufferGet(hb, &l);
	c = MyBufferGetCapability(hb);
	r = MyBufferGetRefCount(hb);

#ifdef _MBCSV6
	printf("[%s:%d] %x:%s c:%d l:%d r:%d\n", __FILE__, __LINE__, b, (char *)b, c, l, r);
#else
#ifdef WIN32
	printf("[%s:%d] %x:%s c:%d l:%d r:%d\n", __FILE__, __LINE__, (long long)b, (char *)b, c, l, r);
#else
	printf("[%s:%d] %x:%s c:%d l:%d r:%d\n", __FILE__, __LINE__, b, (char *)b, c, l, r);
#endif
#endif
}











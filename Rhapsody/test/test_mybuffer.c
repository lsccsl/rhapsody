#ifdef __cplusplus
extern "C"
{
#endif
#include "mybuffer.h"
#ifdef __cplusplus
}
#endif

#include <stdio.h>
#include <string.h>

#define LOOK_BUF(hb) do{\
		size_t l = 0;\
		size_t c = 0;\
		int r = 0;\
		void * b = NULL;\
		b = MyBufferGet(hb, &l);\
		c = MyBufferGetCapability(hb);\
		r = MyBufferGetRefCount(hb);\
		printf("[%s:%d] %x:%s c:%d l:%d r:%d\n", __FILE__, __LINE__, b, b, c, l, r);\
}while(0)

void test_mybuffer()
{
	HMYBUFFER hb = NULL;
	HMYBUFFER hb1 = NULL;
	HMYBUFFER hb2 = NULL;

	hb = MyBufferConstruct(NULL, 0);
	hb1 = MyBufferConstruct(NULL, 0);
	hb2 = MyBufferConstruct(NULL, 0);

	MyBufferSet(hb, (void *)"strlen", strlen("strlen"));
	MyBufferSet(hb2, (void *)"hb2", strlen("hb2"));

	MyBufferGet(hb2, NULL);

	{
		size_t l = 0;
		size_t c = 0;
		int r = 0;
		void * b = NULL;
		b = MyBufferGet(hb, &l);
		c = MyBufferGetCapability(hb);
		r = MyBufferGetRefCount(hb);
		printf("[%s:%d] %x:%s c:%d l:%d r:%d\n", __FILE__, __LINE__, b, b, c, l, r);
	}

	MyBufferAppend(hb, (void *)"abcdedfgfd", strlen("abcdedfgfd"));	
	{
		size_t l = 0;
		size_t c = 0;
		int r = 0;
		char * b = NULL;
		b = (char *)MyBufferGet(hb, &l);
		c = MyBufferGetCapability(hb);
		r = MyBufferGetRefCount(hb);
		printf("[%s:%d] %x:%s%s c:%d l:%d r:%d\n", __FILE__, __LINE__, b, b, b + l, c, l, r);
	}

	MyBufferAppend(hb, (void *)"123", strlen("123"));
	MyBufferLook(hb);

	MyBufferCat(hb, hb2);
	MyBufferLook(hb);

	printf("test ref\n");
	{
		size_t l = 0;
		size_t c = 0;
		int r = 0;
		void * b = NULL;
		b = MyBufferGet(hb1, &l);
		c = MyBufferGetCapability(hb1);
		r = MyBufferGetRefCount(hb1);
		printf("[%s:%d] %x:%s c:%d l:%d r:%d\n", __FILE__, __LINE__, b, b, c, l, r);
	}

	MyBufferRef(hb, hb1);
	{
		size_t l = 0;
		size_t c = 0;
		int r = 0;
		void * b = NULL;
		b = MyBufferGet(hb1, &l);
		c = MyBufferGetCapability(hb1);
		r = MyBufferGetRefCount(hb1);
		printf("[%s:%d] %x:%s c:%d l:%d r:%d\n", __FILE__, __LINE__, b, b, c, l, r);
	}
	{
		size_t l = 0;
		size_t c = 0;
		int r = 0;
		void * b = NULL;
		b = MyBufferGet(hb, &l);
		c = MyBufferGetCapability(hb);
		r = MyBufferGetRefCount(hb);
		printf("[%s:%d] %x:%s c:%d l:%d r:%d\n", __FILE__, __LINE__, b, b, c, l, r);
	}

	MyBufferDestruct(hb);

	MyMemPoolMemReport(0);
	{
		size_t l = 0;
		size_t c = 0;
		int r = 0;
		void * b = NULL;
		b = MyBufferGet(hb1, &l);
		c = MyBufferGetCapability(hb1);
		r = MyBufferGetRefCount(hb1);
		printf("[%s:%d] %x:%s c:%d l:%d r:%d\n", __FILE__, __LINE__, b, b, c, l, r);
	}
	MyBufferDestruct(hb1);
	MyBufferDestruct(hb2);
	MyMemPoolMemReport(1);
}






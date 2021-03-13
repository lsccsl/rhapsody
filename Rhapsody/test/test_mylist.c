#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __cplusplus
extern "C"
{
#endif
#include "mylist.h"
#include "mylistex.h"
#ifdef __cplusplus
}
#endif

void test_list()
{
	int i = 0;
	HMYLIST hlist = MyListConstruct(NULL);
	HMYLIST_ITER iter;

	printf("is emptye:%d\n", MyListIsEmpty(hlist));

	for(i = 10; i < 20; i ++)
	{
		MyListAddTail(hlist,(void *)i);
	}

	for(iter = MyListGetHead(hlist); iter != MyListGetTail(hlist); iter = MyListGetNext(hlist, iter))
	{
		printf("%2d ", (int)MyListGetIterData(iter));
	}

	printf("\n");

	for(i = 9; i >= 0; i --)
	{
		MyListAddHead(hlist,(void *)i);
	}

	for(iter = MyListGetHead(hlist); iter != MyListGetTail(hlist); iter = MyListGetNext(hlist, iter))
	{
		printf("%2d ", (int)MyListGetIterData(iter));
	}

	printf("\n");

	iter = MyListGetHead(hlist);
	for(i = 0; i < 20; i ++)
	{
		printf("%2d ", (int)MyListGetIterData(iter));

		if(i % 2)
		{
			iter = MyListGetNext(hlist, iter);
			continue;
		}

		iter = MyListErase(hlist, iter);
	}

	printf("\n");

	for(iter = MyListGetHead(hlist); iter != MyListGetTail(hlist); iter = MyListGetNext(hlist, iter))
	{
		printf("%2d ", (int)MyListGetIterData(iter));
	}

	printf("\nhead:%d tail:%d\n",MyListPopHead(hlist), MyListPopTail(hlist));

	for(iter = MyListGetHead(hlist); iter != MyListGetTail(hlist); iter = MyListGetNext(hlist, iter))
	{
		printf("%2d ", (int)MyListGetIterData(iter));
	}

	printf("\nis emptye:%d\n", MyListIsEmpty(hlist));

	MyListEraseAll(hlist);

	MyMemPoolMemReport(0);

	printf("\nis emptye:%d\n", MyListIsEmpty(hlist));

	MyListDestruct(hlist);

#ifdef MEM_LEAK_CHECK
	MyMemPoolMemReport(1);
#endif
}


typedef struct __test_list_ex_data
{
	char data1[32];
	char * data;
	int index;
}test_list_ex_data;

/*
*
*描述对象如何被构造
*
*/
void test_list_ex_con(void * obj, size_t obj_size, void * param, size_t param_size)
{
	test_list_ex_data * d = (test_list_ex_data *)obj;
	//memset(obj, 0, obj_size);
	d->data = (char *)MyMemPoolMalloc(NULL, sizeof(d->data1));
	//memset(d->data, 0, 32);
}

/*
*
*描述对象如何被析构
*
*/
void test_list_ex_des(void * obj, size_t obj_size)
{
	test_list_ex_data * d = (test_list_ex_data *)obj;
	MyMemPoolFree(NULL, d->data);
}

/*
*
*描述对象如何被拷贝
*
*/
void test_list_ex_copy(void * dst, size_t dst_len, const void * src, size_t src_len)
{
	test_list_ex_data * d = (test_list_ex_data *)dst;
	test_list_ex_data * s = (test_list_ex_data *)src;

	strncpy(d->data1, s->data1, sizeof(d->data1) - 1);

	if(s->data)
		strncpy(d->data, s->data, 31);

	d->index = s->index;
}

static myobj_ops test_list_ex_ops = {
	test_list_ex_con,
	test_list_ex_des,
	test_list_ex_copy
};

void test_list_ex()
{
	HMYLIST_EX hl;
	hl = MyListExConstruct(NULL, &test_list_ex_ops);

	{
		int i = 0;
		for(i = 0; i < 10; i ++)
		{
			test_list_ex_data d = {"ADFASDFAS", "1212232"};
			MyListExAddHead(hl, &d, sizeof(d));
		}

		for(i = 0; i < 10; i ++)
		{
			test_list_ex_data d = {"ADFASDFAS", "1212232", i};
			MyListExAddTail(hl, &d, sizeof(d));
		}
	}

	MyListExDestruct(hl);

	MyMemPoolMemReport(1);
}
























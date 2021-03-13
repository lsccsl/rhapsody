#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#ifdef __cplusplus
extern "C"
{
#endif
#include "myvector.h"
#include "myheap.h"
#include "mylog.h"
#ifdef __cplusplus
}
#endif

#define LOOP 5000

void test_vector()
{
	HMYVECTOR hv = NULL;
	HMYVECTOR_ITER it = NULL;
	int i = 0;

	hv = MyVectorConstruct(NULL, 10, NULL, NULL);

	LOG_INFO(("add 0-9\n"));
	for(i = 0; i < 10; i ++)
	{
		MyVectorAdd(hv,(void*)i, 0);
	}
	printf("[%s:%d] vector size:%d vector count:%d\n", __FILE__, __LINE__, MyVectorGetSize(hv), MyVectorGetCount(hv));
	for(i = 0; i < MyVectorGetCount(hv); i ++)
	{
		int ret = (int)MyVectorGetIndexData(hv, i, NULL);
		printf("%d ", ret);
	}
	printf("\n\n");

	LOG_INFO(("add 10-19\n"));
	for(i = 0; i < 10; i ++)
	{
		MyVectorAdd(hv,(void*)(i + 10), 0);
	}
	LOG_INFO(("[%s:%d] vector size:%d vector count:%d\n", __FILE__, __LINE__, MyVectorGetSize(hv), MyVectorGetCount(hv)));
	for(i = 0; i < MyVectorGetCount(hv); i ++)
	{
		printf("%d ", MyVectorGetIndexData(hv, i, NULL));
	}
	printf("\n\n");


	LOG_INFO(("del \n"));
	for(i = 0; i < 10; i ++)
	{
		printf("%d ", MyVectorDel(hv, i));
	}
	LOG_INFO(("\n[%s:%d] vector size:%d vector count:%d\n", __FILE__, __LINE__, MyVectorGetSize(hv), MyVectorGetCount(hv)));
	for(i = 0; i < MyVectorGetCount(hv); i ++)
	{
		printf("%d ", MyVectorGetIndexData(hv, i, NULL));
	}
	printf("\n\n");

	MyMemPoolMemReport(0);

	MyVectorDestruct(hv);

	MyMemPoolMemReport(1);
}


typedef struct __test_vector_tag_
{
	int i;
	char * data;
}test_vector_tag;

/*
*
*描述对象如何被构造
*
*/
void test_vector_construct(void * obj, size_t obj_size, void * param, size_t param_size)
{
	test_vector_tag * tag = (test_vector_tag *)obj;
	tag->data = (char *)MyMemPoolMalloc(NULL, 12);
	memset(tag->data, 0, 12);
	strcpy(tag->data,"12");
}

/*
*
*描述对象如何被析构
*
*/
void test_vector_destruct(void * obj, size_t obj_size)
{
	test_vector_tag * tag = (test_vector_tag *)obj;
	MyMemPoolFree(NULL,tag->data);
}

/*
*
*描述如何拷贝key
*
*/
static void test_key_copy(void * dst, size_t dst_len, const void * src, size_t src_len)
{
	test_vector_tag * tag_src = (test_vector_tag *)src;
	test_vector_tag * tag_dst = (test_vector_tag *)dst;

	tag_dst->i = tag_src->i;
}

static myobj_ops test_vector_ops = {test_vector_construct, test_vector_destruct, test_key_copy};

void test_vector_ex()
{
	HMYVECTOR hv = NULL;
	HMYVECTOR_ITER it = NULL;
	int i = 0;

	LOG_INFO((__DATE__" "__TIME__"test_vector_ex\r\n"));
	hv = MyVectorConstruct(NULL, 10,&test_vector_ops, NULL);

	LOG_INFO(("add 0-9\n"));
	for(i = 0; i < 10; i ++)
	{
		test_vector_tag tag = {i};
		MyVectorAdd(hv, &tag, sizeof(tag));
	}
	LOG_INFO(("[%s:%d] vector size:%d vector count:%d\n", __FILE__, __LINE__, MyVectorGetSize(hv), MyVectorGetCount(hv)));
	for(i = 0; i < MyVectorGetCount(hv); i ++)
	{
		test_vector_tag * tag = (test_vector_tag *)MyVectorGetIndexData(hv, i, NULL);
		printf("%d ", tag->i);
	}
	printf("\n\n");

	LOG_INFO(("add 10-19\n"));
	for(i = 0; i < 10; i ++)
	{
		test_vector_tag tag = {i + 10};
		MyVectorAdd(hv, &tag, sizeof(tag));
	}
	LOG_INFO(("[%s:%d] vector size:%d vector count:%d\n", __FILE__, __LINE__, MyVectorGetSize(hv), MyVectorGetCount(hv)));
	for(i = 0; i < MyVectorGetCount(hv); i ++)
	{
		test_vector_tag * tag = (test_vector_tag *)MyVectorGetIndexData(hv, i, NULL);
		printf("%d ", tag->i);
	}
	printf("\n\n");


	LOG_INFO(("del \n"));
	for(i = 0; i < 10; i ++)
	{
		MyVectorDel(hv, i);
	}
	LOG_INFO(("\n[%s:%d] vector size:%d vector count:%d\n", __FILE__, __LINE__, MyVectorGetSize(hv), MyVectorGetCount(hv)));
	for(i = 0; i < MyVectorGetCount(hv); i ++)
	{
		test_vector_tag * tag = (test_vector_tag *)MyVectorGetIndexData(hv, i, NULL);
		printf("%d ", tag->i);
	}
	printf("\n\n");

	MyMemPoolMemReport(0);

	MyVectorDestruct(hv);

	MyMemPoolMemReport(1);
}


/*
*
*1表示 data1 > data2
*0表示 !(data1 > data2)
*
*/
int heap_compare(const void * data1, const void * data2, const void * context)
{
	return (int)data1 - (int)data2;
	/*int d1 = (int)data1;
	int d2 = (int)data2;
	if(d1 >= d2)
		return 1;
	return -1;*/
}

//测试堆算法
void test_vector_heap()
{
	int i = 0;
	HMYVECTOR hv = MyVectorConstruct(NULL, 0, NULL, heap_compare);

	LOG_INFO(("[%s:%d]begin to add\r\n", __FILE__, __LINE__));
	for(i = 0; i < LOOP; i ++)
	{
		MyVectorAdd(hv, (void *)(i), 0);
		printf(".");
	}
	LOG_INFO(("[%s:%d]add over%d\r\n", __FILE__, __LINE__, MyVectorGetCount(hv)));
	//MyVectorPrint(hv);

	LOG_INFO(("[%s:%d]begin make heap\r\n", __FILE__, __LINE__));
	MyVectorHeapMake(hv);
	//MyVectorPrint(hv);
	MyVectorHeapExamin(hv);

	LOG_INFO(("[%s:%d]begin push\r\n", __FILE__, __LINE__));
	for(i = 0; i < LOOP; i ++)
	{
		MyVectorHeapPush(hv, (void *)(rand()), 0);
		//MyVectorPrint(hv);
		MyVectorHeapExamin(hv);
		printf(".");
	}
	MyVectorHeapExamin(hv);
	//MyVectorPrint(hv);

	/*for(i = 0; i < 10; i ++)
	{
		MyVectorHeapPop(hv);
		MyVectorPrint(hv);
	}*/

	//printf("[%s:%d]开始make heap\r\n", __FILE__, __LINE__);
	//MyVectorHeapMake(hv);
	//MyVectorPrint(hv);
	//MyVectorHeapExamin(hv);

	LOG_INFO(("[%s:%d]begin sort heap\r\n", __FILE__, __LINE__));
	MyVectorHeapSort(hv);
	MyVectorPrint(hv);
	MyVectorHeapExaminSortOK(hv);

	MyVectorDestruct(hv);

	MyMemPoolMemReport(1);
}



/*
*
*1表示 data1 > data2
*0表示 !(data1 > data2)
*
*/
static int test_heap_compare(const void * data1, const void * data2)
{
	test_vector_tag * t1 = (test_vector_tag *)data1;
	test_vector_tag * t2 = (test_vector_tag *)data2;

	if(t1->i > t2->i)
		return 1;

	return 0;
}

void test_heap_sort()
{
	int i = 0;
	HMYHEAP hp = MyHeapConstruct(NULL, 0, NULL, heap_compare);
	HMYVECTOR hv = MyVectorConstruct(NULL, 0, NULL, NULL);
	for(i = 0; i < 100; i ++)
	{
		HMYHEAP_KEY key = MyHeapPush(hp, (void *)i, 0);
		MyVectorAdd(hv, key, 0);
		MyHeapExamin(hp);
		printf(".");
	}
	LOG_INFO(("push 100 record over"));
	MyHeapPrint(hp);

	for(i = 0; i < 100; i ++)
	{
		HMYHEAP_KEY key  = (HMYHEAP_KEY)MyVectorGetIndexData(hv, i, NULL);
		MyHeapUpdate(hp, key, (void *)(i + 100), 0);
		MyHeapExamin(hp);
		printf(".");
	}
	LOG_INFO(("push 100 record over"));
	MyHeapPrint(hp);

	LOG_INFO(("sort begin"));

	MyHeapSort(hp);
	MyHeapPrint(hp);
	MyHeapExaminSortOK(hp);
	MyHeapClear(hp);
	MyVectorClear(hv);

	for(i = 0; i < 200; i ++)
	{
		HMYHEAP_KEY key = MyHeapPush(hp, (void *)(i + 300), 0);
		MyVectorAdd(hv, key, 0);
		MyHeapExamin(hp);
		printf(".");
	}

	LOG_INFO(("add 200 over"));

	for(i = 0; i < 100; i ++)
	{
		HMYHEAP_KEY key  = (HMYHEAP_KEY)MyVectorGetIndexData(hv, i, NULL);
		MyHeapDel(hp, key);
		MyHeapExamin(hp);
		printf(".");
	}

	LOG_INFO(("del 200 over"));

	MyHeapSort(hp);
	MyHeapExaminSortOK(hp);
	MyHeapPrint(hp);

	LOG_INFO(("sort over"));

	MyVectorDestruct(hv);
	MyHeapClear(hp);
	MyMemPoolMemReport(0);

	MyHeapDestruct(hp);

	MyMemPoolMemReport(1);
}

void test_heap_real()
{
	int i = 0;
	HMYHEAP hp = MyHeapConstruct(NULL, 0, NULL, heap_compare);
	HMYVECTOR hv = MyVectorConstruct(NULL, 0, NULL, NULL);

	LOG_DEBUG(("begin add"));
	LOG_WARN(("begin add"));
	LOG_ERR(("begin add"));
	LOG_INFO(("begin add"));
	for(i = 0; i < LOOP; i ++)
	{
		HMYHEAP_KEY key = MyHeapPush(hp, (void *)(rand()), 0);
		assert(key);
		MyVectorAdd(hv, key, 0);
		MyHeapExamin(hp);
		printf(".");
	}

	LOG_INFO(("begin update"));
	for(i = 0; i < MyVectorGetCount(hv); i ++)
	{
		HMYHEAP_KEY key  = (HMYHEAP_KEY)MyVectorGetIndexData(hv, i, NULL);
		key = MyHeapUpdate(hp, key, (void *)(rand()), 0);
		assert(key);
		MyHeapExamin(hp);
		printf(".");
	}

	LOG_INFO(("begin pop"));
	for(i = 0; i < LOOP/2; i ++)
	{
		MyHeapPop(hp);
		MyHeapExamin(hp);
		printf(".");
	}


	LOG_INFO(("begin del"));
	for(i = 0; i < MyVectorGetCount(hv); i ++)
	{
		HMYHEAP_KEY key  = (HMYHEAP_KEY)MyVectorGetIndexData(hv, i, NULL);
		MyHeapDel(hp, key);
		MyHeapExamin(hp);
		printf(".");
	}

	MyVectorDestruct(hv);
	MyMemPoolMemReport(0);

	MyHeapDestruct(hp);

	MyMemPoolMemReport(1);
}

void test_heap()
{
	test_heap_sort();
	test_heap_real();
}

























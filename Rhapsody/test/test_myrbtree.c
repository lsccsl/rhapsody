#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
extern void gettimeofday(struct timeval *ptv, void *tzp);
#else
#include <sys/time.h>
#endif
#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include "mymap.h"
#include "myrbtree.h"
#include "mylist.h"
#ifdef __cplusplus
}
#endif

extern void getcallid(char * callid, size_t callid_len);

#define LOOP_COUNT 10000
#define CALLID_LEN 32

enum __rbtree_colour_
{
	rbtree_colour_black,
	rbtree_colour_red,
};

struct __myrbtree_node_t_
{
	struct __myrbtree_node_t_ * left;
	struct __myrbtree_node_t_ * right;
	struct __myrbtree_node_t_ * parent;

	enum __rbtree_colour_ colour;

	char * key;
	char * data;
};

struct __myrbtree_t_
{
	struct __myrbtree_node_t_ * root;

	//内存池
	HMYMEMPOOL hm;

	//比较运算符
	myrbtree_compare compare;
};

int myrbtree_compare1(const void * key1, const void * key2)
{
	return (int)key1 > (int)key2;
}

static void printftree(struct __myrbtree_node_t_ * node)
{
	HMYLIST hlist = MyListConstruct(NULL);
	int son = 0;

	MyListAddTail(hlist, node);

	while(!MyListIsEmpty(hlist))
	{
		node = (struct __myrbtree_node_t_ *)MyListPopHead(hlist);

		if(NULL == node)
			continue;

		printf("%d-%d-%x-%x  ", node->data, node->colour, node, node->parent);

		son --;
		if(son <= 0)
			printf("\n\n\n");

		if(node->left)
		{
			MyListAddTail(hlist, node->left);
			//son ++;
		}

		if(node->right)
		{
			MyListAddTail(hlist, node->right);
			//son ++;
		}
		if(son <= 0)
			son = MyListGetCount(hlist);
	}

	MyListDestruct(hlist);
}

int myrbtree_compare_string(const void * key1, const void * key2)
{
	const char * actemp = (const char * )key1;
	const char * actemp1 = (const char * )key2;

	if(strcmp(actemp, actemp1) > 0)
		return 1;

	return 0;
}

//void getcallid(char * callid, size_t callid_len)
//{
//	int i = 0;
//	for(i=0; i<callid_len - 1; i++)
//	{		
//		char ucRandomNum = (char)(rand()%36);
//		if(ucRandomNum<=9)
//			callid[i] = '0'+ucRandomNum;
//		else
//			callid[i] = 'a'+ucRandomNum-10;
//	}
//
//	//strncpy(callid, "aaaaaaaaaaaaaaa", callid_len/2);
//
//	callid[callid_len - 1] = 0;
//}


typedef struct __test_map_tag_
{
	char rand_string[CALLID_LEN];

#ifdef _DEBUG
	char * data;
#endif
}test_map_tag;

static int map_compare_string(const void * key1, const void * key2)
{
	//const test_map_tag * actemp = key1;
	//const test_map_tag * actemp1 = key2;

	if(strcmp(((test_map_tag*)key1)->rand_string, ((test_map_tag*)key2)->rand_string) > 0)
		return 1;

	return 0;
}

#ifdef _DEBUG

/*
*
*描述如何构造key
*
*/
static void test_key_construct(void * key, size_t key_size, void * param, size_t param_size)
{
	test_map_tag * tag = (test_map_tag *)key;

	tag->data = (char *)MyMemPoolMalloc(NULL, sizeof(tag->rand_string));
}

/*
*
*描述如何析构key
*
*/
static void test_key_destruct(void * key, size_t key_size)
{
	test_map_tag * tag = (test_map_tag *)key;

	MyMemPoolFree(NULL, tag->data);
}

/*
*
*描述如何构造data
*
*/
static void test_data_construct(void * data, size_t data_size, void * param, size_t param_size)
{
	test_map_tag * tag = (test_map_tag *)data;

	tag->data = (char *)MyMemPoolMalloc(NULL, sizeof(tag->rand_string));
}

/*
*
*描述如何析构data
*
*/
static void test_data_destruct(void * data, size_t data_size)
{
	test_map_tag * tag = (test_map_tag *)data;

	MyMemPoolFree(NULL, tag->data);
}

/*
*
*描述如何拷贝key
*
*/
static void test_key_copy(void * dst, size_t dst_len, const void * src, size_t src_len)
{
	test_map_tag * tag_src = (test_map_tag *)src;
	test_map_tag * tag_dst = (test_map_tag *)dst;

	strncpy(tag_dst->rand_string, tag_src->rand_string, sizeof(tag_dst->rand_string));
	strncpy(tag_dst->data, tag_dst->rand_string, sizeof(tag_dst->rand_string));
}

/*
*
*描述如何拷贝data
*
*/
static void test_data_copy(void * dst, size_t dst_len, const void * src, size_t src_len)
{
	test_map_tag * tag_src = (test_map_tag *)src;
	test_map_tag * tag_dst = (test_map_tag *)dst;

	strncpy(tag_dst->rand_string, tag_src->rand_string, sizeof(tag_dst->rand_string));
	strncpy(tag_dst->data, tag_dst->rand_string, sizeof(tag_dst->rand_string));
}
#endif
static myobj_ops key_ops = {
#ifndef _DEBUG
	0
#else
	test_key_construct,
	test_key_destruct,
	test_key_copy,
#endif
};

static myobj_ops data_ops = {
#ifndef _DEBUG
	0
#else
	test_data_construct,
	test_data_destruct,
	test_data_copy,
#endif
};

void test_map()
{

	struct timeval tv1;
	struct timeval tv2;
	HMYMAP hmap = MyMapRealConstruct(NULL, map_compare_string, 
		&key_ops,
		&data_ops
		/*test_key_construct, test_key_destruct,
		test_data_construct, test_data_destruct,
		test_key_copy, test_data_copy*/);

	int i = 0;
	int count = 0;
	int temp_test = 0;
	struct __myrbtree_t * temp = (struct __myrbtree_t * )hmap;

	unsigned long rand_seed = time(0);
	{
		FILE * pfile = fopen("rand_seed.txt", "aw");
		if(pfile)
		{
			fprintf(pfile, "rand seed:%d\n", rand_seed);
			fclose(pfile);
		}
	}
	srand(rand_seed);
	temp_test = rand();
	srand(rand_seed);
	temp_test = rand();

	srand(0/*rand_seed*/);
	gettimeofday(&tv1, 0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
#ifndef _DEBUG
		test_map_tag temp_tag = {0};

		getcallid(temp_tag.rand_string, sizeof(temp_tag.rand_string) - 1);

		MyMapInsertUnique(hmap, &temp_tag, sizeof(temp_tag), &temp_tag, 20);
#else
		int flag = 0;
		test_map_tag temp_tag = {0};

		getcallid(temp_tag.rand_string, sizeof(temp_tag.rand_string)-1);

		if(NULL == MyMapSearch(hmap, &temp_tag))
		{
			flag = 1;
		}
		else
		{
			assert(0);
			flag = 0;
		}

		MyMapInsertUnique(hmap, &temp_tag, sizeof(temp_tag), &temp_tag, sizeof(temp_tag));

		if(flag)
		{
			int a = MyMapExamin(hmap);
			count ++;
			printf(".");
		}
		else
		{
			printf(" ");
		}
#endif
	}
	gettimeofday(&tv2, 0);
	printf("mymap添加用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);

	printf("开始搜索随机记录\n");
	printf("\n开始mymap开始查找1000k记录\n");
	srand(0/*rand_seed*/);
	gettimeofday(&tv1, 0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
#ifndef _DEBUG
		test_map_tag temp_tag = {0};

		getcallid(temp_tag.rand_string, sizeof(temp_tag.rand_string)-1);

		MyMapSearch(hmap, &temp_tag);
#else
		HMYMAP_ITER it = NULL;
		test_map_tag temp_tag = {0};

		getcallid(temp_tag.rand_string, sizeof(temp_tag.rand_string)-1);

		it = MyMapSearch(hmap, &temp_tag);
		assert(it);
#endif
	}
	gettimeofday(&tv2, 0);
	printf("mymap查找用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);

	printf("max path:%d min path:%d black count:%d count:%d-%d\n",
		MyMapLayer(hmap, 1), MyMapLayer(hmap, 0), MyMapExamin(hmap), count, MyMapGetRealCount(hmap));

	printf("开始删除随机记录\n");
	srand(0/*rand_seed*/);
	count = 0;
	gettimeofday(&tv1, 0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
#ifndef _DEBUG
		test_map_tag temp_tag = {0};

		getcallid(temp_tag.rand_string, sizeof(temp_tag.rand_string)-1);

		MyMapDelKey(hmap, &temp_tag);
#else
		HMYMAP_ITER it = NULL;
		int flag = 0;
		test_map_tag temp_tag = {0};

		getcallid(temp_tag.rand_string, sizeof(temp_tag.rand_string)-1);

		it = MyMapSearch(hmap, &temp_tag);
		assert(it);
		if(NULL == it)
		{
			int aasdfsdf = 0;
		}
		else
		{
			assert(strcmp(temp_tag.rand_string, ((test_map_tag*)MyMapGetIterData(it, NULL))->rand_string) == 0);
			count ++;
			flag = 1;
		}

		MyMapDelKey(hmap, &temp_tag);

		if(flag)
		{
			int a = 0;
			a = MyMapExamin(hmap);
			printf(".");
		}
		else
		{
			printf(" ");
		}
#endif
	}
	gettimeofday(&tv2, 0);
	printf("mymap删除用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);
	printf("black count:%d - %5d - %5d %d\n", MyMapExamin(hmap), i, MyMapGetRealCount(hmap), count);
	MyMemPoolMemReport(0);

	MyMapDestruct(hmap);
	MyMemPoolMemReport(1);
}

void test_rbtree1()
{

	HMYRBTREE htree = MyRBTreeConstruct(NULL, myrbtree_compare_string);
	int i = 0;
	int count = 0;
	int temp_test = 0;
	struct __myrbtree_t_ * temp = (struct __myrbtree_t_ * )htree;

	unsigned long rand_seed = time(0);
	{
		FILE * pfile = fopen("rand_seed.txt", "aw");
		if(pfile)
		{
			fprintf(pfile, "rand seed:%d\n", rand_seed);
			fclose(pfile);
		}
	}
	srand(rand_seed);
	temp_test = rand();
	srand(rand_seed);
	temp_test = rand();

	srand(rand_seed);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		int flag = 0;
		char * actemp =	NULL;
		char * actemp1 = NULL;

		actemp = (char *)MyMemPoolMalloc(NULL, CALLID_LEN);
		actemp1 = (char *)MyMemPoolMalloc(NULL, CALLID_LEN);
		getcallid(actemp, CALLID_LEN);
		strncpy(actemp1, actemp, CALLID_LEN);

		if(NULL == MyRBTreeSearch(htree, actemp))
		{
			flag = 1;
		}
		else
		{
			flag = 0;
		}

		MyRBTreeInsertUnique(htree, actemp, actemp1);

		if(flag)
		{
			int a = MyRBTreeExamin(htree);
			count ++;
			printf(".");
		}
		else
		{
			printf(" ");
		}
	}

	printf("max path:%d min path:%d black count:%d count:%d-%d\n",
		MyRBTreeLayer(htree, 1), MyRBTreeLayer(htree, 0), MyRBTreeExamin(htree), count, MyRBTreeGetRealCount(htree));

	//{
	//	scanf("\n");
	//}

	printf("开始删除随机记录\n");
	srand(rand_seed);
	count = 0;
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		HMYRBTREE_ITER it = NULL;
		void * data = NULL;
		void * key = NULL;
		int flag = 0;
		char actemp[CALLID_LEN] = {0};
		

		getcallid(actemp, sizeof(actemp));

		it = MyRBTreeSearch(htree, actemp);
		if(NULL == it)
		{
			int aasdfsdf = 0;
		}
		else
		{
			assert(strcmp(actemp, (char *)MyRBTreeGetIterData(it)) == 0);
			count ++;
			flag = 1;
		}

		MyRBTreeDelKey(htree, actemp, &key, &data);

		if(flag)
		{
			int a = 0;
			assert(strcmp(actemp, (char *)key) == 0);
			a = MyRBTreeExamin(htree);
			printf(".");
		}
		else
		{
			printf(" ");
		}

		if(key)
			MyMemPoolFree(NULL, key);
		if(data)
			MyMemPoolFree(NULL, data);
	}
	printf("black count:%d - %5d - %5d %d\n", MyRBTreeExamin(htree), i, MyRBTreeGetRealCount(htree), count);
	MyMemPoolMemReport(0);

	MyRBTreeDestruct(htree);
	MyMemPoolMemReport(1);

	//{
	//	char sdfasdf;
	//	scanf("%c\n", &sdfasdf);
	//}
}

#ifndef MEM_LEAK_CHECK


void test_rbtree()
{
	HMYRBTREE htree = MyRBTreeConstruct(NULL, myrbtree_compare1);
	HMYRBTREE_ITER it = NULL;
	struct timeval tv1;
	struct timeval tv2;
	int i = 0;

	printf("\n开始添加1000k记录\n");
	gettimeofday(&tv1, 0);
	for(i = 0; i < 1000000; i ++)
	{
		MyRBTreeInsertUnique(htree, (void *)i, (void *)i);
	}
	gettimeofday(&tv2, 0);
	printf("添加用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);

	printf("添加1000k记录完毕[%d],开始查询\n", MyRBTreeGetRealCount(htree));
	gettimeofday(&tv1, 0);
	for(i = 0; i < 1000000; i ++)
	{
		it = MyRBTreeSearch(htree, (void *)i);
	}
	gettimeofday(&tv2, 0);

	printf("查询用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);

	printf("开始删除1000k记录\n");
	gettimeofday(&tv1, 0);
	for(i = 0; i < 1000000; i ++)
	{
		MyRBTreeDelKey(htree, (void *)i, NULL, NULL);
	}
	gettimeofday(&tv2, 0);
	printf("删除1000k记录完毕[%d]\n", MyRBTreeGetRealCount(htree));
	printf("删除用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);
}

#else

void test_rbtree()
{
	HMYRBTREE htree = MyRBTreeConstruct(NULL, myrbtree_compare1);
	struct __myrbtree_t_ * temp = (struct __myrbtree_t_ * )htree;
	HMYRBTREE_ITER iter = NULL;
	int i = 0;
	int count = 0;
	int temp_test = 0;

	unsigned long rand_seed = time(0);
	{
		FILE * pfile = fopen("rand_seed.txt", "aw");
		if(pfile)
		{
			fprintf(pfile, "rand seed:%d\n", rand_seed);
			fclose(pfile);
		}
	}
	srand(rand_seed);
	temp_test = rand();
	srand(rand_seed);
	temp_test = rand();

	MyRBTreeInsertUnique(htree, (void *)14, (void *)14);
	MyRBTreeInsertUnique(htree, (void *)8, (void *)8);
	MyRBTreeInsertUnique(htree, (void *)3, (void *)3);
	MyRBTreeInsertUnique(htree, (void *)2, (void *)2);
	MyRBTreeInsertUnique(htree, (void *)5, (void *)5);
	MyRBTreeInsertUnique(htree, (void *)10, (void *)10);
	MyRBTreeInsertUnique(htree, (void *)6, (void *)6);
	MyRBTreeInsertUnique(htree, (void *)13, (void *)13);
	MyRBTreeInsertUnique(htree, (void *)4, (void *)4);
	MyRBTreeInsertUnique(htree, (void *)1, (void *)1);
	MyRBTreeInsertUnique(htree, (void *)9, (void *)9);
	MyRBTreeInsertUnique(htree, (void *)7, (void *)7);
	MyRBTreeInsertUnique(htree, (void *)11, (void *)11);
	MyRBTreeInsertUnique(htree, (void *)12, (void *)12);
	MyRBTreeInsertUnique(htree, (void *)15, (void *)15);
	MyRBTreeInsertUnique(htree, (void *)15, (void *)12345);
	printf("black count:%d\n", MyRBTreeExamin(htree));
	printf("max path:%d min path:%d record count:%d\n", MyRBTreeLayer(htree, 1), MyRBTreeLayer(htree, 0), MyRBTreeGetRealCount(htree));

	{
		HMYRBTREE_ITER it_temp = it_temp = MyRBTreeBegin(htree);
		for(; it_temp != NULL/*MyRBTreeEnd(htree)*/; it_temp = MyRBTreeGetNext(it_temp))
		{
			int temp_look = (int)MyRBTreeGetIterData(it_temp);
			printf("%d ", temp_look);
		}
		printf("\n");
	}
	{
		HMYRBTREE_ITER it_temp = it_temp = MyRBTreeEnd(htree);
		for(; it_temp != NULL/*MyRBTreeEnd(htree)*/; it_temp = MyRBTreeGetPrev(it_temp))
		{
			int temp_look = (int)MyRBTreeGetIterData(it_temp);
			printf("%d ", temp_look);
		}
		printf("\n");
	}
	MyRBTreeDestruct(htree);
	return ;

	for(i = 0; i < 16; i ++)
	{
		MyRBTreeDelKey(htree, (void *)i, NULL, NULL);
		MyRBTreeExamin(htree);
	}

	printftree(/*(struct __myrbtree_node_t_ * )*/(temp->root));

	printf("开始添加随机记录\n");
	srand(rand_seed);
	for(i = 0; i < 400000; i ++)
	{
		unsigned int temp = rand();
		int flag = 0;

		if(NULL == MyRBTreeSearch(htree, (void *)(temp + 16)))
		{
			flag = 1;
		}
		else
		{
			flag = 0;
		}

		MyRBTreeInsertUnique(htree, (void *)(temp + 16), (void *)(temp + 16));

		if(flag)
		{
			MyRBTreeExamin(htree);
			count ++;
			printf(".");
		}
		else
		{
			printf(" ");
		}
	}

	printf("max path:%d min path:%d black count:%d count:%d-%d\n",
		MyRBTreeLayer(htree, 1), MyRBTreeLayer(htree, 0), MyRBTreeExamin(htree), count, MyRBTreeGetRealCount(htree));

#define TEMP_TEST 912	

	iter = MyRBTreeSearch(htree, (void *)TEMP_TEST);
	printf("search TEMP_TEST is %d\n", MyRBTreeGetIterData(iter));

	if(NULL == iter)
	{
		MyRBTreeInsertEqual(htree, (void *)TEMP_TEST, (void *)TEMP_TEST);
		iter = MyRBTreeSearch(htree, (void *)TEMP_TEST);
		printf("search TEMP_TEST is %d\n", MyRBTreeGetIterData(iter));
	}

	printf("black count:%d\n", MyRBTreeExamin(htree));

	{
		void * key;
		void * data;
		MyRBTreeDelKey(htree, (void *)TEMP_TEST, &key, &data);
		printf("del %d:%d\n", key, data);
	}

	printf("black count:%d\n", MyRBTreeExamin(htree));

	printf("开始删除随机记录\n");
	srand(rand_seed);
	count = 0;
	for(i = 0; i < 400000; i ++)
	{
		unsigned int temp = rand();
		int flag = 0;

		if(NULL == MyRBTreeSearch(htree, (void *)(temp + 16)))
		{
			int aasdfsdf = 0;
		}
		else
		{
			count ++;
			flag = 1;
		}

		MyRBTreeDelKey(htree, (void *)(temp + 16), NULL, NULL);

		if(flag)
		{
			MyRBTreeExamin(htree);
			printf(".");
		}
		else
		{
			printf(" ");
		}
	}
	printf("black count:%d - %5d - %5d %d\n", MyRBTreeExamin(htree), i, MyRBTreeGetRealCount(htree), count);

#ifdef MEM_LEAK_CHECK
	MyMemPoolMemReport(1);
#endif

	iter = MyRBTreeSearch(htree, (void *)TEMP_TEST);
	printf("search TEMP_TEST is %d\n", MyRBTreeGetIterData(iter));

	printf("max path:%d min path:%d record count:%d\n", MyRBTreeLayer(htree, 1), MyRBTreeLayer(htree, 0), count);

	temp = (struct __myrbtree_t_ * )htree;

	for(i = 0; i < 16; i ++)
	{
		MyRBTreeDelKey(htree, (void *)i, NULL, NULL);
	}

#ifdef MEM_LEAK_CHECK
	MyMemPoolMemReport(1);
#endif

	MyRBTreeClear(htree);
	MyRBTreeDestruct(htree);

#ifdef MEM_LEAK_CHECK
	MyMemPoolMemReport(1);
#endif
}

#endif




















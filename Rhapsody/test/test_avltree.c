#include "myAVLTree.h"
#include "mylog.h"
#include <assert.h>
#include <time.h>
#include <stdio.h>

#define LOOP 10000

typedef struct __myavltree_node_t1_
{
	struct __myavltree_node_t1_ * parent;
	struct __myavltree_node_t1_ * left;
	struct __myavltree_node_t1_ * right;

	/*
	* 记录关键字信息
	*/
	void * key;
	size_t height;
	void * userdata;
}myavltree_node_t1;

typedef struct __myavltree_t1_
{
	HMYMEMPOOL hm;
	myavltree_node_t1 * root;
	ALG_COMPARE compare;
}myavltree_t1;


/**
 * @brief 
 * 1 表示 key1 比 key2 大
 * 0 表示 key1 比 key2 小 
 */
static int test_bbstree_compare(const void * key1, const void * key2, const void * context)
{
	return (int)key1 - (int)key2;
}

static void test_avltree_int()
{
	int i = 0;
	myavltree_t1 * temp;
	time_t t = time(0);

	HMYAVL_TREE havl_tree = MyAVLTreeConstruct(NULL, test_bbstree_compare);
	temp = (myavltree_t1 *)havl_tree;

	{
		FILE * pf = fopen("rand_seed.txt", "aw");
		if(pf)
		{
			fprintf(pf, "avl seed:%d\r\n", t);
			fclose(pf);
		}
	}

	LOG_DEBUG(("avl tree开始添加"));
	srand(t);
	for(i = 0; i < LOOP; i ++)
	{
		if(MyAVLTreeInsert(havl_tree, (void *)rand(), (void *)i) != 0)
		{
			printf(" ");
			continue;
		}

		MyAVLTreeExamin(havl_tree);
		printf(".");
	}

	MyAVLTreeExamin(havl_tree);

	LOG_DEBUG(("max path:%d min path:%d 节点个数:%d", MyAVLTreeMaxPath(havl_tree), MyAVLTreeMinPath(havl_tree), MyAVLTreeGetCount(havl_tree)));

	LOG_DEBUG(("avl tree开始删除"));
	srand(t);
	for(i = 0; i < LOOP; i ++)
	{
		int ret = MyAVLTreeDel(havl_tree, (void *)rand(), NULL, NULL);

		//if(0 == i % 1000)
		if(0 == ret)
		{
			MyAVLTreeExamin(havl_tree);
			printf(".");
		}
		else
		{
			printf(" ");
		}
	}

	MyMemPoolMemReport(0);

	for(i = 0; i < 100; i ++)
	{
		MyAVLTreeInsert(havl_tree, (void *)i, (void *)i);
	}

	MyAVLTreeDestruct(havl_tree);

	MyMemPoolMemReport(1);
}


static int test_bbstree_compare_string(const void * key1, const void * key2, const void * context)
{
	int ret = 0;
	char * k1 = (char *)key1;
	char * k2 = (char *)key2;

	assert(key1 && key2);

	ret = strcmp(key1, key2);

	return ret;
}

static void test_avltree_string()
{
	int i = 0;
	myavltree_t1 * temp;
	time_t t = time(0);

	HMYAVL_TREE havl_tree = MyAVLTreeConstruct(NULL, test_bbstree_compare_string);
	temp = (myavltree_t1 *)havl_tree;

	{
		FILE * pf = fopen("rand_seed.txt", "aw");
		if(pf)
		{
			fprintf(pf, "avl seed:%d\r\n", t);
			fclose(pf);
		}
	}

	LOG_DEBUG(("avl tree开始添加字符串"));

	srand(t);
	for(i = 0; i < LOOP; i ++)
	{
		char * key = MyMemPoolMalloc(NULL, 36);
		getcallid(key, 32);
		if(MyAVLTreeInsert(havl_tree, key, (void *)i))
		{
			printf(" ");
			assert(0);
			continue;
		}

		MyAVLTreeExamin(havl_tree);
		printf(".");
	}

	MyAVLTreeExamin(havl_tree);

	LOG_DEBUG(("max path:%d min path:%d", MyAVLTreeMaxPath(havl_tree), MyAVLTreeMinPath(havl_tree)));

	LOG_DEBUG(("avl tree开始删除字符串"));
	LOG_DEBUG(("avl tree node:%d", MyAVLTreeGetCount(havl_tree)));
	srand(t);
	for(i = 0; i < LOOP; i ++)
	{
		int ret = 0;
		char key[36] = {0};
		void * key_ret = NULL;
		getcallid(key, 32);
		ret = MyAVLTreeDel(havl_tree, key, &key_ret, NULL);
		assert(0 == ret);
		assert(key_ret);
		MyMemPoolFree(NULL, key_ret);
		MyAVLTreeExamin(havl_tree);
		printf(".");
	}

	MyMemPoolMemReport(0);

	MyAVLTreeDestruct(havl_tree);

	MyMemPoolMemReport(1);
}

void test_avltree()
{
	test_avltree_string();
	test_avltree_int();
}






#include "myBTree.h"
#include "mylog.h"

#include <assert.h>
#include <stdio.h>

#define LOOP 10000

static int btree_compare(const void * data1, const void * data2, const void * context)
{
	int d1 = (int)data1;
	int d2 = (int)data2;
	if(d1 > d2)
		return 1;
	else if(d1 < d2)
		return -1;
	else
		return 0;
}

static int btree_compare_string(const void * data1, const void * data2, const void * context)
{
	char * d1 = (char *)data1;
	char * d2 = (char *)data2;

	return strncmp(d1, d2, strlen(d1) > strlen(d2) ? strlen(d1) : strlen(d2));
}

static void test_btree_string()
{
	int i = 0;
	size_t count = 0;

	HMYBTREE hbtree = MyBTreeConstruct(NULL, btree_compare_string, 3);
	unsigned long rand_seed = time(0);
	{
		FILE * pfile = fopen("rand_seed.txt", "aw");
		if(pfile)
		{
			fprintf(pfile, "B tree rand seed:%d\n", rand_seed);
			fclose(pfile);
		}
	}

	LOG_INFO(("test btree add string"));
	srand(rand_seed);
	for(i = 0; i < LOOP; i ++)
	{
		char * temp_string = MyMemPoolMalloc(NULL, 32);
		getcallid(temp_string, 31);
		temp_string[31] = 0;

		if(0 == MyBTreeAdd(hbtree, temp_string))
		{
			MyBTreeExamin(hbtree, 0);
			count ++;
			assert(count == MyBTreeCalCount(hbtree));
			assert(MyBTreeGetCount(hbtree) == MyBTreeCalCount(hbtree));
			{
				char * pctemp = NULL;
				char actemp[32] = {0};
				int ret = 0;
				strncpy(actemp, temp_string, sizeof(actemp) - 1);
				ret = MyBTreeSearch(hbtree, (void *)actemp, (void **)&pctemp);

				assert(0 == ret);
				assert(0 == strncmp(pctemp, actemp, sizeof(actemp) - 1));
			}
			printf(".");
		}
		else
			printf(" ");
	}

	LOG_INFO(("test btree search string"));
	srand(rand_seed);
	for(i = 0; i < LOOP; i ++)
	{
		char * pctemp = NULL;
		char actemp[32] = {0};
		int ret = 0;
		getcallid(actemp, 31);
		ret = MyBTreeSearch(hbtree, (void *)actemp, (void **)&pctemp);

		assert(0 == ret);
		assert(0 == strncmp(pctemp, actemp, sizeof(actemp)));
		printf(".");
	}

	LOG_INFO(("test btree del string"));
	srand(rand_seed);
	for(i = 0; i < LOOP; i ++)
	{
		char * pctemp = NULL;
		char actemp[32] = {0};
		getcallid(actemp, 31);

		//printf("%d %d", i, data_del);
		if(0 == MyBTreeDel(hbtree, (void *)actemp, (void **)&pctemp))
		{
			count --;
			assert(0 == strncmp(actemp, pctemp, sizeof(actemp) - 1));
			assert(count == MyBTreeCalCount(hbtree));
			assert(MyBTreeGetCount(hbtree) == MyBTreeCalCount(hbtree));
			MyBTreeExamin(hbtree, 0);

			assert(0 != MyBTreeSearch(hbtree, (void *)actemp, NULL));
			MyMemPoolFree(NULL, pctemp);
			printf(".");
		}
		else
			printf(" ");
	}

	LOG_INFO(("btree count %d ", MyBTreeCalCount(hbtree)));
	LOG_INFO(("del %d", count));

	MyMemPoolMemReport(0);

	MyBTreeDestruct(hbtree);

	MyMemPoolMemReport(1);
}

static void test_btree_int()
{
	int i = 0;
	size_t count = 0;

	HMYBTREE hbtree = MyBTreeConstruct(NULL, btree_compare, 5);

	LOG_INFO(("test btree add"));
	srand(0);
	for(i = 0; i < LOOP; i ++)
	{
		int data = rand();
		//printf("%d %d", i, data);
		if(0 == MyBTreeAdd(hbtree, (void *)data))
		{
			count ++;
			if(0 == i % 1000)
			{
				assert(count == MyBTreeCalCount(hbtree));
				assert(MyBTreeGetCount(hbtree) == MyBTreeCalCount(hbtree));
				MyBTreeExamin(hbtree, 0);
			}
			{
				int data_ret = 0;
				int ret = MyBTreeSearch(hbtree, (void *)data, (void **)&data_ret);

				assert(0 == ret);
				assert(data == data_ret);
			}
			printf(".");
		}
		else
		{
			printf(" ");
		}
	}

	LOG_INFO(("btree count %d layer:%d", MyBTreeCalCount(hbtree), MyBTreeExamin(hbtree, 0)));
	assert(MyBTreeCalCount(hbtree) == count);


	LOG_INFO(("test btree search"));
	srand(0);
	for(i = 0; i < LOOP; i ++)
	{
		int data = rand();
		int data_ret = 0;
		int ret = MyBTreeSearch(hbtree, (void *)data, (void **)&data_ret);

		assert(0 == ret);
		assert(data == data_ret);

		printf(".");
	}

	LOG_INFO(("test btree del"));
	srand(0);
	for(i = 0; i < LOOP/2; i ++)
	{
		void * data = NULL;
		int data_del = rand();

		//printf("%d %d", i, data_del);
		if(0 == MyBTreeDel(hbtree, (void *)data_del, &data))
		{
			count --;
			//if(i >= 399)
			//	MyBTreeExamin(hbtree, 1);
			//else
			if(0 == i % 1000)
			{
				assert(data_del == (int)data);
				assert(count == MyBTreeCalCount(hbtree));
				assert(MyBTreeGetCount(hbtree) == MyBTreeCalCount(hbtree));
				MyBTreeExamin(hbtree, 0);
			}
			assert(0 != MyBTreeSearch(hbtree, (void *)data_del, NULL));
			printf(".");
		}
		else
			printf(" ");
	}

	LOG_INFO(("btree count %d ", MyBTreeCalCount(hbtree)));
	LOG_INFO(("del %d", count));

	MyMemPoolMemReport(0);

	MyBTreeDestruct(hbtree);

	MyMemPoolMemReport(1);
}

void test_btree()
{
	test_btree_string();
	test_btree_int();
}











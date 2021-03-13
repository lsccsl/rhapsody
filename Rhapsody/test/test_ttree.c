#include "myTTree.h"
#include <assert.h>
#include <stdio.h>
#include "mylog.h"


#define TEST_LOOP 10000


int test_ttree_compare(const void * data1, const void * data2, const void * context)
{
	unsigned int d1 = (unsigned int)data1;
	unsigned int d2 = (unsigned int)data2;

	if(d1 > d2)
		return 1;
	if(d1 == d2)
		return 0;
	else
		return -1;
}

static void test_ttree_int()
{
	int i = 0;

	HMYTTREE httree = MyTTreeConstruct(NULL, test_ttree_compare, 5, 10);

	srand(0);
	for(i = 0; i < TEST_LOOP * 2; i ++)
	{
		unsigned int temp = rand();
		if(0 == MyTTreeAdd(httree, (void *)temp))
		{
			MyTTreeExamin(httree, 0);
			printf(".");
		}
		else
			printf(" ");
	}

	MyTTreeExamin(httree, 0);
	printf("\r\n");
	LOG_INFO(("T tree has node:%d", MyTTreeGetCount(httree)));


	srand(0);
	for(i = 0; i < TEST_LOOP; i ++)
	{
		unsigned int temp = rand();
		unsigned int data = 0;
		int ret = MyTTreeSearch(httree, (void *)temp, (void **)&data);

		assert(ret == 0 && data == temp);
	}

	srand(0);
	for(i = 0; i < TEST_LOOP; i ++)
	{
		unsigned int temp = rand();
		unsigned int data = 0;
		int ret1 = -1;
		int ret = -1;

		ret1 = MyTTreeSearch(httree, (void *)temp, (void **)&data);
		ret = MyTTreeDel(httree, (void *)temp, (void **)&data);

		if(0 == ret1)
		{
			assert(ret == 0 && data == temp);
			MyTTreeExamin(httree, 0);
			printf(".");
		}
		else
			printf(" ");
	}

	MyTTreeExamin(httree, 0);
	printf("\r\n");
	LOG_INFO(("T tree has node:%d", MyTTreeGetCount(httree)));

	MyMemPoolMemReport(0);
	MyTTreeDestruct(httree);
	MyMemPoolMemReport(1);
}

static int test_ttree_string_compare(const void * d1, const void * d2, const void * context)
{
	char * s1 = (char *)d1;
	char * s2 = (char *)d2;

	return strncmp(s1, s2, strlen(s1));
}

static void test_ttree_string()
{
	int i = 0;

	HMYTTREE httree = MyTTreeConstruct(NULL, test_ttree_string_compare, 5, 10);
	unsigned long rand_seed = time(0);
	{
		FILE * pfile = fopen("rand_seed.txt", "aw");
		if(pfile)
		{
			fprintf(pfile, "T tree rand seed:%d\n", rand_seed);
			fclose(pfile);
		}
	}

	LOG_INFO(("test T tree string begin"));

	srand(rand_seed);
	for(i = 0; i < TEST_LOOP; i ++)
	{
		char * temp_string = MyMemPoolMalloc(NULL, 32);
		getcallid(temp_string, 31);
		temp_string[31] = 0;
		if(0 == MyTTreeAdd(httree, (void *)temp_string))
		{
			MyTTreeExamin(httree, 0);
			printf(".");
		}
		else
			printf(" ");
	}

	MyTTreeExamin(httree, 0);
	printf("\r\n");
	LOG_INFO(("test T tree string, has node:%d", MyTTreeGetCount(httree)));
	LOG_INFO(("begin search"));

	srand(rand_seed);
	for(i = 0; i < TEST_LOOP; i ++)
	{
		int ret = -1;
		char * data = 0;

		char temp_string[32] = {0};
		getcallid(temp_string, sizeof(temp_string) - 1);

		ret = MyTTreeSearch(httree, (void *)temp_string, (void **)&data);

		assert(ret == 0 && data);
		assert(strncmp(data, temp_string, sizeof(temp_string) - 1) == 0);
	}

	LOG_INFO(("end search"));

	LOG_INFO(("T tree string begin del"));

	srand(rand_seed);
	for(i = 0; i < TEST_LOOP; i ++)
	{
		int ret = -1;
		char * data = 0;

		char temp_string[32] = {0};
		getcallid(temp_string, sizeof(temp_string) - 1);

		ret = MyTTreeDel(httree, (void *)temp_string, (void **)&data);
		assert(ret == 0 && data);
		assert(strncmp(data, temp_string, sizeof(temp_string) - 1) == 0);
		MyMemPoolFree(NULL, data);

		MyTTreeExamin(httree, 0);

		printf(".");
	}

	MyTTreeExamin(httree, 0);
	printf("\r\n");
	LOG_INFO(("end del, test T tree string, has node:%d", MyTTreeGetCount(httree)));
	assert(MyTTreeGetCount(httree) == 0);

	MyMemPoolMemReport(0);
	MyTTreeDestruct(httree);
	MyMemPoolMemReport(1);
}

void test_ttree()
{
	test_ttree_string();
	test_ttree_int();
}





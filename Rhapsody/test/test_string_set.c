#include <assert.h>
#ifdef __cplusplus
extern "C"
{
#endif
#include "string_set.h"
#include "MyStringSet.h"
#include "MyStringSetEx.h"
#include "mylog.h"
#ifdef __cplusplus
}
#endif
#ifdef WIN32
	#include <winsock2.h>
#else
	#include <time.h>
	#include <string.h>
	#include <stdio.h>
	#include <sys/time.h>
#endif
extern void getcallid(char * callid, size_t callid_len);



#define LOOP_COUNT 300

void test_mystring_set()
{
#ifdef WIN32
	struct timeval tv1;
	struct timeval tv2;
	int i = 0;
	HMYSTRING_SET hss = MyStringSetConstruct(NULL);
	LOG_INFO(("test_mystring_set begin add string set\r\n"));
	srand(0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		int ret = -1;
		char callid[32] = {0};
		getcallid(callid, sizeof(callid) - 1);

		ret = MyStringSetAdd(hss, callid, callid + strlen(callid) - 1, (void *)(i+1), 0);
		assert(0 == ret);
		printf(".");
		//printf("%4d - %s\n", i, callid);
	}

	LOG_INFO(("begin search string set\r\n"));
	srand(0);
	LOG_INFO(("\nbegin search stringset %d record\r\n", LOOP_COUNT));
	gettimeofday(&tv1, 0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		int ret = -1;
		char callid[32] = {0};
		void * data = NULL;
		getcallid(callid, sizeof(callid) - 1);

		//if(0 == i % 10)
		//	printf("\n");

		ret = MyStringSetSearch(hss, callid, callid + sizeof(callid) - 3, &data, NULL);
		assert(0 == ret);
		//assert(data == i + 1);

		//printf("%4d - %s\n", data, callid);
		//printf(".");
	}
	gettimeofday(&tv2, 0);
	LOG_INFO(("mystringset search user:%f sec\r\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0));
#endif
}

void test_string_set()
{
	struct timeval tv1;
	struct timeval tv2;

	int i = 0;
	HSTRING_SET hss = StringSetConstruct(NULL, NULL);

	LOG_INFO(("begin add string set \r\n"));
	srand(0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		int ret = -1;
		char callid[32] = {0};
		getcallid(callid, sizeof(callid) - 1);

		ret = StringSetAdd(hss, callid, callid + strlen(callid), (unsigned long)(i+1), 0);
		assert(0 == ret);
		printf(".");
	}
	printf("\n");

	srand(0);
	LOG_INFO(("begine search stringset %d \n", LOOP_COUNT));
	gettimeofday(&tv1, 0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		int ret = -1;
		char callid[32] = {0};
		unsigned long data = 0;
		getcallid(callid, sizeof(callid) - 1);

		//if(0 == i % 10)
		//	printf("\n");

		ret = StringSetSearch(hss, callid, callid + sizeof(callid) - 1, &data, NULL);
		assert(0 == ret);
		assert(data == i + 1);

		//printf("%4d - %s\n", data, callid);
	}
	gettimeofday(&tv2, 0);
	LOG_INFO(("string set search use:%f sec \r\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0));

	{
		int ret = 0;
		char callid[32] = "abcd";
		ret = StringSetSearch(hss, callid, callid + strlen(callid), NULL, NULL);
		assert(ret != 0);
	}

	LOG_INFO(("begin del\r\n"));
	srand(0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		int ret = -1;
		char callid[32] = {0};
		unsigned long data = 0;
		getcallid(callid, sizeof(callid) - 1);

		ret = StringSetDel(hss, callid, callid + strlen(callid), &data, NULL);
		assert(0 == ret);
		assert(data == i + 1);

		//???
		ret = StringSetDel(hss, callid, callid + strlen(callid), &data, NULL);
		assert(-1 == ret);

		printf(".");
	}

	MyMemPoolMemReport(0);

	LOG_INFO(("begin add string set\r\n"));
	srand(0);
	for(i = 0; i < 100; i ++)
	{
		int ret = -1;
		char callid[32] = {0};
		getcallid(callid, sizeof(callid) - 1);

		ret = StringSetAdd(hss, callid, callid + strlen(callid), (unsigned long)(i+1), 0);
		assert(0 == ret);
		printf(".");
	}
	printf("\n");

	MyMemPoolMemReport(0);
	StringSetDestruct(hss);
	MyMemPoolMemReport(1);

	printf("\n");
}

void test_my_string_set_ex()
{
#ifdef WIN32
	struct timeval tv1;
	struct timeval tv2;
	int i = 0;
	HMYSTRING_SET_EX hss = MyStringSetExConstruct(NULL);
	printf("test_mystring_set_ex开始添加字符串集合\n");
	srand(0);
	gettimeofday(&tv1, 0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		char callid[32] = {0};
		getcallid(callid, sizeof(callid) - 1);

		MyStringSetExAdd(hss, callid, (void *)(i+1));
#ifdef _DEBUG
		{
			int ret = -1;
			ret = MyStringSetExAdd(hss, callid, (void *)(i+1));
			assert(0 == ret);
			printf(".");
			printf("%4d - %s\n", i, callid);
		}
#endif
	}
	gettimeofday(&tv2, 0);
	printf("mystringset_EX 添加用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);

	printf("开始查找字符串集合\n");
	srand(0);
	printf("\n开始stringset开始查找1000k记录\n");
	gettimeofday(&tv1, 0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		char callid[32] = {0};
		getcallid(callid, sizeof(callid) - 1);

		//if(0 == i % 10)
		//	printf("\n");

		MyStringSetExSearch(hss, callid, NULL);

#ifdef _DEBUG
		{
			int ret = -1;
			void * data = NULL;
			ret = MyStringSetExSearch(hss, callid, &data);
			assert(0 == ret);
			assert(data == (void *)(i + 1));

			printf("%4d - %s\n", data, callid);
			printf(".");
		}
#endif
	}
	gettimeofday(&tv2, 0);
	printf("mystringset_Ex查找用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);
#endif
}




















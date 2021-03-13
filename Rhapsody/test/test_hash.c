#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include "myhashmap.h"
#ifdef __cplusplus
}
#endif

#define LOOP 10000
#define CALLID_LEN 32
extern void getcallid(char * callid, size_t callid_len);

typedef struct __test_map_tag_
{
	char rand_string[CALLID_LEN];
	char * data;
}test_map_tag;


static size_t string_fun(const void * key)
{
	unsigned long h = 0;
	char * p = ((test_map_tag *)key)->rand_string;
	for(; *p; p++)
		h = 5 * h + *p;

	return h;
}

static int string_equal_fun(const void * key1, const void * key2)
{
	const test_map_tag * actemp = (const test_map_tag *)key1;
	const test_map_tag * actemp1 = (const test_map_tag *)key2;

	if(strcmp(actemp->rand_string, actemp1->rand_string) == 0)
		return 1;

	return 0;
}


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


size_t hash_fun(const void * key)
{
	return (size_t)key;
}

int equal_fun(const void * key1, const void * key2)
{
	if(key1 == key2)
		return 1;
	return 0;
}


void test_hashmap()
{
	int i = 0;

	myobj_ops key_ops1 = {
		test_key_construct,
		test_key_destruct,
		test_key_copy,
	};

	myobj_ops data_ops1 = {
		test_data_construct,
		test_data_destruct,
		test_data_copy,
	};
	HMYHASHMAP hhm = MyHashMapConstruct(NULL, string_fun, string_equal_fun, 0, 
		&key_ops1,
		&data_ops1);
	
	srand(0);
	for(i = 0; i < LOOP; i ++)
	{
		test_map_tag temp_tag = {0};

		getcallid(temp_tag.rand_string, sizeof(temp_tag.rand_string));

		MyHashMapInsertUnique(hhm, &temp_tag, sizeof(temp_tag), &temp_tag, sizeof(temp_tag));
	}
	MyHashMapPrint(hhm);

	srand(0);
	for(i = 0; i < LOOP; i ++)
	{
		HMYHASHMAP_ITER it = NULL;
		test_map_tag temp_tag = {0};

		getcallid(temp_tag.rand_string, sizeof(temp_tag.rand_string));

		it = MyHashMapSearch(hhm, &temp_tag);
		assert(it);

		printf(".");

		//if(0 == (i % 10))
		//	printf("\n");

		//printf("%d ", MyHashTableGetIterData(it));
	}
	printf("\n");

	{
		HMYHASHMAP_ITER it = MyHashMapBegin(hhm);
		for(; it != NULL; it = MyHashMapGetNext(hhm, it))
		{
			test_map_tag * temp_tag = (test_map_tag *)MyHashMapGetIterData(it);
			test_map_tag  * key = (test_map_tag *)MyHashMapGetIterData(it);
			assert(it);
			printf("%s-%s\n", temp_tag->rand_string, temp_tag->data);
			printf("%s-%s\n", key->rand_string, key->data);
		}
	}

	srand(0);
	for(i = 0; i < LOOP; i ++)
	{
		test_map_tag temp_tag = {0};

		getcallid(temp_tag.rand_string, sizeof(temp_tag.rand_string));

		MyHashMapDelKey(hhm, &temp_tag);

		assert(MyHashMapGetElementCount(hhm) == (LOOP - i - 1));
	}

	MyMemPoolMemReport(0);

	MyHashMapDestruct(hhm);

	MyMemPoolMemReport(1);
}

void test_hash()
{
	int i = 0;
	HMYHASHTABLE hht = MyHashTableConstruct(NULL, hash_fun, equal_fun, 0);
	
	srand(0);
	for(i = 0; i < LOOP; i ++)
	{
		int element = rand();
		MyHashTableInsertUnique(hht, (void *)element , (void *)i);
		printf("loop:%d\n", i);
	}
	MyHashTablePrint(hht);

	srand(0);
	for(i = 0; i < LOOP; i ++)
	{
		int element = rand();
		HMYHASHTABLE_ITER it = MyHashTableSearch(hht, (void *)element );
		assert(it);

		if(0 == (i % 10))
			printf("\n");

		printf("%d ", MyHashTableGetIterData(it));
	}

	srand(0);
	for(i = 0; i < LOOP; i ++)
	{
		void * key = NULL;
		void * data = NULL;

		int element = rand();
		MyHashTableDelKey(hht, (void *)element , &key, &data);
	}
	MyHashTablePrint(hht);

	MyMemPoolMemReport(0);

	MyHashTableDestruct(hht);

	MyMemPoolMemReport(1);
}










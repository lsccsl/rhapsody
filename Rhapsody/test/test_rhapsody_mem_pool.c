#include "mydefmempool.h"
#include "myrbtree.h"
#include "mylog.h"
#include "myhashtable.h"
#include <assert.h>

#define LOOP_COUNT 10000

/*
*
*1 表示 key1 比 key2 大
*0 表示 key1 比 key2 小 
*
*/
static int rbtree_compare(const void * key1, const void * key2)
{
	return key1 > key2;
}

static void test_rhapsody_rbtree()
{
	int i = 0;
	int malloc_count = 0;
	rhapsody_info_t rhapsody_info = {0};

	HMYMEMPOOL hm = RhapsodyMemPoolConstruct();

	HMYRBTREE hrbtree = MyRBTreeConstruct(hm, rbtree_compare);

	for(i = 0; i < LOOP_COUNT; i ++)
	{
		MyRBTreeInsertUnique(hrbtree, (void *)i, (void *)i);
	}
	
	malloc_count = MyMemPoolGetBlkCount();
	
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		MyRBTreeDelKey(hrbtree, (void *)i, NULL, NULL);
	}

	LOG_INFO(("after del rbtree rhapsody view"));
	MyMemPoolView(hm, &rhapsody_info, sizeof(rhapsody_info));

#ifdef MEM_LEAK_CHECK
	assert(malloc_count - 3 == rhapsody_info.blk_count);
#endif
	
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		MyRBTreeInsertUnique(hrbtree, (void *)i, (void *)i);
	}
	malloc_count = MyMemPoolGetBlkCount();

	LOG_INFO(("after insert rbtree again rhapsody view"));
	MyMemPoolView(hm, &rhapsody_info, sizeof(rhapsody_info));

	assert(rhapsody_info.blk_count == 0);

	MyRBTreeDestruct(hrbtree);
	
	LOG_INFO(("after destroy rbtree rhapsody view"));
	MyMemPoolView(hm, &rhapsody_info, sizeof(rhapsody_info));

#ifdef MEM_LEAK_CHECK
	assert(malloc_count - 2 == rhapsody_info.blk_count);
#endif
	
	MyMemePoolDestruct(hm);
	
	MyMemPoolMemReport(1);
}




static size_t hash_fun(const void * key)
{
	return (size_t)key;
}

static int equal_fun(const void * key1, const void * key2)
{
	return key1 == key2;
}
static void test_rhapsody_hash()
{
	int i = 0;
	int malloc_count = 0;
	rhapsody_info_t rhapsody_info = {0};

	HMYMEMPOOL hm = RhapsodyMemPoolConstruct();

	HMYHASHTABLE hht = MyHashTableConstruct(hm, hash_fun, equal_fun, 0);

	for(i = 0; i < LOOP_COUNT; i ++)
	{
		MyHashTableInsertUnique(hht, (void *)i, (void *)i);
		printf(".");
	}
	malloc_count = MyMemPoolGetBlkCount();
	
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		MyHashTableDelKey(hht, (void *)i, NULL, NULL);
		printf(".");
	}
	MyMemPoolMemReport(0);
	MyMemPoolView(hm, &rhapsody_info, sizeof(rhapsody_info));

#ifdef MEM_LEAK_CHECK
	assert(malloc_count - 3 == rhapsody_info.blk_count);
#endif

	for(i = 0; i < LOOP_COUNT; i ++)
	{
		MyHashTableInsertUnique(hht, (void *)i, (void *)i);
		printf(".");
	}
	malloc_count = MyMemPoolGetBlkCount();
	MyMemPoolView(hm, &rhapsody_info, sizeof(rhapsody_info));
	assert(1 == rhapsody_info.blk_count);
	
	MyHashTableDestruct(hht);
	MyMemPoolView(hm, &rhapsody_info, sizeof(rhapsody_info));

#ifdef MEM_LEAK_CHECK
	assert(malloc_count - 2 == rhapsody_info.blk_count);
#endif

	MyMemePoolDestruct(hm);
	
	MyMemPoolMemReport(1);
}

static void test_rhapsody_heap()
{	
}

void test_rhapsody_mem_pool()
{
	LOG_INFO(("test rbtree wor with rhapsody mempool"));
	test_rhapsody_rbtree();

	LOG_INFO(("test hashtable wor with rhapsody mempool"));
	test_rhapsody_hash();
}



























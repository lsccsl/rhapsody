#include "btree.h"
#include "OsFile.h"
#include "mylog.h"
#include <stdio.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

#define TEST_LOOP_1 1000 /*100000 通过改变这个宏,改变测试循环次数,笔者已经在linux下面完成了10万次循环的详细测试*/
#define TEST_FILE_DB "./test.db"
#define TEST_FILE_DB_JOURNAL "./test.db-journal"
#define PAGE_SZ 1024
#define TEST_TABLE "test_table"

static int xcompare(const void * k1, const unsigned int k1_sz, 
					const void * k2, const unsigned int k2_sz, 
					const void * context, const unsigned int context_sz)
{	
	//return strncmp((char *)k1 + 4, (char *)k2 + 4, strlen((char *)k1 + 4) > strlen((char *)k2 + 4) ? strlen((char *)k1 + 4) : strlen((char *)k2 + 4));
	return strncmp((char *)k1 + 4, (char *)k2 + 4, (k1_sz - 4) > (k2_sz - 4) ? (k1_sz - 4) : (k2_sz - 4));
}

static int xcompare_master(const void * k1, const unsigned int k1_sz, 
					const void * k2, const unsigned int k2_sz, 
					const void * context, const unsigned int context_sz)
{
	return strncmp((char *)k1 + 4, (char *)k2 + 4, (k1_sz - 4) > (k2_sz - 4) ? (k1_sz - 4) : (k2_sz - 4));
}


char temp_string[8192] = {0,0,0,0, 'a', 'b', 'c', 'd'};


static void test_add(HBTREE_MGR hbtree_mgr, HBTREE_CURSOR hcur,
					 int loop_count, unsigned int ini_node_count,
					 int need_commint, int need_chk)
{
	int i;
	unsigned int count = 0;
	for(i = 0; i < loop_count; i ++)
	{
		int ret = 0;
		unsigned int node_count = 0;

		unsigned int key_sz = 0;
		key_sz = rand() % 1024;
		if(key_sz <= 16)
			key_sz = 16;
		getcallid(temp_string + 8, key_sz);
		key_sz += 8;
		memcpy(temp_string, &key_sz, sizeof(key_sz));

		ret = btreeMgrAdd(hcur,
			temp_string, key_sz,
			temp_string, key_sz);

		assert(0 == ret);

		if(0 == i % 100)
		{
			if(need_commint)
				btreeMgrCommit(hbtree_mgr);
		}

		if(need_chk)
		{
			count = btreeMgrGetCount(hcur);
			assert((i + 1) == count);
			ExaminBtree(hcur, &node_count, 1);

			{
				pager_info_t pgr_info;
				btreeMgrGetPagerInfo(hbtree_mgr, &pgr_info);

				/* 防止"外存泄漏" */
				assert(pgr_info.total_page_count - pgr_info.free_page_count == (node_count + ini_node_count));
			}
			assert(2 == btreeMgrGetPagerRefCount(hbtree_mgr));
		}

		printf(".");
	}
}

static void test_del(HBTREE_MGR hbtree_mgr, HBTREE_CURSOR hcur, int loop_count, int ini_total_count, int ini_node_count)
{
	unsigned int count = 0;
	int i;
	for(i = 0; i < loop_count; i ++)
	{
		int ret;
		unsigned int key_sz = 0;
		unsigned int node_count = 0;

		key_sz = rand() % 1024;
		if(key_sz <= 16)
			key_sz = 16;
		getcallid(temp_string + 8, key_sz);

		ret = btreeMgrDel(hcur, temp_string, key_sz + 8); 

		assert(0 == ret);

		if(0 == i % 100)
		{
			//btreeMgrCommit(hbtree_mgr);
		}

		{
			count = btreeMgrGetCount(hcur);
			assert((ini_total_count - (i + 1)) == count);
			ExaminBtree(hcur, &node_count, 1);

			{
				pager_info_t pgr_info;
				btreeMgrGetPagerInfo(hbtree_mgr, &pgr_info);

				assert(pgr_info.total_page_count - pgr_info.free_page_count == node_count + ini_node_count);
			}
		}

		assert(2 == btreeMgrGetPagerRefCount(hbtree_mgr));
		printf(".");
	}
}

void test_open_or_create_tbl()
{
	int i;
	HBTREE_MGR hbtree_mgr = NULL;
	HBTREE_CURSOR hcur_master = NULL;
	HBTREE_CURSOR hcur = NULL;
	HMYBUFFER hb = MyBufferConstruct(NULL, 0);

	hbtree_mgr = btreeMgrConstruct(NULL, TEST_FILE_DB, PAGE_SZ,
		100, 512, NULL, 0);

	assert(hbtree_mgr);

	btreeMgrOpenMaster(hbtree_mgr, &hcur_master, xcompare_master, NULL, 0);

	LOG_DEBUG(("create or open tables"));
	for(i = 0; i < 100; i ++)
	{
		unsigned int key_sz = 8;
		snprintf(temp_string + 8, sizeof(temp_string) - 9, "test%d", i);
		key_sz += strlen(temp_string + 8) + 1;
		memcpy(temp_string, &key_sz, sizeof(key_sz));

		if(0 != btreeMgrOpenTable(hcur_master, &hcur, xcompare, NULL, 0, temp_string, key_sz, hb))
		{
			int ret = btreeMgrCreateTable(hcur_master, &hcur, xcompare, NULL, 0, temp_string, key_sz, temp_string, key_sz - 4, e_btree_page_flag_hasdata);
			assert(0 == ret);
			printf("!");

			btreeMgrCommit(hbtree_mgr);
			{
				unsigned int count = 0;
				unsigned int node_count = 0;
				count = btreeMgrGetCount(hcur_master);
				assert((i + 1) == count);
				ExaminBtree(hcur_master, &node_count, 1);

				{
					pager_info_t pgr_info;
					btreeMgrGetPagerInfo(hbtree_mgr, &pgr_info);

					/* 防止"外存泄漏" */
					assert(pgr_info.total_page_count - pgr_info.free_page_count == node_count + 1 + (i + 1));
				}
				assert(2 == btreeMgrGetPagerRefCount(hbtree_mgr));
			}
		}
		else
		{
			unsigned int count = 0;
			unsigned int node_count = 0;
			ExaminBtree(hcur_master, &node_count, 1);
			assert(2 == btreeMgrGetPagerRefCount(hbtree_mgr));
			printf(".");

			{
				pager_info_t pgr_info;
				btreeMgrGetPagerInfo(hbtree_mgr, &pgr_info);

				/* 防止"外存泄漏" */
				//assert(pgr_info.total_page_count - pgr_info.free_page_count == (node_count + 1 + 100));
			}
		}

		assert(hcur);
		btreeMgrReleaseCursor(hcur);
	}

	MyBufferDestruct(hb);
	btreeMgrDestruct(hbtree_mgr);

	MyMemPoolMemReport(1);
}

void test_btree_new()
{
	unsigned int count = 0;
	int i = 0;
	HBTREE_MGR hbtree_mgr = NULL;
	HBTREE_CURSOR hcur_master = NULL;
	HBTREE_CURSOR hcur = NULL;

	/* 初始化随机记录种子 */
	unsigned int rand_seed = /*1207587769*/time(0);
	{
		FILE * pfile = fopen("rand_seed.txt", "aw");
		if(pfile)
		{
			fprintf(pfile, "btree db rand seed:%d\n", rand_seed);
			fclose(pfile);
		}
	}

	OsFileDel(TEST_FILE_DB);
	OsFileDel(TEST_FILE_DB_JOURNAL);

	printf("进行btree算法详细测试,每添加/删除一条都验证内存的b树完整性,以及外存文件的正确性");

	test_open_or_create_tbl();

	hbtree_mgr = btreeMgrConstruct(NULL,
		TEST_FILE_DB,
		PAGE_SZ,
		10,
		512,
		NULL, 0);

	btreeMgrOpenMaster(hbtree_mgr, &hcur_master, xcompare_master, NULL, 0);

	{
		char actemp[32] = {0,0,0,0,'a','b','c','d','t','e','s','t','1',0};
		HMYBUFFER hb = MyBufferConstruct(NULL, 0);
		btreeMgrOpenTable(hcur_master, &hcur, xcompare, NULL, 0, actemp, sizeof(actemp), hb);
		assert(hcur);
		MyBufferDestruct(hb);
	}

	{
		//unsigned int count = 0;
		//unsigned int node_count = 0;
		//unsigned int node_count_master = 0;
		////ExaminBtree(hcur, &node_count, 1);
		//ExaminBtree(hcur_master, &node_count_master, 1);

		//{
		//	pager_info_t pgr_info;
		//	btreeMgrGetPagerInfo(hbtree_mgr, &pgr_info);

		//	/* 防止"外存泄漏" */
		//	assert(pgr_info.total_page_count - pgr_info.free_page_count == node_count_master + node_count + 100);
		//}
		//assert(2 == btreeMgrGetPagerRefCount(hbtree_mgr));
	}

	srand(rand_seed);
	{
		unsigned int node_count = 0;
		ExaminBtree(hcur_master, &node_count, 1);
		test_add(hbtree_mgr, hcur, TEST_LOOP_1, node_count + 100, 1, 1);
	}

	ExaminBtree(hcur, NULL, 1);
	btreeMgrCommit(hbtree_mgr);

	count = btreeMgrGetCount(hcur);
	assert(TEST_LOOP_1 == count);

	LOG_DEBUG(("btree layer:%d", ExaminBtree(hcur, NULL, 0)));

	srand(rand_seed);
	{
		unsigned int node_count = 0;
		ExaminBtree(hcur_master, &node_count, 1);
		test_del(hbtree_mgr, hcur, TEST_LOOP_1, count, node_count + 100);
	}

	btreeMgrCommit(hbtree_mgr);
	count = btreeMgrGetCount(hcur);
	assert(0 == count);

	{
		unsigned int node_count = 0;
		pager_info_t pgr_info;
		btreeMgrGetPagerInfo(hbtree_mgr, &pgr_info);
		ExaminBtree(hcur_master, &node_count, 1);
		assert(pgr_info.total_page_count - pgr_info.free_page_count == node_count + 1 + 100);
	}

	LOG_DEBUG(("btree layer:%d", ExaminBtree(hcur, NULL, 0)));

	srand(rand_seed);
	{
		unsigned int node_count = 0;
		ExaminBtree(hcur_master, &node_count, 1);
		test_add(hbtree_mgr, hcur, TEST_LOOP_1, node_count + 100, 0, 0);
	}
	btreeMgrRollBack(hbtree_mgr);
	count = btreeMgrGetCount(hcur);
	assert(0 == count);

	//{
	//	unsigned int node_count = 0;
	//	pager_info_t pgr_info;
	//	btreeMgrGetPagerInfo(hbtree_mgr, &pgr_info);
	//	ExaminBtree(hcur, &node_count, 1);
	//	assert(pgr_info.total_page_count - pgr_info.free_page_count == node_count + 1);
	//}

	btreeMgrDestruct(hbtree_mgr);

	MyMemPoolMemReport(1);

	return;
}



















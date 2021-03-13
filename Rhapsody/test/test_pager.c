#include "pager.h"
#include <stdio.h>
#include "mylog.h"
#include "OsFile.h"


static void test_page1(int max_cache_pages)
{
#define TEST_FILE "./pager.test"
#define TEST_FILE_JOURNAL "./pager.test-journal"

	unsigned int page_size = 64;
	HPAGE_HEAD pg = NULL;
	HPAGE_HEAD pg2 = NULL;
	unsigned int pgno = 0;
	char * buf = 0;
	unsigned int i = 0;
	HPAGER hpgr = NULL;

	OsFileDel(TEST_FILE);
	OsFileDel(TEST_FILE_JOURNAL);

	hpgr = PagerConstruct(NULL, TEST_FILE, NULL, NULL, 
		0, max_cache_pages, &page_size, NULL, 0, 128);

	PagerExamin(hpgr, 1, 0, "page", 4);

	pg = PagerGetPage(hpgr, 1);
	assert(1 == PagerGetPageRefCount(pg));
	PagerExamin(hpgr, 1, 0, "page", 4);

	LOG_DEBUG(("test get page, create page file, and write init data"));
	for(i = 2; i < 1000; i ++)
	{
		pgno = i;
		pg2 = PagerGetPage(hpgr, pgno);
		PagerExamin(hpgr, 1, 0, "page", 4);

		if(NULL == pg2)
		{
			pgno = PagerGetPageNo(hpgr);
			PagerExamin(hpgr, 1, 0, "page", 4);
			pg2 = PagerGetPage(hpgr, pgno);
			PagerExamin(hpgr, 1, 0, "page", 4);
		}

		buf = (char *)PageHeadMakeWritable(pg2);
		PagerExamin(hpgr, 1, 0, "page", 4);
		assert(buf);
		sprintf(buf, "page %d", pgno);

		if(i % 2)
		{
			PagerReleasePage(pg2);
			PagerExamin(hpgr, 1, 0, "page", 4);
			assert(0 == PagerGetPageRefCount(pg2));
		}
		else
		{
			assert(1 == PagerGetPageRefCount(pg2));
		}

		printf(".");
	}

	if(PagerSyn(hpgr))
		LOG_ERR(("pager sync err"));

	PagerExamin(hpgr, 1, 0, "page", 4);

	LOG_DEBUG(("test get page, create page file, make dirty"));
	for(i = 2; i < 1000; i ++)
	{
		pgno = i;
		pg2 = PagerGetPage(hpgr, pgno);
		PagerExamin(hpgr, 1, 0, "page", 4);

		if(NULL == pg2)
		{
			pgno = PagerGetPageNo(hpgr);
			PagerExamin(hpgr, 1, 0, "page", 4);
			pg2 = PagerGetPage(hpgr, pgno);
			PagerExamin(hpgr, 1, 0, "page", 4);
		}

		buf = (char *)PageHeadMakeWritable(pg2);
		PagerExamin(hpgr, 1, 0, "page", 4);
		assert(buf);
		sprintf(buf, "re page %d", pgno);

		if(i % 2)
		{
			PagerReleasePage(pg2);
			PagerExamin(hpgr, 1, 0, "page", 4);
			assert(0 == PagerGetPageRefCount(pg2));
		}
		else
		{
			assert(2 == PagerGetPageRefCount(pg2));
		}

		printf(".");
	}

	for(i = 2; i < 1000; i ++)
	{
		pgno = i;
		if(!(i % 2))
		{
			pg2 = PagerGetPage(hpgr, pgno);
			assert(3 == PagerGetPageRefCount(pg2));
			PagerReleasePage(pg2);
			PagerReleasePage(pg2);
			PagerReleasePage(pg2);
			assert(0 == PagerGetPageRefCount(pg2));
		}
		else
		{
			pg2 = PagerGetPage(hpgr, pgno);
			PagerReleasePage(pg2);
			assert(0 == PagerGetPageRefCount(pg2));
		}
	}

	/* 检查是否有未解除的除用 */
	LOG_DEBUG(("%d", PagerGetRefCount(hpgr)));
	assert(1 == PagerGetRefCount(hpgr));

	PagerRollBack(hpgr);
	for(i = 2; i < 1000; i ++)
	{
		pg2 = PagerGetPage(hpgr, i);
		PagerReleasePage(pg2);
		assert(0 == PagerGetPageRefCount(pg2));
	}
	assert(1 == PagerGetRefCount(hpgr));
	PagerExamin(hpgr, 1, 0, "page", 4);

	LOG_DEBUG(("test release page num"));
	for(i = 2; i < 1000; i ++)
	{
		int j = 0;
		PagerReleasePageNo(hpgr, i);
		PagerExamin(hpgr, 1, 1, "page", 4);
		for(j = 2; j < 1000; j ++)
		{
			pg2 = PagerGetPage(hpgr, j);
			PagerReleasePage(pg2);
			assert(0 == PagerGetPageRefCount(pg2));
		}
		assert(1 == PagerGetRefCount(hpgr));

		printf(".");
	}

	PagerSyn(hpgr);

	LOG_DEBUG(("test get page num"));
	for(i = 2; i < 1000; i ++)
	{
		pgno = PagerGetPageNo(hpgr);
		PagerExamin(hpgr, 0, 1,"page", 4);
		assert(pgno);
		pg2 = PagerGetPage(hpgr, pgno);
		assert(pg2);
		PagerExamin(hpgr, 0, 1,"page", 4);

		buf = (char *)PageHeadMakeWritable(pg2);
		PagerExamin(hpgr, 0, 1,"page", 4);
		assert(buf);
		sprintf(buf, "new page %d", pgno);

		PagerReleasePage(pg2);
		PagerExamin(hpgr, 0, 1, "page", 4);
		assert(0 == PagerGetPageRefCount(pg2));
		assert(1 == PagerGetRefCount(hpgr));

		printf(".");
	}
	PagerSyn(hpgr);
	PagerExamin(hpgr, 0, 1,"page", 4);

	assert(1 == PagerGetRefCount(hpgr));

	MyMemPoolMemReport(0);

	PagerDestruct(hpgr);

	MyMemPoolMemReport(1);
}

static void test_page_1()
{
#define TEST_FILE "./pager.test"
#define TEST_FILE_JOURNAL "./pager.test-journal"

	unsigned int page_size = 64;
	HPAGE_HEAD pg = NULL;
	unsigned int pgno = 0;
	char * buf = 0;
	unsigned int i = 0;
	HPAGER hpgr = NULL;

	OsFileDel(TEST_FILE);
	OsFileDel(TEST_FILE_JOURNAL);

	hpgr = PagerConstruct(NULL, TEST_FILE, NULL, NULL, 
		0, 3, &page_size, NULL, 0, 128);

	for(i = 0; i < 4; i ++)
	{
		unsigned int pgno = i + 2;
		pg = PagerGetPage(hpgr, pgno);
		if(NULL == pg)
		{
			pgno = PagerGetPageNo(hpgr);
			pg = PagerGetPage(hpgr, pgno);
		}

		PageHeadMakeWritable(pg);

		PagerReleasePage(pg);
	}

	pg = PagerGetPage(hpgr, 1);
	PageHeadMakeWritable(pg);
	PagerReleasePage(pg);
	PagerSyn(hpgr);

	PagerDestruct(hpgr);
}

void test_page()
{
	test_page_1();
	test_page1(2000);
	test_page1(10);
}


//
///**
// * @brief 同步所有脏缓存页
// */
//extern int PagerSyn(HPAGER hpgr);
//
///**
// * @brief 取消所有的页的更改
// */
//extern int PagerRollBack(HPAGER hpgr);
//
///**
// * @brief 申请一个页号
// * @return 返回一个可用的空闲页号
// */
//extern unsigned int PagerGetPageNo(HPAGER hpgr);
//
///**
// * @brief 释放一个页号,将置成空闲页
// */
//extern int PagerReleasePageNo(HPAGER hpgr, unsigned int pgno);
//
///**
// * @brief 获取当前页文件中有多少页
// */
//extern unsigned int PagerGetTotalPagesCount(HPAGER hpgr);
//
///**
// * @brief 获取当前页文件中有多少空闲页
// */
//extern unsigned int PagerGetFreePages(HPAGER hpgr);
//
///**
// * @brief 去掉page文件中的空闲页
// */
//extern int PagerTruncate(HPAGER hpgr);
//
///**
// * @brief 重设使用率
// */
//extern int PagerSetUsedRate(HPAGER hpgr, int used_rate);
//
///**
// * @brief 从页缓存管理中获取一页 与PagerReleasePage是对偶的操作
// */
//extern HPAGE_HEAD PagerGetPage(HPAGER hpgr, unsigned int pgno);
//
///**
// * @brief 取消对当前页的引用 与PagerGetPage是对偶的操作
// */
//extern int PagerReleasePage(HPAGE_HEAD pg);
//
///**
// * @brief 获取页内容的缓冲区,用于写入
// */
//extern void * PageHeadMakeWritable(HPAGE_HEAD pg);
//
///**
// * @brief 获取某一个页内容,用于读取
// */
//extern const void * PageHeadMakeReadable(HPAGE_HEAD pg);
//
///**
// * @brief 获取某一页的用户数据
// */
//extern void * PageHeadGetUserData(HPAGE_HEAD pg);

















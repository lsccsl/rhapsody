#include "btree_demo.h"
#include <stdio.h>
#include <string.h>
#include "mymempool.h"
#include "myOsFile.h"

#ifdef WIN32
#include <winsock2.h>
#define snprintf _snprintf
#endif

extern void gettimeofday(struct timeval *ptv, void *tzp);
#define BTREE_TEST_LOOP (1000 * 1000)

/* ��ʾ����,��ʾbtree���ʹ�� */
void test_btree_demo()
{
	struct timeval tv1;
	struct timeval tv2;

	int i = 0;
	char actemp[32] = {0};
	//char ackey[60] = {0};
	void * data = NULL;
	unsigned int data_sz = 0;
	HBTREE_CURSOR hcur = NULL;
	HBTREE_MGR hbtree = NULL;

	myOsFileDel("btree.db");
	myOsFileDel("btree.db-journal");
	hbtree = BtreeDemoOpen("btree.db");
	
	if(NULL == hbtree)
	{
		printf("fail open btree db");
	}
	else
		printf("\r\n\r\nbegin btree test\r\n��ʼbtree�㷨����\r\n");

	/* �򿪱� */
	hcur = BtreeDemoOpenTable(hbtree, "test_table");
	//ExaminBtree(hcur, NULL, 1);

	gettimeofday(&tv1, 0);
	for(i = 0; i < BTREE_TEST_LOOP; i ++)
	{
		//sprintf(ackey, "%d", i);
		getcallid(actemp, sizeof(actemp) - 1);
		BtreeDemoInsert(hbtree, hcur, actemp, sizeof(actemp), actemp, sizeof(actemp));

		/*if(i == BTREE_TEST_LOOP/2)
			BtreeDemoCommit(hbtree);*/
	}
	gettimeofday(&tv2, 0);
	printf("�����ʱ:%f��\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);

	gettimeofday(&tv1, 0);
	BtreeDemoCommit(hbtree);
	gettimeofday(&tv2, 0);
	printf("�ύ��ʱ:%f��\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);

	/* ��Ӽ�¼,�����Ӧ�ļ�¼�Ѿ�����,�����ظ���� */
	BtreeDemoInsert(hbtree, hcur, "abc", (unsigned int)(strlen("abc") + 1), "abc", (unsigned int)(strlen("abc") + 1));
	BtreeDemoInsert(hbtree, hcur, "def", (unsigned int)(strlen("def") + 1), "def", (unsigned int)(strlen("def") + 1));

	/* ��ѯ */
	BtreeDemoSearch(hcur, "abc", (unsigned int)(strlen("abc") + 1), &data, &data_sz);
	if(data)
	{
		/* ��ӡ��ѯ��� */
		printf("%d:%s\r\n", data_sz, data);
		MyMemPoolFree(NULL, data);
		data = NULL;
	}

	BtreeDemoSearch(hcur, "def", (unsigned int)(strlen("def") + 1), &data, &data_sz);
	if(data)
	{
		/* ��ӡ��ѯ��� */
		printf("%d:%s\r\n", data_sz, data);
		MyMemPoolFree(NULL, data);
		data = NULL;
	}

	BtreeDemoDel(hbtree, hcur, "def", (unsigned int)(strlen("def") + 1));

	BtreeDemoSearch(hcur, "def", (unsigned int)(strlen("def") + 1), &data, &data_sz);
	if(data)
	{
		/* ��ӡ��ѯ��� */
		printf("%d:%s\r\n", data_sz, data);
		MyMemPoolFree(NULL, data);
		data = NULL;
	}
	else
	{
		printf("def not exist");
	}

	printf("\r\n�������\r\n");

	btreeMgrDestruct(hbtree);

	MyMemPoolMemReport(1);
}















#include "./bdb/db.h"
#include "myOsFile.h"
#ifdef WIN32
#include <winsock2.h>
#endif

#define DATABASE "bdb.db"
#pragma comment(lib, "./bdb/libdb45s.lib")

#define BDB_TEST_LOOP (1000 * 1000)

void test_bdb()
{
	struct timeval tv1;
	struct timeval tv2;
	DB * dbp;
	DBT key;
	DBT data;

	int ret;
	int i;
	char actemp[32] = {0};
	//char ackey[60] = {0};

	printf("bdb test\r\n");
	myOsFileDel(DATABASE);

	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		fprintf(stderr, "db_create: %s\n", db_strerror(ret));
		return ;
	}

	if ((ret = dbp->open(dbp, NULL, DATABASE, NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
		dbp->err(dbp, ret, "%s", DATABASE);
		return ;
	}

	{
		unsigned int a;
		unsigned int b;
		int c;
		dbp->get_cachesize(dbp, &a, &b, &c);
		printf("cache param %d %d %d\r\n", a, b, c);

		//ret = dbp->set_cachesize(dbp, 0, b, 4);

		//dbp->get_cachesize(dbp, &a, &b, &c);
		//printf("%d cache param %d %d %d\r\n", ret, a, b, c);
	}

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	data.data = actemp;
	data.size = sizeof(actemp);
	key.data = actemp;
	key.size = sizeof(actemp);

	gettimeofday(&tv1, 0);
	for(i = 0; i < BDB_TEST_LOOP; i ++)
	{
		//sprintf(ackey, "%d", i);
		getcallid(actemp, sizeof(actemp) - 1);

		//key.size = strlen(ackey);
		//data.size = sizeof(actemp);
		dbp->put(dbp, NULL, &key, &data, 0);

		/*if(i == BTREE_TEST_LOOP/2)
		{
			printf("sync bdb\r\n");
			dbp->sync(dbp, 0);
		}*/
		//printf(".");
	}
	gettimeofday(&tv2, 0);
	printf("添加用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);

	gettimeofday(&tv1, 0);
	dbp->sync(dbp, 0);
	gettimeofday(&tv2, 0);
	printf("提交用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);

	dbp->close(dbp, 0);

	printf("bdb test over\r\n");
}

//int main()
//{
//	DB *dbp;
//	DBT key,data;
//	int ret;
//
//	test_bdb();
//	while(1){}
//	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
//		fprintf(stderr, "db_create: %s\n", db_strerror(ret));
//		return -1;
//	}
//
//	if ((ret = dbp->open(dbp, NULL, DATABASE, NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
//		dbp->err(dbp, ret, "%s", DATABASE);
//		return -1;
//	}
//	
//	memset(&key, 0, sizeof(key));
//	memset(&data, 0, sizeof(data));
//
//	key.data = "aaaaaaaaaaaa";
//	key.size = sizeof("aaaaaaaaaaaa");
//	key.data = "aaaaaaaaaaaa";
//	key.size = sizeof("aaaaaaaaaaaa");
//	
//	if((ret = dbp->get(dbp, NULL, &key, &data, 0)) != 0)
//	{
//		printf("fail get \r\n");
//	}
//	else
//	{
//		printf("get %d %s\r\n", data.dlen, data.data);
//	}
//
//	memset(&key, 0, sizeof(key));
//	memset(&data, 0, sizeof(data));
//	key.data = "aaaaaaaaaaaa";
//	key.size = sizeof("aaaaaaaaaaaa");
//
//	data.data = "data1";
//	data.size = sizeof("data1");
//
//	if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) == 0)
//		printf("store ok\r\n");
//
//	dbp->sync(dbp, 0);
//
//	memset(&key, 0, sizeof(key));
//	memset(&data, 0, sizeof(data));
//	key.data = "aaaaaaaaaaaa";
//	key.size = sizeof("aaaaaaaaaaaa");
//	
//	if((ret = dbp->get(dbp, NULL, &key, &data, 0)) != 0)
//	{
//		printf("fail get \r\n");
//	}
//	else
//	{
//		printf("get %d %s\r\n", data.dlen, data.data);
//	}
//	
//
//	dbp->close(dbp, 0);
//
//	return 0;
//}







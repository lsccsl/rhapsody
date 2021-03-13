#include <stdlib.h>
#include <winsock2.h>
#include <map>
#include <string>
#include <queue>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <algorithm>

extern "C"{
#include "mylog.h"
}

#ifdef __cplusplus
extern "C"
{
#endif
	void test_my_string_set_ex();
	void test_mybuffer();
	void test_list();
	void test_rbtree();
	void test_rbtree1();
	void test_ini();
	void test_hash();
	void test_vector();
	void test_map();
	void test_hashmap();
	void test_string_set();
	void test_mystring_set();
	void getcallid(char * callid, size_t callid_len);
	void gettimeofday(struct timeval *ptv, void *tzp);
	void test_msgparse();
	void test_list_ex();
	void test_vector_ex();
	void test_vector_heap();
	void test_deque();
	void test_heap();
	void test_timer_heap();
	void test_timer_thread();
	void test_listerner();
	void test_heapSort();
	void test_quickSort();
	void test_insertSort();
	void test_avltree();
	void test_rhapsody_mem_pool();
	void test_binary_search();
	void test_ttree();
	void test_btree();
	void test_bitvector();
	void test_page();
	void test_osfile();
	void test_rand();
	void test_btree_new();
	void test_btree_demo();
	void test_uos();
	void test_timer_thread_del();
	//void test_queue();
#ifdef __cplusplus
}

unsigned long read_ini_crc();

#endif
void test_stl_map();
void test_boost();
void test_pipe();
static void test_stl_sort();

extern "C" unsigned long __temp_[10000] ;
extern "C" unsigned long __temp_1 ;
extern "C" unsigned long __swap_unsigned_long_ ;

extern "C" int test_alg_compare_unsigned_long1(const void * data1,
						   const void * data2);

typedef int (*ALG_COMPARE1)(const void * data1,
						   const void * data2);

ALG_COMPARE1 temp_compare = test_alg_compare_unsigned_long1;

class pred
{
public:
	int operator ()(unsigned long k1, unsigned long k2)
	{
		return temp_compare(&k1, &k2);
	}
};

static void test_stl_sort()
{
	struct timeval tv1;
	struct timeval tv2;

	int i = 0;
	srand(0);
	for(i = 0 ;i < sizeof(__temp_)/sizeof(__temp_[0]); i ++)
	{
		__temp_[i] = rand();
	}

	LOG_DEBUG(("测试stl快速排序"));

	gettimeofday(&tv1, 0);
	//std::stable_sort(__temp_, &__temp_[sizeof(__temp_)/sizeof(__temp_[0])]);
	//std::make_heap(__temp_, &__temp_[sizeof(__temp_)/sizeof(__temp_[0])], pred());
	//std::sort_heap(__temp_, &__temp_[sizeof(__temp_)/sizeof(__temp_[0])], pred());
	std::sort(__temp_, &__temp_[sizeof(__temp_)/sizeof(__temp_[0])], pred());
	gettimeofday(&tv2, 0);
	printf("stl快速排序用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);
}

int main()
{
	int i = 0;

	for(i = 0; i < 1; i ++)
	{
		test_rhapsody_mem_pool();

		test_btree_demo();
		test_btree_new();
		test_page();
		test_rand();
		test_osfile();

		test_timer_thread_del();
		test_listerner();
		test_uos();

		test_bitvector();
		test_btree();
		test_binary_search();
		test_heap();
		test_vector_heap();

		test_timer_heap();
		test_timer_thread();

		test_string_set();

		test_hashmap();
		test_hash();

		test_stl_sort();
		test_heapSort();
		test_quickSort();
		test_insertSort();
		test_avltree();
		test_rbtree();
		test_rbtree1();
		test_map();

		test_ttree();

		test_vector_ex();
		test_vector();
		test_list_ex();
		test_mybuffer();
		test_list();

		test_deque();
	}

		//test_ini();
		//test_mystring_set();
		//test_my_string_set_ex();
		//test_msgparse();
		//printf("\n");
		//test_boost();
		//printf("\n");
		//test_stl_map();
		//printf("\n");
		//test_queue();

	return 0;
}



#define LOOP_COUNT 100000

struct stl_mag_key
{
	//stl_map_key(){}
	//stl_map_key(stl_mag_key& right)
	//{
	//	strncpy(callid, right.callid, sizeof(callid));
	//}

	char callid[32];
	bool operator < (const stl_mag_key& right)const
	{
		if(strcmp(this->callid, right.callid) > 0)
			return true;
		return false;
	}
};

void test_stl_map()
{
	struct timeval tv1;
	struct timeval tv2;
	int i = 0;
	std::map<stl_mag_key,int> stl_map;
	std::map<stl_mag_key,int>::iterator it;

	srand(0);
	gettimeofday(&tv1, 0);
	for(i = 0; i < LOOP_COUNT; i++)
	{
		stl_mag_key key = {0};
		getcallid(key.callid, sizeof(key.callid) - 1);

		stl_map[key] = i;
	}
	gettimeofday(&tv2, 0);
	printf("stl-map添加用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);

	srand(0);
	gettimeofday(&tv1, 0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		stl_mag_key key = {0};
		getcallid(key.callid, sizeof(key.callid) - 1);
		stl_map.find(key);
	}
	gettimeofday(&tv2, 0);
	printf("stl-map查找用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);

	srand(0);
	gettimeofday(&tv1, 0);
	for(i = 0; i < LOOP_COUNT; i ++)
	{
		stl_mag_key key = {0};
		getcallid(key.callid, sizeof(key.callid) - 1);
		stl_map.erase(key);
	}
	gettimeofday(&tv2, 0);
	printf("stl-map删除用时:%f秒\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0);
}

void test_queue()
{
	int i = 0;
	std::queue<int> q;
	for(i = 0; i < 1200; i ++)
	{
		q.push(i);
		//q.pop();
	}

	for(i = 0; i < 1000; i ++)
	{
		q.push(i);
		q.pop();
	}
	for(i = 0; i < 1200; i ++)
	{
		//q.push(i);
		q.pop();
	}
}


void getcallid(char * callid, size_t callid_len)
{
	/*int i = 0;
	for(i=0; i<callid_len - 1; i++)
	{		
		char ucRandomNum = (char)(rand()%36);
		if(ucRandomNum<=9)
			callid[i] = '0'+ucRandomNum;
		else
			callid[i] = 'a'+ucRandomNum-10;
	}

	callid[callid_len - 1] = 0;*/

	int i = 0;
	for(i=0; i<callid_len - 1; i++)
	{	
		/*if(i <= 23)
		{
			callid[i] = 'a';
			continue;
		}
		else*/
		{
			char ucRandomNum = (char)(rand()%26);
			//if(ucRandomNum<=9)
			//	callid[i] = '0'+ucRandomNum;
			//else
				callid[i] = 'a'+ucRandomNum;
		}
	}

	callid[callid_len - 1] = 0;
}

//int fdpipe[2] = {0};
//int fd = 0;
//
//static void test_pipe_thread(void * param)
//{
//	::Sleep(3 * 1000);
//
//	::send(fd, "socket", sizeof("socket"), 0);
//	::write(fd, "socket", sizeof("socket"));
//	::_write(fd, "socket", sizeof("socket"));
//
//	::_write(fdpipe[1], "abc", sizeof("abc"));
//}
//
//void test_pipe()
//{
//	struct fd_set fd_set1 = {0};
//	char actemp[32] = {0};
//	WORD version_requested = MAKEWORD (1, 1);
//	WSADATA wsa_data;
//	int error = WSAStartup (version_requested, &wsa_data);
//	::_pipe(fdpipe, 256, O_BINARY);
//
//
//	::_beginthread(test_pipe_thread, 0, NULL);
//
//	fd = ::socket(AF_INET, SOCK_RAW, PF_INET);
//
//	FD_SET(fd, &fd_set1);
//	::select(1, &fd_set1, NULL, NULL, NULL);
//
//	::recv(fd, actemp, sizeof(actemp), 0);
//	printf("%s\n", actemp);
//
//	::_read(fdpipe[0], actemp, sizeof(actemp));
//	printf("%s\n", actemp);
//}















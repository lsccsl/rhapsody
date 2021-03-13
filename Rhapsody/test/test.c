#include <stdlib.h>

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
	void test_event_pipe();
	void test_page();
	void test_osfile();
	void test_rand();
	void test_btree_new();
	void test_btree_demo();
	void test_uos();
	void test_bdb();
#ifdef __cplusplus
}
#endif


int main()
{
	int i = 0;

	for(i = 0; i < 1; i ++)
	{
#ifdef WIN32	/* 只在windows平台下做btree性能比较测试 */
#ifdef NDEBUG	/* 在release版本中进行btree性能测试 */
		
#ifndef _MBCSV6	/* 只在vs2003的工程里做btree性能测试 */
		/* berkeleyDB的性能测试 */
		test_bdb();
#endif
		/* btree数据库性能测试 */
		test_btree_demo();
#endif
#endif
		/* btree数据库索引算法测试 */
		test_btree_new();
		test_page();
		test_osfile();

		/* 定时器,os api等相关封装的测试 */
		test_rand();
		test_rhapsody_mem_pool();
		test_timer_thread_del();
		test_listerner();
		test_uos();
		test_timer_heap();
		test_timer_thread();

		/* 容器算法相关的测试 */
		test_bitvector();
		test_btree();
		test_binary_search();
		test_heap();
		test_vector_heap();
		test_string_set();
		test_hashmap();
		test_hash();
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
		//test_stl_sort();
		//printf("\n");
		//test_stl_map();
		//printf("\n");
		//test_queue();

	return 0;
}










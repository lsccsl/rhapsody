#include "MyAlgorithm.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "mymempool.h"
#include "mylog.h"
#ifdef WIN32
#include <winsock2.h>
#endif


static unsigned char __tempheap_[] = {100, 8, 43, 6, 7, 5, 45, 89, 90};
static unsigned char __tempheap_1[] = {5, 6, 7, 8, 43, 45, 89, 90, 100};
static unsigned char __swaperheap_ = 0;

unsigned long __temp_[10000] = {0};
unsigned long __temp_1 = 0;
unsigned long __swap_unsigned_long_ = 0;

static int test_alg_compare(const void * data1,
						   const void * data2, const void * context)
{
	return (*(unsigned char *)data1) - (*(unsigned char *)data2);
}

int test_alg_compare_unsigned_long(const void * data1,
						   const void * data2, const void * context)
{
	return (*(unsigned long *)data1) - (*(unsigned long *)data2);
}


int test_alg_compare_unsigned_long1(const void * data1,
						   const void * data2, const void * context)
{
	return (*(unsigned long *)data1) > (*(unsigned long *)data2);
}

void test_heapSort()
{
	struct timeval tv1;
	struct timeval tv2;

	int i = 0;
	srand(0);
	for(i = 0 ;i < sizeof(__temp_)/sizeof(__temp_[0]); i ++)
	{
		__temp_[i] = rand();
	}

	LOG_DEBUG(("²âÊÔ¶ÑÅÅÐò"));

	gettimeofday(&tv1, 0);
	MyAlgMakeHeap(__temp_, sizeof(__temp_)/sizeof(__temp_[0]), sizeof(__temp_[0]), test_alg_compare_unsigned_long, NULL, NULL, NULL, 0);
	MyAlgHeapSort(__temp_, sizeof(__temp_)/sizeof(__temp_[0]), sizeof(__temp_[0]), test_alg_compare_unsigned_long, NULL, NULL, NULL, 0);
	gettimeofday(&tv2, 0);
	LOG_INFO(("¶ÑÅÅÐòÓÃÊ±:%fÃë\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0));

	//MyAlgExaminHeap(__temp_, sizeof(__temp_)/sizeof(__temp_[0]), sizeof(__temp_[0]), test_alg_compare_unsigned_long, NULL);
	MyAlgSortOK(__temp_, sizeof(__temp_)/sizeof(__temp_[0]), sizeof(__temp_[0]), test_alg_compare_unsigned_long, NULL);

	MyAlgMakeHeap(__tempheap_, sizeof(__tempheap_)/sizeof(__tempheap_[0]), sizeof(unsigned char), test_alg_compare, NULL, NULL, &__swaperheap_, sizeof(__swaperheap_));
	MyAlgExaminHeap(__tempheap_, sizeof(__tempheap_)/sizeof(__tempheap_[0]), sizeof(unsigned char), test_alg_compare, NULL);
	MyAlgHeapSort(__tempheap_, sizeof(__tempheap_)/sizeof(__tempheap_[0]), sizeof(unsigned char), test_alg_compare, NULL, NULL, &__swaperheap_, sizeof(__swaperheap_));
	assert(0 == memcmp(__tempheap_, __tempheap_1, sizeof(__tempheap_1)));
	MyAlgSortOK(__tempheap_, sizeof(__tempheap_)/sizeof(__tempheap_[0]), sizeof(unsigned char), test_alg_compare, NULL);

	MyMemPoolMemReport(1);
}

void test_quickSort()
{
	struct timeval tv1;
	struct timeval tv2;

	int i = 0;
	srand(0);
	for(i = 0 ;i < sizeof(__temp_)/sizeof(__temp_[0]); i ++)
	{
		__temp_[i] = rand();
	}

	LOG_DEBUG(("²âÊÔ¿ìËÙÅÅÐò"));
	gettimeofday(&tv1, 0);
	MyAlgQuickSort(__temp_, sizeof(__temp_)/sizeof(__temp_[0]), sizeof(__temp_[0]), test_alg_compare_unsigned_long, NULL, NULL, NULL, NULL, NULL, 0);
	gettimeofday(&tv2, 0);
	LOG_INFO(("²âÊÔ¿ìËÙÅÅÐò:%fÃë\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0));

	MyAlgSortOK(__temp_, sizeof(__temp_)/sizeof(__temp_[0]), sizeof(__temp_[0]), test_alg_compare_unsigned_long, NULL);
	MyMemPoolMemReport(1);
}

void test_insertSort()
{
	struct timeval tv1;
	struct timeval tv2;

	int i = 0;
	for(i = 0 ;i < sizeof(__temp_)/sizeof(__temp_[0]); i ++)
	{
		__temp_[i] = rand();
	}
	LOG_DEBUG(("²âÊÔ²åÈëÊ½ÅÅÐò"));

	gettimeofday(&tv1, 0);
	MyAlgInsertSort(__temp_, sizeof(__temp_)/sizeof(__temp_[0]), sizeof(__temp_[0]), test_alg_compare_unsigned_long, 
		NULL, NULL, NULL, NULL, NULL, 0);
	gettimeofday(&tv2, 0);
	LOG_INFO(("²åÈëÊ½ÅÅÐòÓÃÊ±:%fÃë\n", tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0));

	MyAlgSortOK(__temp_, sizeof(__temp_)/sizeof(__temp_[0]), sizeof(__temp_[0]), test_alg_compare_unsigned_long, NULL);
	MyMemPoolMemReport(1);
}

















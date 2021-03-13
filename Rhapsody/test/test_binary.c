#include "MyAlgorithm.h"
#include <assert.h>
#include "mylog.h"

static int __temp_binary_search_array[1000] = {0};

int test_binary_search_compare(const void * data1,
						   const void * data2, const void * context)
{
	return (*(int *)data1) - (*(int *)data2);
}

void test_binary_search()
{
	int i = 0;

	LOG_INFO(("test binary search begin"));

	for(i = 0; i < sizeof(__temp_binary_search_array) / sizeof(__temp_binary_search_array[0]); i ++)
	{
		__temp_binary_search_array[i] = i * 2 + 1;
	}

	for(i = 0; i < sizeof(__temp_binary_search_array) / sizeof(__temp_binary_search_array[0]); i ++)
	{
		int temp = i * 2 + 1;
		size_t index = 0;

		int ret = MyBinarySearch1(__temp_binary_search_array, 
			sizeof(__temp_binary_search_array) / sizeof(__temp_binary_search_array[0]),
			sizeof(__temp_binary_search_array[0]), &temp, test_binary_search_compare, &index, NULL);

		assert(index == i);
		assert(ret == 0);
		printf(".");
	}

	LOG_INFO(("test binary search begin (no found)"));
	{
		size_t index = 0;
		for(i = 0; i < sizeof(__temp_binary_search_array) / sizeof(__temp_binary_search_array[0]); i ++)
		{
			int temp = i * 2;
			//size_t index = 0;

			int ret = MyBinarySearch1(__temp_binary_search_array, 
				sizeof(__temp_binary_search_array) / sizeof(__temp_binary_search_array[0]),
				sizeof(__temp_binary_search_array[0]), &temp, test_binary_search_compare, &index, NULL);

			assert(index == i);
			assert(ret == -1);
			printf(".");
		}
		//index = index;
	}
}

#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "mydeque.h"

#ifdef __cplusplus
}
#endif

void test_deque1(const int LOOP, const size_t buffer_size, const size_t map_size)
{
//#define LOOP 100
	int i = 0;
	HMYDEQUE hdq = MyDequeConstruct(NULL, NULL, buffer_size, map_size);

	for(i = 0; i < LOOP; i ++)
	{
		MyDequeAddHead(hdq, (void *)i, 0);
	}
	//MyDequePrint(hdq);
	assert(MyDequeGetCount(hdq) == LOOP);
	{
		void * data = 0;
		MyDequeGetHead(hdq, &data, NULL);
		assert(data == (void *)(LOOP - 1));
		MyDequeGetTail(hdq, &data, NULL);
		assert(data == 0);
	}

	for(i = 0; i < LOOP; i ++)
	{
		MyDequeAddTail(hdq, (void *)(i + LOOP), 0);
	}
	assert(MyDequeGetCount(hdq) == 2 * LOOP);
	//MyDequePrint(hdq);
	{
		void * data = 0;
		MyDequeGetHead(hdq, &data, NULL);
		assert(data == (void *)(LOOP - 1));
		MyDequeGetTail(hdq, &data, NULL);
		assert(data == (void *)(2 * LOOP - 1));
	}

	for(i = 0; i < LOOP; i ++)
	{
		MyDequeDelHead(hdq);
	}
	assert(MyDequeGetCount(hdq) == LOOP);
	//MyDequePrint(hdq);
	assert(MyDequeGetCount(hdq) == LOOP);
	{
		void * data = 0;
		MyDequeGetHead(hdq, &data, NULL);
		assert(data == (void *)LOOP);
		MyDequeGetTail(hdq, &data, NULL);
		assert(data == (void *)(2 * LOOP - 1));
	}

	for(i = 0; i < LOOP; i ++)
	{
		MyDequeDelTail(hdq);
	}
	assert(MyDequeGetCount(hdq) == 0);
	//MyDequePrint(hdq);
	//MyMemPoolMemReport(0);
	{
		assert(MyDequeGetHead(hdq, NULL, NULL) == -1);
		assert(MyDequeGetTail(hdq, NULL, NULL) == -1);
	}

	//==================================================================//

	for(i = 0; i < LOOP; i ++)
	{
		MyDequeAddHead(hdq, (void *)i, 0);
	}
	assert(MyDequeGetCount(hdq) == LOOP);
	//MyDequePrint(hdq);
	{
		void * data = 0;
		MyDequeGetHead(hdq, &data, NULL);
		assert(data == (void *)(LOOP - 1));
		MyDequeGetTail(hdq, &data, NULL);
		assert(data == 0);
	}

	for(i = 0; i < LOOP; i ++)
	{
		MyDequeAddTail(hdq, (void *)(i + LOOP), 0);
	}
	assert(MyDequeGetCount(hdq) == LOOP * 2);
	//MyDequePrint(hdq);
	{
		void * data = 0;
		MyDequeGetHead(hdq, &data, NULL);
		assert(data == (void *)(LOOP - 1));
		MyDequeGetTail(hdq, &data, NULL);
		assert(data == (void *)(2 * LOOP - 1));
	}

	for(i = 0; i < LOOP; i ++)
	{
		MyDequeDelHead(hdq);
	}
	assert(MyDequeGetCount(hdq) == LOOP);
	//MyDequePrint(hdq);
	assert(MyDequeGetCount(hdq) == LOOP);
	{
		void * data = 0;
		MyDequeGetHead(hdq, &data, NULL);
		assert(data == (void *)LOOP);
		MyDequeGetTail(hdq, &data, NULL);
		assert(data == (void *)(2 * LOOP - 1));
	}

	for(i = 0; i < LOOP; i ++)
	{
		MyDequeDelTail(hdq);
	}
	assert(MyDequeGetCount(hdq) == 0);
	//MyDequePrint(hdq);
	{
		assert(MyDequeGetHead(hdq, NULL, NULL) == -1);
		assert(MyDequeGetTail(hdq, NULL, NULL) == -1);
	}

	//MyMemPoolMemReport(0);

	for(i = 0; i < LOOP; i ++)
	{
		MyDequeAddTail(hdq, (void *)(i + LOOP), 0);
	}
	assert(MyDequeGetCount(hdq) == LOOP);
	{
		void * data = 0;
		MyDequeGetHead(hdq, &data, NULL);
		assert(data == (void *)LOOP);
		MyDequeGetTail(hdq, &data, NULL);
		assert(data == (void *)(2 * LOOP - 1));
	}

	MyDequeDestruct(hdq);

	MyMemPoolMemReport(1);
}

void test_deque()
{
	int i = 1;
	for(i = 1; i <= 1000; i += 50)
	{
		test_deque1(i, 4, 8);
	}
}













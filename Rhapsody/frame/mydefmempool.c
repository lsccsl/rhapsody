/**
 *
 * @file mydefmempool.c �ڴ�� 2007-8-25 2:23
 * @brief ���Ҫ������ڴ��С��8�ֽ�,����8�ֽ�,
 * 	������С���ڴ��������,���뵽8�ֽ�,
 * 	����128�ֽڵ��ڴ�����������malloc����
 * @author lin shao chuan 
 *
 */
#include "mydefmempool.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "mymutex.h"


static const unsigned int log_2_table[] =
{
	8,   8,   8,   8,   8,   8,   8,   8,
	16,  16,  16,  16,  16,  16,  16,  16,
	24,  24,  24,  24,  24,  24,  24,  24,
	32,  32,  32,  32,  32,  32,  32,  32,
	40,  40,  40,  40,  40,  40,  40,  40,
	48,  48,  48,  48,  48,  48,  48,  48,
	56,  56,  56,  56,  56,  56,  56,  56,
	64,  64,  64,  64,  64,  64,  64,  64,
	72,  72,  72,  72, 	72,  72,  72,  72,
	80,  80,  80,  80,  80,  80,  80,  80,	
	88,  88,  88,  88,  88,  88,  88,  88,	
	96,  96,  96,  96,  96,  96,  96,  96,	
	104, 104, 104, 104, 104, 104, 104, 104,	
	112, 112, 112, 112, 112, 112, 112, 112,	
	120, 120, 120, 120, 120, 120, 120, 120,	
	128, 128, 128, 128, 128, 128, 128, 128,
	128,
};
#define MAX_BLK_SIZE ((sizeof(log_2_table)/sizeof(log_2_table[0])) - 1)
#define HEAD_SIZE (4)

typedef struct __rhapsody_block_t_
{
	size_t blk_size;
	struct __rhapsody_block_t_ * next;
}rhapsody_block;

typedef struct __rhapsody_mempool_t_
{
	/*
	* �ڴ�ر�����
	*/
	HMYMUTEX protecter;
	
	/*
	* �����ڴ���¼��
	*/
	rhapsody_block * blk[(MAX_BLK_SIZE/8) + 1];	
}rhapsody_mempool;


/**
 * @brief �����ڴ��
 */
static rhapsody_mempool * create_mempool()
{
	/* �����ڴ������Ϣ�ռ�,��ʼ�������� */
	rhapsody_mempool * mp = (rhapsody_mempool *)MyMemPoolMalloc(NULL, sizeof(*mp));
	
	mp->protecter = MyMutexConstruct(NULL);
	
	memset(mp->blk, 0, sizeof(mp->blk));
	
	return mp;
}

#define RH_MEMPOOL(x) ((rhapsody_mempool *)x)

/**
 * @brief �����ڴ�Ļص�����
 */
static void * rhapsody_malloc(size_t size, void * context_data)
{
	size_t index = 0;
	rhapsody_block * ptr = NULL;
	
	assert(context_data);
	
	/* �������128,�����,ֱ��malloc */
	if(size > MAX_BLK_SIZE)
	{
		ptr = (rhapsody_block *)malloc(HEAD_SIZE + size);
		ptr->blk_size = size;
		ptr->next = NULL;

		return ((char *)ptr) + HEAD_SIZE;
	}

	/* �����ڴ����������,�ó�Ӧ������ڴ���С */
	index = log_2_table[size]/8;

	assert(log_2_table[size] >= size);

	/* ���� */
	MyMutexLock(RH_MEMPOOL(context_data)->protecter);

	assert(index <= sizeof(RH_MEMPOOL(context_data)->blk) / sizeof(RH_MEMPOOL(context_data)->blk[0]));
	
	/* ����Ӧ�����ڴ���Сȥ��Ӧ�Ŀ����ڴ��¼����ȡ�ڴ� */
	ptr = RH_MEMPOOL(context_data)->blk[index];
	if(ptr)
	{
		RH_MEMPOOL(context_data)->blk[index] = ptr->next;

		assert(log_2_table[size] == ptr->blk_size);
	}
	else
	{
		/* �Ҳ���, ������ڴ� */
		ptr = (rhapsody_block *)malloc(HEAD_SIZE + log_2_table[size]);
		ptr->blk_size = log_2_table[size];
		ptr->next = NULL;	
	}

	/* ���� */
	MyMutexUnLock(RH_MEMPOOL(context_data)->protecter);
	
	return ((char *)ptr) + HEAD_SIZE;
}

/**
 * @brief �ͷ��ڴ�Ļص�����
 */
static void rhapsody_free(void * ptr, void * context_data)
{
	size_t index = 0;	

	/* �������ƫ��,�ó��ڴ�����Ϣ */
	rhapsody_block * ptr_blk = (rhapsody_block *)(((char *)ptr) - HEAD_SIZE);

	assert(context_data);

	/* ���� */	
	MyMutexLock(RH_MEMPOOL(context_data)->protecter);

	/* ����128���ڴ�ֱ��free */
	if(ptr_blk->blk_size > MAX_BLK_SIZE)
	{
		free(ptr_blk);
		goto rhapsody_free_end_;
	}

	assert(ptr_blk->blk_size == log_2_table[ptr_blk->blk_size - 1]);

	/* ����������� */
	index = log_2_table[ptr_blk->blk_size - 1]/8;
	
	assert(index <= sizeof(RH_MEMPOOL(context_data)->blk) / sizeof(RH_MEMPOOL(context_data)->blk[0]));

	ptr_blk->next = RH_MEMPOOL(context_data)->blk[index];
	
	RH_MEMPOOL(context_data)->blk[index] = ptr_blk;
		
rhapsody_free_end_:

	/* ���� */
	MyMutexUnLock(RH_MEMPOOL(context_data)->protecter);
}

/**
 * @brief �ͷ��ڴ�Ļص�����
 */
static void rhapsody_destroy(void * context_data)
{
	int i = 0;
	int blk_count = 0;
	rhapsody_mempool * mp = (rhapsody_mempool *)context_data;
	if(NULL == mp)
		return;

	/* ���� */	
	MyMutexLock(RH_MEMPOOL(context_data)->protecter);
	
	for(i = 0; i < sizeof(mp->blk)/sizeof(mp->blk[0]); i ++)
	{
		rhapsody_block * blk = mp->blk[i];
		while(blk)
		{
			rhapsody_block * temp = blk;
			blk_count ++;
			blk = blk->next;
			free(temp);
		}
	}	
	
	/* ���� */
	MyMutexUnLock(RH_MEMPOOL(context_data)->protecter);
	
	MyMutexDestruct(RH_MEMPOOL(context_data)->protecter);
	
	MyMemPoolFree(NULL, context_data);

	#ifdef _MBCSV6
		printf("[%s:%d]rhapsody_destroy %x %d blk has been free\r\n", __FILE__, __LINE__, context_data, blk_count);
	#else
	#ifdef WIN32
		printf("[%s:%d]rhapsody_destroy %d blk has been free from pool %x \r\n", __FILE__, __LINE__, blk_count, (long long)context_data);
	#else
		printf("[%s:%d]rhapsody_destroy %x %d blk has been free\r\n", __FILE__, __LINE__, context_data, blk_count);
	#endif
	#endif
}

/**
 * @brief �ͷ��ڴ�Ļص�����
 */
static void rhapsody_view(void * context_data, void * info, size_t info_size)
{
	int i = 0;
	int blk_count = 0;

	rhapsody_mempool * mp = (rhapsody_mempool *)context_data;
	if(NULL == mp)
		return;

	/* ���� */	
	MyMutexLock(RH_MEMPOOL(context_data)->protecter);

	for(i = 0; i < sizeof(mp->blk)/sizeof(mp->blk[0]); i ++)
	{
		rhapsody_block * blk = mp->blk[i];

		#ifdef _MBCSV6		
			printf("[%s:%d]blk : %x\r\n", __FILE__, __LINE__, blk);
		#else
		#ifdef WIN32
			printf("[%s:%d]blk : %x\r\n", __FILE__, __LINE__, (long long)blk);
		#else
			printf("[%s:%d]blk : %x\r\n", __FILE__, __LINE__, blk);
		#endif
		#endif

		while(blk)
		{
			blk_count ++;
			blk = blk->next;
		}
	}

	/* ���� */
	MyMutexUnLock(RH_MEMPOOL(context_data)->protecter);
	
	printf("[%s:%d]rhapsody mem pool has blk : %d\r\n\r\n", __FILE__, __LINE__, blk_count);
	
	if(info && info_size == sizeof(rhapsody_info_t))
		((rhapsody_info_t *)info)->blk_count = blk_count;
}

/**
 * @brief ����һ���ڴ��
 */
HMYMEMPOOL RhapsodyMemPoolConstruct()
{
	rhapsody_mempool * rmp = create_mempool();

	return MyMemPoolConstruct(rhapsody_malloc, rhapsody_free, rhapsody_destroy, rhapsody_view, rmp);
}













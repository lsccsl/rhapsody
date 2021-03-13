/**
 *
 * @file MyBinarySearch1.c ���ֲ���
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "MyAlgorithm.h"

#include <assert.h>

#include "myutility.h"


/**
 * @brief ��һ�����������������ֲ���
 */
static int __alg_binary_search(const void * buf, size_t len, size_t step_size,
							   const void * key,
							   ALG_COMPARE compare, size_t * pindex,
							   const void * context)
{
	size_t hi = len;
	size_t lo = 0;

	assert(buf && len && step_size && compare);

	while(hi > lo)
	{
		size_t mid = (hi + lo) / 2;
		int ret = compare(key, GET_INDEX_PTR(buf, mid, step_size), context);

		if(ret > 0)
		{
			lo = mid + 1;
			continue;
		}
		else if(ret < 0)
		{
			hi = mid;
			continue;
		}
		else
		{
			if(pindex)
				*pindex = mid;
			return 0;
		}
	}

	assert(hi == lo);

	if(pindex)
		*pindex = lo;

	return -1;
}


/**
 *
 * @brief ��һ�����������������ֲ���
 *
 * @param buf:�����������ݻ�������ʼ��ַ
 * @param len:���������е�Ԫ�ظ���
 * @param step_size:ÿ��Ԫ�صĴ�С
 * @param key:Ҫ���ҵĹؼ���
 * @param compare:�Ƚϻص�(����Ϊ��)
 * @param pindex:����ҵ�,����Ԫ�ص�λ��,
 *               ����Ҳ���,�򷵻�Ԫ��Ӧ��ӵ�λ��,����:
 *               1 3 4 6 7 --- Դ����,����2,
 *               pindex�����ó�[1],����ʾ2Ӧ��ӵ�1��3֮��
 *
 * @retval 0:�ɹ�
 * @retval ����:ʧ��
 *
 */
int MyBinarySearch1(const void * buf, size_t len, size_t step_size,
					const void * key,
					ALG_COMPARE compare, size_t * pindex,
					const void * context)
{
	if(NULL == buf || 0 == len || 0 == step_size || NULL == compare)
		return -1;

	return __alg_binary_search(buf, len, step_size, key, compare, pindex, context);
}


///**
// * @brief
// *  > 0  ��ʾ key1 ���� key2
// *  == 0 ��ʾ key1 ���� key2
// *  < 0  ��ʾ key1 С�� key2
// *
// * @param context:�û��Զ��������������
// */
//typedef int (*BINSEARCH_COMPARE)(const void * key, unsigned int key_sz, 
//								 const void * data2, 
//								 const void * context, unsigned int context_sz);

/**
 * @brief ��һ�����������������ֲ���
 *
 * @param buf:�����������ݻ�������ʼ��ַ
 * @param len:���������е�Ԫ�ظ���
 * @param step_size:ÿ��Ԫ�صĴ�С
 * @param key:Ҫ���ҵĹؼ���
 * @param key_sz:�ؼ��ֻ���ĳ���
 * @param compare:�Ƚϻص�(����Ϊ��)
 * @param pindex:����ҵ�,����Ԫ�ص�λ��,
 *               ����Ҳ���,�򷵻�Ԫ��Ӧ��ӵ�λ��,����:
 *               1 3 4 6 7 --- Դ����,����2,
 *               pindex�����ó�[1],����ʾ2Ӧ��ӵ�1��3֮��
 * @param context:��������Ϣ
 * @param context_sz:��������Ϣ����ĳ���
 *
 * @retval 0:�ɹ�
 * @retval ����:ʧ��
 */
int MyBinarySearch(const void * buf, unsigned int len, unsigned int step_size,
				   const void * key, unsigned int key_sz,
				   BINSEARCH_COMPARE compare, unsigned int * pindex,
				   const void * context, unsigned int context_sz)
{
	unsigned int hi = len;
	unsigned int lo = 0;

	if(NULL == buf || 0 == len || 0 == step_size || NULL == compare)
		return -1;

	while(hi > lo)
	{
		unsigned int mid = (hi + lo) / 2;
		int ret = compare(key, key_sz, GET_INDEX_PTR(buf, mid, step_size), context, context_sz);

		if(ret > 0)
		{
			lo = mid + 1;
			continue;
		}
		else if(ret < 0)
		{
			hi = mid;
			continue;
		}
		else
		{
			if(pindex)
				*pindex = mid;
			return 0;
		}
	}

	assert(hi == lo);

	if(pindex)
		*pindex = lo;

	return -1;
}



















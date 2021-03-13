/**
 *
 * @file MyInsertSort.c �����㷨��������
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#include "MyAlgorithm.h"
#include <stdlib.h>
#include <assert.h>
#include "__def_fun_def.h"
#include "mymempool.h"


/**
 *
 * @brief:����ʽ����
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:��������Ԫ�صĸ���
 * @param step_size:ָ�����ߵĿ��(1��ʾһ�ֽ�)
 * @param compare:�Ƚϻص�(����Ϊ��)
 * @param swaper:��ν�������Ԫ�ص�����(��Ϊ��)
 * @param move:����ƶ�һ����(������memmove����,��Ϊ��)
 * @param copier:��ο�������(��Ϊ��)
 * @param context:�û��Զ��������������(��Ϊ��)
 * @param swap_buf:��������������ڱ�����ʱ����,��СΪһ��Ԫ�صĴ�С����
 * @param swap_buf_size:swap_buf��ʵ�ʴ�С
 *
 */
int MyAlgInsertSort(void * buf, size_t len, size_t step_size,
					ALG_COMPARE compare, ALG_SWAP swaper, ALG_MOVE move, ALG_COPY copier,
					const void * context, void * swap_buf, size_t swap_buf_size)
{
	size_t i = 0;
	size_t k = 0;
	unsigned char * start = (unsigned char *)buf;

	unsigned char * temp = NULL;
	int need_release = 0;
	if(swap_buf && swap_buf_size >= step_size)
		temp = (unsigned char *)swap_buf;
	else
	{
		temp = (unsigned char *)MyMemPoolMalloc(NULL, step_size);
		need_release = 1;
	}

	assert(start && compare && temp);

	if(NULL == swaper)
		swaper = __def_alg_swap_;

	if(NULL == move)
		move = __def_alg_move_;

	if(NULL == copier)
		copier = __def_alg_copy_;

	for(i = 1; i < len; i ++)
	{
		unsigned char * current = (start + i * step_size);

		/* 
		* ��Ҫ��������ݱ��浽temp 
		*/
		copier(temp, current, step_size, context);

		/*
		* ���Ҫ�����ֵ�ȵ�һ��Ԫ�ػ�С(��һ��������������������С��).ֱ�������ƿ���
		*/
		if(compare(start, current, context) > 0)
		{
			move(start + step_size, start, i * step_size, context);
			copier(start, temp, step_size, context);
			continue;
		}

		/* 
		* ����˳��Ƚ�,������ʵ�λ�� 
		*/
		k = i - 1;
		while(compare(start + k * step_size, current, context) > 0)
		{
			k --;
		}

		/* 
		* �������ȷ�Ĳ���λ��
		*/
		k += 1;

		/*
		* �ҵ���kλ��Ϊ��ȷ�Ĳ���λ��,��������������1��step_size
		*/
		move(start + (k + 1) * step_size,
			start + k * step_size, (i - k) * step_size, context);

		/*
		* ����Ҫ��������ݵ�kλ��
		*/
		copier(start + k * step_size, temp, step_size, context);
	}

	if(need_release)
		MyMemPoolFree(NULL, temp);

	return 0;
}













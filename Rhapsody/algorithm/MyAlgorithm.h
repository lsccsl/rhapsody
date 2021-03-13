/**
 *
 * @file MyAlgorithm.h �����㷨��������
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __MYALGORITHM_H__
#define __MYALGORITHM_H__


#include <stdlib.h>
#include "MyfunctionDef.h"


/**
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
 */
extern int MyAlgInsertSort(void * buf, size_t len, size_t step_size,
						ALG_COMPARE compare, ALG_SWAP swaper, ALG_MOVE move, ALG_COPY copier,
						const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief ��������
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�ŵ����е�Ԫ�ظ���
 * @param step_size:ָ�����ߵĿ��(1��ʾһ�ֽ�)
 * @param context:�û��Զ��������������
 */
extern int MyAlgQuickSort(void * buf, size_t len, size_t step_size,
					   ALG_COMPARE compare, ALG_SWAP swaper, ALG_MOVE move, ALG_COPY copier,
					   const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief ��һ��������
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����������Ԫ�صĸ���
 * @param step_size:ָ�����ߵĿ��(1��ʾһ�ֽ�)
 * @param compare:�Ƚϻص�����(����Ϊ��)
 * @param copier:�����ص�����(��Ϊ��)
 * @param context:�û��Զ��������������(��Ϊ��)
 * @param swap_buf:���ɶѹ������õ�����ʱ�ռ�(��Ϊnull)
 * @param swap_buf_size:swap_buf�Ĵ�С
 */
extern int MyAlgHeapSort(void * buf, size_t len, size_t step_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief ���ɶ�
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����������Ԫ�صĸ���
 * @param step_size:ָ�����ߵĿ��(1��ʾһ�ֽ�)
 * @param compare:�Ƚϻص�����
 * @param copier:�����ص�����
 * @param context:�û��Զ��������������
 * @param swap_buf:���ɶѹ������õ�����ʱ�ռ�(��Ϊnull)
 * @param swap_buf_size:swap_buf�Ĵ�С
 */
extern int MyAlgMakeHeap(void * buf, size_t len, size_t step_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief ������
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����������Ԫ�صĸ���
 * @param step_size:ָ�����ߵĿ��(1��ʾһ�ֽ�)
 * @param compare:�Ƚϻص�����
 * @param copier:�����ص�����
 * @param context:�û��Զ��������������
 * @param swap_buf:���ɶѹ������õ�����ʱ�ռ�(��Ϊnull)
 * @param swap_buf_size:swap_buf�Ĵ�С
 */
extern int MyAlgHeapMakeAndSort(void * buf, size_t len, size_t step_size,
								ALG_COMPARE compare, ALG_COPY copier,
								const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief �Ӷ��е���һ��Ԫ��
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:������Ԫ�صĸ���
 * @param step_size:ָ�����ߵĿ��(1��ʾһ�ֽ�)
 * @param compare:�Ƚϻص�����
 * @param copier:�����ص�����
 * @param context:�û��Զ��������������
 * @param swap_buf:���ɶѹ������õ�����ʱ�ռ�(��Ϊnull)
 * @param swap_buf_size:swap_buf�Ĵ�С
 */
extern int MyAlgHeapPop(void * buf, size_t len, size_t step_size,
						ALG_COMPARE compare, ALG_COPY copier,
						const void * context, void * swap_buf, size_t swap_buf_size);

/**
 * @brief ��һ��Ԫ��������
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:�����е�Ԫ�ظ���
 * @param step_size:ÿ��Ԫ�صĴ�С
 * @param value:Ҫ�����ֵ
 * @param value_size:value�Ĵ�С
 * @param context:�û��Զ��������������
 */
extern int MyAlgHeapPush(void * buf, size_t len, size_t step_size,
						 const void * value, size_t value_size,
						 ALG_COMPARE compare, ALG_COPY copier,
						 const void * context);

/**
 * @brief ����һ����
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����Ԫ�ظ���
 * @param step_size:ÿ��Ԫ�صĴ�С
 * @param hole_index:�ն��ڵ������(0��ʾ��һ���ڵ�)
 * @param value:ֵ
 * @param value_size:ֵ�Ĵ�С
 * @param compare:�Ƚϻص�(����Ϊ��)
 * @param copier:��ο���(��Ϊ��)
 * @param context:�û��Զ��������������
 */
/*extern int MyAlgAdjustHeap(void * buf, size_t len, size_t step_size, 
						   size_t hole_index, const void * value, size_t value_size,
						   ALG_COMPARE compare, ALG_COPY copier,
						   const void * context);*/

/**
 * @brief ���һ�������Ƿ�Ϊһ����
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����Ԫ�ظ���
 * @param step_size:ÿ��Ԫ�صĴ�С
 * @param compare:�Ƚϻص�(����Ϊ��)
 * @param context:�û��Զ��������������
 */
extern int MyAlgExaminHeap(void * buf, size_t len, size_t step_size,
						   ALG_COMPARE compare,
						   const void * context);

/**
 * @brief ���һ�������Ƿ�Ϊһ����
 *
 * @param buf:Ҫ��������ݻ�������ʼ��ַ
 * @param len:Ҫ�����Ԫ�ظ���
 * @param step_size:ÿ��Ԫ�صĴ�С
 * @param compare:�Ƚϻص�(����Ϊ��)
 * @param context:�û��Զ��������������
 */
extern int MyAlgSortOK(void * buf, size_t len, size_t step_size,
					   ALG_COMPARE compare,
					   const void * context);

/**
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
 */
extern int MyBinarySearch1(const void * buf, size_t len, size_t step_size,
						  const void * key,
						  ALG_COMPARE compare, size_t * pindex,
						  const void * context);


/**
 * @brief
 *  > 0  ��ʾ key ���� data
 *  == 0 ��ʾ key ���� data
 *  < 0  ��ʾ key С�� data
 *
 * @param key:�ؼ���
 * @param key_sz:�ؼ��ֵĻ�������С
 * @param context:�û��Զ��������������
 * @param context_sz:�û��Զ�������������ݵĳ���
 */
typedef int (*BINSEARCH_COMPARE)(const void * key, unsigned int key_sz, 
								 const void * data2, 
								 const void * context, unsigned int context_sz);

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
extern int MyBinarySearch(const void * buf, unsigned int len, unsigned int step_size,
						  const void * key, unsigned int key_sz,
						  BINSEARCH_COMPARE compare, unsigned int * pindex,
						  const void * context, unsigned int context_sz);


#endif
















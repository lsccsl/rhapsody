/**
 *
 * @file MyfunctionDef.h ���ֻص�������ԭ������
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __MYFUNCTIONDEF_H__
#define __MYFUNCTIONDEF_H__


#include <stdlib.h>


/**
 * @brief
 *  > 0  ��ʾ key1 ���� key2
 *  == 0 ��ʾ key1 ���� key2
 *  < 0  ��ʾ key1 С�� key2
 *
 * @param context:�û��Զ��������������
 */
typedef int (*ALG_COMPARE)(const void * data1, const void * data2, const void * context);

/**
 *
 * ������ν�src������ݿ�����dst
 *
 * @param context:�û��Զ��������������
 *
 */
typedef int (*ALG_COPY)(void * dst,
						const void * src, const size_t size_copy,
						const void * context);

/**
 *
 * ������ν�src��ʼ��һ�����ݿ�����dst��ʼ��һ�οռ�
 *
 * @param context:�û��Զ��������������
 *
 */
typedef int (*ALG_MOVE)(void * dst, 
						const void * src, const size_t size_move,
						const void * context);

/**
 *
 * ������ν�data1��data2�����ݻ���
 *
 * @param context:�û��Զ��������������
 *
 */
typedef int (*ALG_SWAP)(void * data1,
						void * data2, const size_t data_size,
						const void * context);


#endif














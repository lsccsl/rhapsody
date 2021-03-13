/**
 *
 * @file def_fun_def.h ���ֻص�������Ĭ�϶���
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __DEF_FUN_DEF_H__
#define __DEF_FUN_DEF_H__


#include <stdlib.h>


/**
 *
 * @brief ������ν�src������ݿ�����dst
 *
 * @param context:�û��Զ��������������
 *
 */
extern int __def_alg_copy_(void * dst,
					const void * src, const size_t size_copy,
					const void * context);

/**
 *
 * @brief ������ν�src��ʼ��һ�����ݿ�����dst��ʼ��һ�οռ�
 *
 * @param context:�û��Զ��������������
 *
 */
extern int __def_alg_move_(void * dst,
					const void * src, const size_t size_move,
					const void * context);

/**
 *
 * @brief ������ν�data1��data2�����ݻ���
 *
 * @param context:�û��Զ��������������
 *
 */
extern int __def_alg_swap_(void * data1, 
					void * data2, const size_t data_size,
					const void * context);

#endif













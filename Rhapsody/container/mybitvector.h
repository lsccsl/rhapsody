/**
 * @file mybitvector.h λ���� 2008-2-11 21:34
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  lin shao chuan makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 * see the GNU General Public License  for more detail.
 */
#ifndef __MYBITVECTOR_H__
#define __MYBITVECTOR_H__


#include "mymempool.h"


/* ������� */
struct __mybitvector_t_;
typedef struct __mybitvector_t_ * HMYBITVECTOR;


/**
 * @brief ����һ��λ����
 * @param hm:�ڴ�ؾ��
 * @param vector_size:����ĳ�ʼ��С,��ʾ�ж���bit
 * @param ini_val:λ�ĳ�ʼֵ����(0:0, ����:1)
 */
extern HMYBITVECTOR mybitVectorConstruct(HMYMEMPOOL hm, const size_t vector_size, const unsigned int ini_val);

/**
 * @brief ����λ����
 */
extern void mybitVectorDestruct(HMYBITVECTOR hbv);

/**
 * @brief ����ĳһ��pos��ֵ
 * @param val:λ��ֵ 0:0 ����:1
 * @param pos:λ��,��ʾλƫ��
 */
extern int mybitVectorSetbit(HMYBITVECTOR hbv, const unsigned int val, const size_t pos);

/**
 * @brief ��ȡĳһ��pos��ֵ
 * @param pos:λ��,��ʾλƫ��
 * @return ��ָ��λ��ֵ,���ָ��λ������,�򷵻س�ʼֵ����,���û��ڴ���ʱ�����ini_val��ֵ(0:0, ����:1, ʧ��:-1)
 */
extern int mybitVectorGetbit(HMYBITVECTOR hbv, const size_t pos);

/**
 * @brief ��ȡĳһ��pos��ֵ,���ж��Ƿ�Խ�����(����һ��������ʼλ�����С��Խ��,mybitVectorSetbit�����posΪ��׼)
 * @param pos:λ��,��ʾλƫ��
 * @return ��ָ��λ��ֵ(0:0, ����:1, Խ��:-1)
 */
extern int mybitVectorGetbitAndJudgeOver(HMYBITVECTOR hbv, const size_t pos);

/**
 * @brief ����λ����Ĵ�С
 * @param vector_size:����,��ʾ�ж���bit,vector_size�Ȼ�����С,�����bitλ��Ϣ��ʧ(��Ϊini_val)
 */
extern int mybitVectorSetSize(HMYBITVECTOR hbv, const size_t vector_size);

/**
 * @brief ��ӡ������
 */
extern int mybitVectorPrint(HMYBITVECTOR hbv);


#endif
















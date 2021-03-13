/**
 *
 * @file myTTree.h T��
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __MYTTREE_H__
#define __MYTTREE_H__


#include "mymempool.h"
#include "MyfunctionDef.h"


struct __myttree_t_;
typedef struct __myttree_t_ * HMYTTREE;


/**
 *
 * @brief T������
 *
 * @param hm:�ڴ��
 * @param compare:�Ƚϻص�����
 * @param key_op:�����ؼ��ֵĹ��������뿽��
 * @param data_op:�������ݵĹ��������뿽��
 * @param underflow:T���ڵ�ؼ��ֵ�����
 * @param overflow:T���ڵ�ؼ��ֵ�����
 *
 */
extern HMYTTREE MyTTreeConstruct(HMYMEMPOOL hm, ALG_COMPARE compare, size_t underflow, size_t overflow);

/**
 * @brief T������
 */
extern void MyTTreeDestruct(HMYTTREE httree);

/**
 * @brief ��Ӽ�¼
 *
 * @param key:�ؼ���
 * @param index_info:������Ϣ
 * @param index_info_size:������Ϣ�Ĵ�С
 */
extern int MyTTreeAdd(HMYTTREE httree, const void * index_info);

/**
 * @brief ɾ����¼
 *
 * @param key:Ҫɾ���Ĺؼ���
 */
extern int MyTTreeDel(HMYTTREE httree, const void * index_info, void ** index_info_out);

/**
 * @brief ���Ҽ�¼
 *
 * @param key:Ҫ���ҵĹؼ���
 */
extern int MyTTreeSearch(HMYTTREE httree, const void * index_info, void ** index_info_out);

/**
 * ���T���ĺϷ���
 */
extern void MyTTreeExamin(HMYTTREE httree, int bprint);

/**
 * ��ȡT���еĽڵ����
 */
extern size_t MyTTreeGetCount(HMYTTREE httree);


#endif
















/**
 *
 * @file myBTree.c B��
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 */
#ifndef __MYBTREE_H__
#define __MYBTREE_H__


#include "mymempool.h"
#include "MyfunctionDef.h"


struct __mybtree_t_;
typedef struct __mybtree_t_ * HMYBTREE;


/**
 * @brief B���Ĵ���
 * @param mini_sub:��С�ӽڵ����Ŀ
 */
extern HMYBTREE MyBTreeConstruct(HMYMEMPOOL hm, ALG_COMPARE compare, size_t min_sub);

/**
 * @brief B��������
 */
extern void MyBTreeDestruct(HMYBTREE hbtree);

/**
 * @brief ��B�������һ���ڵ�
 * @param index_info:Ҫ��ӵ�������Ϣ
 * @return 0:�ɹ� ����:ʧ��
 */
extern int MyBTreeAdd(HMYBTREE hbtree, void * index_info);

/**
 * @brief ��B����ɾ��һ���ڵ�
 * @param index_info:Ҫɾ����������Ϣ
 * @param index_info_out:���ش洢��B�����������Ϣָ����û�
 * @return 0:�ɹ� ����:ʧ��
 */
extern int MyBTreeDel(HMYBTREE hbtree, void * index_info, void ** index_info_out);

/**
 * @brief ��B���в���
 * @param index_info:Ҫ���ҵ�������Ϣ
 * @param index_info_out:���ش洢��B�����������Ϣָ����û�
 * @return 0:�ɹ� ����:ʧ��
 */
extern int MyBTreeSearch(HMYBTREE hbtree, const void * index_info, void ** index_info_out);

/**
 * @brief ��ȡB���нڵ�ĸ���
 */
extern size_t MyBTreeGetCount(HMYBTREE hbtree);

/**
 * @brief �ݹ����B���еĽڵ����
 */
extern size_t MyBTreeCalCount(HMYBTREE hbtree);

/**
 * @brief ���һ��B���Ƿ�Ϸ�
 */
extern int MyBTreeExamin(HMYBTREE hbtree, int look_printf);


#endif









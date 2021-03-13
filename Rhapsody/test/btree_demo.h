/**
 * @file btree_demo.h ��ʾ����,���ʹ��b�� 2008-04-08 20:46
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
#ifndef __BTREE_DEMO_H__
#define __BTREE_DEMO_H__


#include "btree.h"


/**
 * @brief ��һ��btree�ļ�
 */
extern HBTREE_MGR BtreeDemoOpen(const char * file_name);

/**
 * @brief �ر�һ��btree���ݿ�
 */
extern void BtreeDemoClose(HBTREE_MGR hbtree);

/**
 * @brief �����ݿ��д������ߴ򿪱�,���ض�Ӧ�ñ���α�,���ڼ�¼�Ĳ�ѯ,���,ɾ������
 */
extern HBTREE_CURSOR BtreeDemoOpenTable(HBTREE_MGR hbtree, const char * tbl_name);

/**
 * @brief ���һ����¼
 */
extern void BtreeDemoInsert(HBTREE_MGR hbtree, HBTREE_CURSOR hcur, 
							const char * key, unsigned int key_sz,
							const void * data, unsigned int data_sz);

/**
 * @brief ɾ����¼
 */
extern void BtreeDemoDel(HBTREE_MGR hbtree, HBTREE_CURSOR hcur, const char * key, unsigned int key_sz);

/**
 * @brief ��ѯ��¼
 */
extern int BtreeDemoSearch(HBTREE_CURSOR hcur,
							const char * key, unsigned int key_sz,
							void ** data, unsigned int * data_sz);

/**
 * @brief �ύ����
 */
extern int BtreeDemoCommit(HBTREE_MGR hbtree);

#endif





















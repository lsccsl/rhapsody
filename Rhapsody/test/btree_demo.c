/**
 * @file btree_demo.c ��ʾ����,���ʹ��b�� 2008-04-08 20:46
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
#include "btree_demo.h"

#include <string.h>


/**
 * @brief �����Ƚϻص�����,���Ը���Щ����,ʵ���Զ���ı����Ƚ�
 */
static int xcompare_master(const void * k1, const unsigned int k1_sz,
						   const void * k2, const unsigned int k2_sz,
						   const void * context, const unsigned int context_sz)
{	
	return strncmp(k1, k2, k1_sz > k2_sz ? k1_sz : k2_sz);
}

/**
 * @brief �ؼ��ֱȽϻص�����,���Ը��Ĵ˺���,ʵ���Զ���Ĺؼ��ֱȽ�
 */
static int xcompare(const void * k1, const unsigned int k1_sz,
					const void * k2, const unsigned int k2_sz,
					const void * context, const unsigned int context_sz)
{	
	return strncmp(k1, k2, k1_sz > k2_sz ? k1_sz : k2_sz);
}

/**
 * @brief ��һ��btree�ļ�
 */
HBTREE_MGR BtreeDemoOpen(const char * file_name)
{
	HBTREE_MGR hbtree_mgr = NULL;

	if(NULL == file_name)
		return NULL;

	hbtree_mgr = btreeMgrConstruct(NULL,
		file_name,
		1024,
		256,
		512,
		NULL, 0);

	return hbtree_mgr;
}

/**
 * @brief �ر�һ��btree���ݿ�
 */
void BtreeDemoClose(HBTREE_MGR hbtree)
{
	btreeMgrDestruct(hbtree);
}

/**
 * @brief �����ݿ��д�����,���ض�Ӧ�ñ���α�,���ڼ�¼�Ĳ�ѯ,���,ɾ������
 */
HBTREE_CURSOR BtreeDemoOpenTable(HBTREE_MGR hbtree, const char * tbl_name)
{
	HBTREE_CURSOR hcur = NULL;
	HBTREE_CURSOR hcur_master = NULL;

	btreeMgrOpenMaster(hbtree,
		&hcur_master,
		xcompare_master,
		NULL, 0);

	if(NULL == hcur_master)
		return NULL;
	
	btreeMgrCreateTable(hcur_master,
		&hcur,
		xcompare, NULL, 0,
		tbl_name, (unsigned int)(strlen(tbl_name) + 1),
		tbl_name, (unsigned int)(strlen(tbl_name) + 1),
		e_btree_page_flag_hasdata);

	if(NULL == hcur)
	{
		btreeMgrOpenTable(hcur_master, &hcur, xcompare, NULL, 0,
			tbl_name, (unsigned int)(strlen(tbl_name) + 1), NULL);
	}

	btreeMgrCommit(hbtree);

	btreeMgrCloseMaster(hcur_master);

	return hcur;
}

/**
 * @brief ���һ����¼
 */
void BtreeDemoInsert(HBTREE_MGR hbtree, HBTREE_CURSOR hcur,
					 const char * key, unsigned int key_sz,
					 const void * data, unsigned int data_sz)
{
	btreeMgrAdd(hcur, key, key_sz, data, data_sz);
	//btreeMgrCommit(hbtree);
}

/**
 * @brief ɾ����¼
 */
void BtreeDemoDel(HBTREE_MGR hbtree, HBTREE_CURSOR hcur, const char * key, unsigned int key_sz)
{
	btreeMgrDel(hcur, key, key_sz);
	//btreeMgrCommit(hbtree);
}

/**
 * @brief ��ѯ��¼
 */
int BtreeDemoSearch(HBTREE_CURSOR hcur,
					 const char * key, unsigned int key_sz,
					 void ** data, unsigned int * data_sz)
{
	if(NULL == data || NULL == data_sz)
		return -1;

	if(0 != btreeMgrSearch(hcur, key, key_sz))
		return -1;

	btreeMgrGetData(hcur, data, data_sz, NULL);

	return 0;
}

/**
 * @brief �ύ����
 */
int BtreeDemoCommit(HBTREE_MGR hbtree)
{
	return btreeMgrCommit(hbtree);
}
















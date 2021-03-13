/**
 * @file btree.h ����b���㷨 2008-03-03 23:26
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.
 *        ����b���㷨,����ҳ�������pager�����Ϲ���
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
#ifndef __BTREE_H__
#define __BTREE_H__


#include "mymempool.h"
#include "mybuffer.h"
#include "myobj.h"


/* b������ */
struct __btree_mgr_t_;
typedef struct __btree_mgr_t_ * HBTREE_MGR;

/* �α������� */
struct __btree_cursor_;
typedef struct __btree_cursor_ * HBTREE_CURSOR;

/**
 *  > 0  ��ʾ k1 ���� k2
 *  == 0 ��ʾ k1 ���� k2
 *  < 0  ��ʾ k1 С�� k2
 * 
 * @brief �ؼ��ֱȽϻص�����
 * @param k1:�ؼ���1
 * @param k1_sz:k1�������Ĵ�С
 * @param k2:�ؼ���2
 * @param k2_sz:k2�������Ĵ�С
 * @param context:�ȽϹ����еĻص�������
 */
typedef int (*XCOMPARE)(const void * k1, const unsigned int k1_sz, 
						const void * k2, const unsigned int k2_sz, 
						const void * context, const unsigned int context_sz);


/**
 * @brief ����btree
 * @param pg_sz:btree �� page size,��Ϊ0
 * @param cache_pg_count:btreeҳ�������ֵ,��Ϊ0;
 * @param sector_sz:OS�����Ĵ�С,��Ϊ0
 * @param rand_seed:�������ʼ������
 * @param rand_seed_sz:rand_seed��ָ�������Ĵ�С
 * @param user_rate:ҳ�ļ�ʹ����,��Ϊ0
 */
extern HBTREE_MGR btreeMgrConstruct(HMYMEMPOOL hm,
									const char * page_file_name,
									unsigned int pg_sz,
									unsigned int cache_pg_count,
									unsigned int sector_sz,
									void * rand_seed,
									unsigned int rand_seed_sz);

/**
 * @brief ����btree
 */
extern int btreeMgrDestruct(HBTREE_MGR hbtreeMgr);

/**
 * @brief �ͷ��α� ��btreeMgrGetCursor�Ƕ�ż����
 */
extern int btreeMgrReleaseCursor(HBTREE_CURSOR hcur);

/**
 * @brief ��Ӽ�ֵ
 */
extern int btreeMgrAdd(HBTREE_CURSOR hcur,
					   const void * key, const unsigned int key_sz,
					   const void * data, const unsigned int data_sz);

/**
 * @brief ɾ����ֵ
 */
extern int btreeMgrDel(HBTREE_CURSOR hcur,
					   const void * key,
					   const unsigned int key_sz);

/**
 * @brief ���Ҽ�ֵ
 */
extern int btreeMgrSearch(HBTREE_CURSOR hcur,
						  const void * key,
						  const unsigned int key_sz);

/**
 * @brief ��ȡ�α굱ǰ���ڼ�¼��key
 */
extern int btreeMgrGetKey(HBTREE_CURSOR hcur, void ** pkey, unsigned int * pkey_sz, HMYMEMPOOL hm);

/**
 * @brief ��ȡ�α굱ǰ���ڼ�¼��data
 */
extern int btreeMgrGetData(HBTREE_CURSOR hcur, void ** pdata, unsigned int * pdata_sz, HMYMEMPOOL hm);

/**
 * @brief ���α���ָ�ı�������м�¼ȫ��ɾ��
 */
extern int btreeMgrClear(HBTREE_CURSOR hcur);

/**
 * @brief ��ȡ�������α�,������¼����b��������������b���ĸ��ڵ�
 */
extern int btreeMgrOpenMaster(HBTREE_MGR hbtreeMgr, 
							  HBTREE_CURSOR * phcur,
							  XCOMPARE cmp,
							  const void * context, unsigned int context_sz);
#define btreeMgrCloseMaster(_hcur) btreeMgrReleaseCursor(_hcur)


/* ����btree��ҳ��ʶ */
enum BTREE_PAGE_FLAG
{
	/* ҳ�Ƿ�ΪҶ�ڵ� */
	e_btree_page_flag_leaf    = 0x01,

	/* ҳ�Ƿ���int����Ϊ�ؼ��� */
	e_btree_page_flag_intkey  = 0x02,

	/* ҳ�Ƿ��������� */
	e_btree_page_flag_hasdata = 0x04,
};
/**
 * @brief ����һ����
 * @param hcur:����Ϊ��,�����ɹ�����hcur��root_pg��ֵ
 */
extern int btreeMgrCreateTable(HBTREE_CURSOR hcur_master,
							   HBTREE_CURSOR * phcur,
							   XCOMPARE cmp, const void * context, unsigned int context_sz,
							   const void * table_id, const unsigned int table_id_sz,
							   const void * table_info, const unsigned int table_info_sz,
							   unsigned char flag);

/**
 * @brief ��һ����
 * @param hcur:����Ϊ��,�򿪳ɹ�����hcur��root_pg��ֵ
 */
extern int btreeMgrOpenTable(HBTREE_CURSOR hcur_master,
							 HBTREE_CURSOR * phcur,
							 XCOMPARE cmp, const void * context, unsigned int context_sz,
							 const void * table_id, const unsigned int table_id_sz,
							 HMYBUFFER hb_tbl_info);

/**
 * @brief ɾ��һ����
 */
extern int btreeMgrDropTable(HBTREE_CURSOR hcur_master,
							 const void * table_id, const unsigned int table_id_sz);

/**
 * @brief ��ȡһ�������rowid
 */
extern int btreeMgrTableGetRowid(HBTREE_CURSOR hcur,
								 unsigned int * prowid);

/**
 * @brief �ύ����
 */
extern int btreeMgrCommit(HBTREE_MGR hbtreeMgr);

/**
 * @brief �ع�
 */
extern int btreeMgrRollBack(HBTREE_MGR hbtreeMgr);

/**
 * @brief ��ȡһ��b���ļ�¼����
 */
extern unsigned int btreeMgrGetCount(HBTREE_CURSOR hcur);

/**
 * @brief ��ȡ���ҳ�ļ��ж���ҳ,�Լ��ж��ٿ���ҳ
 */
typedef struct __pager_info_t_
{
	unsigned int total_page_count;
	unsigned int free_page_count;
}pager_info_t;
extern int btreeMgrGetPagerInfo(HBTREE_MGR hbtreeMgr, pager_info_t * pager_info);

/**
 * @brief ��ȡҳ�������ü���
 */
extern int btreeMgrGetPagerRefCount(HBTREE_MGR hbtreeMgr);

/**
 * @brief ���һ��b���Ƿ�Ϸ�
 */
extern int ExaminBtree(HBTREE_CURSOR hcur, unsigned int * node_count, int need_exam_pager);


#endif

























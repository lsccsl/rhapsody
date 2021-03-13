/**
 * @file btree.h 描述b树算法 2008-03-03 23:26
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.
 *        描述b树算法,基于页缓存机制pager基础上构建
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


/* b树定义 */
struct __btree_mgr_t_;
typedef struct __btree_mgr_t_ * HBTREE_MGR;

/* 游标句柄定义 */
struct __btree_cursor_;
typedef struct __btree_cursor_ * HBTREE_CURSOR;

/**
 *  > 0  表示 k1 大于 k2
 *  == 0 表示 k1 等于 k2
 *  < 0  表示 k1 小于 k2
 * 
 * @brief 关键字比较回调函数
 * @param k1:关键字1
 * @param k1_sz:k1缓冲区的大小
 * @param k2:关键字2
 * @param k2_sz:k2缓冲区的大小
 * @param context:比较过程中的回调上下文
 */
typedef int (*XCOMPARE)(const void * k1, const unsigned int k1_sz, 
						const void * k2, const unsigned int k2_sz, 
						const void * context, const unsigned int context_sz);


/**
 * @brief 创建btree
 * @param pg_sz:btree 的 page size,可为0
 * @param cache_pg_count:btree页缓存最大值,可为0;
 * @param sector_sz:OS扇区的大小,可为0
 * @param rand_seed:随机数初始化种子
 * @param rand_seed_sz:rand_seed所指缓冲区的大小
 * @param user_rate:页文件使用率,可为0
 */
extern HBTREE_MGR btreeMgrConstruct(HMYMEMPOOL hm,
									const char * page_file_name,
									unsigned int pg_sz,
									unsigned int cache_pg_count,
									unsigned int sector_sz,
									void * rand_seed,
									unsigned int rand_seed_sz);

/**
 * @brief 销毁btree
 */
extern int btreeMgrDestruct(HBTREE_MGR hbtreeMgr);

/**
 * @brief 释放游标 与btreeMgrGetCursor是对偶操作
 */
extern int btreeMgrReleaseCursor(HBTREE_CURSOR hcur);

/**
 * @brief 添加键值
 */
extern int btreeMgrAdd(HBTREE_CURSOR hcur,
					   const void * key, const unsigned int key_sz,
					   const void * data, const unsigned int data_sz);

/**
 * @brief 删除键值
 */
extern int btreeMgrDel(HBTREE_CURSOR hcur,
					   const void * key,
					   const unsigned int key_sz);

/**
 * @brief 查找键值
 */
extern int btreeMgrSearch(HBTREE_CURSOR hcur,
						  const void * key,
						  const unsigned int key_sz);

/**
 * @brief 获取游标当前所在记录的key
 */
extern int btreeMgrGetKey(HBTREE_CURSOR hcur, void ** pkey, unsigned int * pkey_sz, HMYMEMPOOL hm);

/**
 * @brief 获取游标当前所在记录的data
 */
extern int btreeMgrGetData(HBTREE_CURSOR hcur, void ** pdata, unsigned int * pdata_sz, HMYMEMPOOL hm);

/**
 * @brief 将游标所指的表里的所有记录全部删除
 */
extern int btreeMgrClear(HBTREE_CURSOR hcur);

/**
 * @brief 获取主树的游标,主树记录所有b树管理器的其它b树的根节点
 */
extern int btreeMgrOpenMaster(HBTREE_MGR hbtreeMgr, 
							  HBTREE_CURSOR * phcur,
							  XCOMPARE cmp,
							  const void * context, unsigned int context_sz);
#define btreeMgrCloseMaster(_hcur) btreeMgrReleaseCursor(_hcur)


/* 描述btree的页标识 */
enum BTREE_PAGE_FLAG
{
	/* 页是否为叶节点 */
	e_btree_page_flag_leaf    = 0x01,

	/* 页是否以int类型为关键字 */
	e_btree_page_flag_intkey  = 0x02,

	/* 页是否含有数据项 */
	e_btree_page_flag_hasdata = 0x04,
};
/**
 * @brief 创建一个表
 * @param hcur:不可为空,创建成功将给hcur的root_pg赋值
 */
extern int btreeMgrCreateTable(HBTREE_CURSOR hcur_master,
							   HBTREE_CURSOR * phcur,
							   XCOMPARE cmp, const void * context, unsigned int context_sz,
							   const void * table_id, const unsigned int table_id_sz,
							   const void * table_info, const unsigned int table_info_sz,
							   unsigned char flag);

/**
 * @brief 打开一个表
 * @param hcur:不可为空,打开成功将给hcur的root_pg赋值
 */
extern int btreeMgrOpenTable(HBTREE_CURSOR hcur_master,
							 HBTREE_CURSOR * phcur,
							 XCOMPARE cmp, const void * context, unsigned int context_sz,
							 const void * table_id, const unsigned int table_id_sz,
							 HMYBUFFER hb_tbl_info);

/**
 * @brief 删除一个表
 */
extern int btreeMgrDropTable(HBTREE_CURSOR hcur_master,
							 const void * table_id, const unsigned int table_id_sz);

/**
 * @brief 获取一个表的新rowid
 */
extern int btreeMgrTableGetRowid(HBTREE_CURSOR hcur,
								 unsigned int * prowid);

/**
 * @brief 提交更改
 */
extern int btreeMgrCommit(HBTREE_MGR hbtreeMgr);

/**
 * @brief 回滚
 */
extern int btreeMgrRollBack(HBTREE_MGR hbtreeMgr);

/**
 * @brief 获取一棵b树的记录总数
 */
extern unsigned int btreeMgrGetCount(HBTREE_CURSOR hcur);

/**
 * @brief 获取外存页文件有多少页,以及有多少空闲页
 */
typedef struct __pager_info_t_
{
	unsigned int total_page_count;
	unsigned int free_page_count;
}pager_info_t;
extern int btreeMgrGetPagerInfo(HBTREE_MGR hbtreeMgr, pager_info_t * pager_info);

/**
 * @brief 获取页缓存引用计数
 */
extern int btreeMgrGetPagerRefCount(HBTREE_MGR hbtreeMgr);

/**
 * @brief 检查一棵b树是否合法
 */
extern int ExaminBtree(HBTREE_CURSOR hcur, unsigned int * node_count, int need_exam_pager);


#endif

























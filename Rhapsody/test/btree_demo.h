/**
 * @file btree_demo.h 演示程序,如何使用b树 2008-04-08 20:46
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
 * @brief 打开一个btree文件
 */
extern HBTREE_MGR BtreeDemoOpen(const char * file_name);

/**
 * @brief 关闭一个btree数据库
 */
extern void BtreeDemoClose(HBTREE_MGR hbtree);

/**
 * @brief 在数据库中创建或者打开表,返回对应该表的游标,用于记录的查询,添加,删除操作
 */
extern HBTREE_CURSOR BtreeDemoOpenTable(HBTREE_MGR hbtree, const char * tbl_name);

/**
 * @brief 添加一条记录
 */
extern void BtreeDemoInsert(HBTREE_MGR hbtree, HBTREE_CURSOR hcur, 
							const char * key, unsigned int key_sz,
							const void * data, unsigned int data_sz);

/**
 * @brief 删除记录
 */
extern void BtreeDemoDel(HBTREE_MGR hbtree, HBTREE_CURSOR hcur, const char * key, unsigned int key_sz);

/**
 * @brief 查询记录
 */
extern int BtreeDemoSearch(HBTREE_CURSOR hcur,
							const char * key, unsigned int key_sz,
							void ** data, unsigned int * data_sz);

/**
 * @brief 提交更改
 */
extern int BtreeDemoCommit(HBTREE_MGR hbtree);

#endif





















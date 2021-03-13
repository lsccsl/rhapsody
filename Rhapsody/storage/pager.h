/**
 * @file page.h 外存页缓存管理 2008-1-29 23:59
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.
 *        描述辅存页缓存管理的接口,并通过备份的机制实现数据的完整性.
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
#ifndef __PAGER_H__
#define __PAGER_H__


#include "mymempool.h"

/* 页管理器句柄 */
struct __pager_t_;
typedef struct __pager_t_ * HPAGER;

/* 页头信息句柄 */
struct __pghd_t_;
typedef struct __pghd_t_ * HPAGE_HEAD;

/* 页析构回调函数 */
typedef int(*PAGE_RELEASE)(HPAGE_HEAD pg);

/* 页重新载入回调函数 */
typedef int(*PAGE_RELOAD)(HPAGE_HEAD pg);

/* 页被移动时回调函数 */
typedef int(*PAGE_MOVE)(HPAGE_HEAD pg, unsigned int old_pgno, unsigned int new_pgno);


/**
 * @brief 创建一个缓存页管理器
 * @param hm:内存池句柄
 * @param file_name:辅存的文件名
 * @param page_release_cb:页析构回调函数
 * @param page_reload_cb:页重载回调函数
 * @param extra_size:每页附加的扩展用户数据大小
 * @param max_page:缓存中的最大页数
 * @param page_size:每一页的大小,如果对应的页文件已存在,则返回页的大小,可为空,默认为1024
 * @param used_rate:外存文件使用率, 0:表示不需要进行栽减, 1-9分别表示使用率,一旦低于使用率,则进行自动栽减,>=10表示总是要栽减
 * @param rand_seed:随机数初始化种子
 * @param rand_seed_size:种子的长度
 * @param sector_size:所用系统的扇区大小
 */
extern HPAGER PagerConstruct(HMYMEMPOOL hm, 
							 const char * file_name, 
							 PAGE_RELEASE page_release_cb,
							 PAGE_RELOAD page_reload_cb,
							 unsigned int extra_size,
							 unsigned int max_cache_pages,
							 unsigned int * page_size,
							 void * rand_seed,
							 unsigned int rand_seed_size,
							 unsigned int sector_size);

/**
 * @brief 销毁一个缓存页管理器
 */
extern void PagerDestruct(HPAGER hpgr);

/**
 * @brief 同步所有脏缓存页
 */
extern int PagerSyn(HPAGER hpgr);

/**
 * @brief 取消所有的页的更改
 */
extern int PagerRollBack(HPAGER hpgr);

/**
 * @brief 申请一个页号
 * @return 返回一个可用的空闲页号
 */
extern unsigned int PagerGetPageNo(HPAGER hpgr);

/**
 * @brief 释放一个页号,将置成空闲页
 */
extern int PagerReleasePageNo(HPAGER hpgr, unsigned int pgno);

/**
 * @brief 获取当前页文件中有多少页
 */
extern unsigned int PagerGetTotalPagesCount(HPAGER hpgr);

/**
 * @brief 获取当前页文件中有多少空闲页
 */
extern unsigned int PagerGetFreePages(HPAGER hpgr);

/**
 * @brief 从页缓存管理中获取一页 与PagerReleasePage是对偶的操作
 */
extern HPAGE_HEAD PagerGetPage(HPAGER hpgr, unsigned int pgno);

/**
 * @brief 对一个页缓存引用计数增加,需要用PagerReleasePage降低引用引数
 */
extern int PageHeadRef(HPAGE_HEAD pg);

/**
 * @brief 从缓存里引用指定页号的页缓存,如该页不在缓存里,返回NULL
 */
extern HPAGE_HEAD PagerGetCachedPage(HPAGER hpgr, unsigned int pgno);

/**
 * @brief 取消对当前页的引用 与PagerGetPage是对偶的操作
 */
extern int PagerReleasePage(HPAGE_HEAD pg);

/**
 * @brief 获取页内容的缓冲区,用于写入
 */
extern void * PageHeadMakeWritable(HPAGE_HEAD pg);

/**
 * @brief 获取某一个页内容,用于读取
 */
extern const void * PageHeadMakeReadable(HPAGE_HEAD pg);

/**
 * @brief 获取某一页的用户数据
 */
extern void * PageHeadGetUserData(HPAGE_HEAD pg);

/**
 * @brief 获取某一页的用户数据的长度
 */
extern unsigned PageHeadGetUserDataSize(HPAGE_HEAD pg);

/**
 * @brief 获取页缓存管理器的引用计数
 */
extern unsigned int PagerGetRefCount(HPAGER hpgr);

/**
 * @brief 获取某一页缓存的引用计数
 */
extern unsigned int PagerGetPageRefCount(HPAGE_HEAD pg);

/**
 * @brief 检查某个指定是否越界了
 */
extern int PagePtrIsInpage(HPAGE_HEAD pg, const void * ptr);

/**
 * @brief 检查页缓存的状态
 */
extern void PagerExamin(HPAGER hpgr, int check_jf, int check_free, void * buf, unsigned int buf_sz);


#endif




















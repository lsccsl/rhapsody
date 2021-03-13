/**
 * @file page.c page cache management 2008-1-29 23:59
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it. 
 *        负责缓存page管理,journal机制保证数据的完整性, 外存空闲页管理
 *
 * 外存页文件格式描述
 * 第一页
 *  offset bytes        des
 *  0      16           格式版本
 *  16     4            页大小
 *  20     4            第一个空闲页
 *  24     40           保留
 *  64   pg_sz - 64     用户数据     
 *
 * 空闲页:
 * 第一个空闲页
 * offset bytes   des
 * 0      4       总的空闲页数
 * 4      4       空闲页"链表"的下一个空闲页号
 * 8      4       当前页里记载了多少的空闲页号
 * 12             空闲数组开始,每4个字节表示一个空闲页号
 * 非第一个空闲页
 * 0      4       无意义
 * 4      4       空闲页"链表"的下一个空闲页号
 * 8      4       当前页里记载了多少的空闲页号
 * 12             空闲数组开始,每4个字节表示一个空闲页号
 *
 * 页:
 * offset bytes      des
 * 0      1          1:表示此页被使用,0:表示此页空闲
 * 1      3          保留
 * 4      pg_sz - 4  供用户使用的空间
 *
 * journal日志文件头定义
 * offset bytes          des
 * 0      4              头的大小hd_sz
 * 4      16             格式版本
 * 20     4              日志里备份了几页
 * 24     4              校验和初始随机值
 * 28     4              备份日志前页文件的大小
 * 32     hd_sz-24       保留
 * 
 * journal记录:
 * <页内容 + 页号 + 校验和>
 *
 * 验证:os是否是按块写文件,sync时是否安脏块同步,
 * seek:win的seek一般会成功,即使是seek的位置超过了文件的大小,在写入时,如果磁盘空间不够(seek的位置太大了),则会提示磁盘空间不足(errcode = 112)
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
#include "pager.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mybuffer.h"
#include "myhashtable.h"
#include "mylog.h"
#include "myrand.h"
#include "MyAlgorithm.h"

#include "OsFile.h"


#ifdef WIN32
#pragma warning(disable:4312)
#endif


/* 默认的日志文件附加 */
#define JOURNAL "-journal"

/* 页文件格式版本 */
#define VERSION "Rhapsody V0.1.7\000"

/* 最小以及最大的页文件大小 */
#define	MIN_PAGE_SIZE 64
#define MAX_PAGE_SIZE 32768

/* 默认的页缓存上限 */
#define DEFAULT_CACHE_PAGES_COUNT 8

/* 日志版本长度 */
#define JOURNAL_MAGIC_STRING "1234567890123456"

/* 页文件的第一页 */
#define PAGER_FIRST_PAGE_NO 1


/**
 * 外存页文件格式描述
 * 第一页
 *  offset bytes        des
 *  0      16           格式版本
 *  16     4            页大小
 *  20     4            第一个空闲页
 *  24     40           保留
 *  64   pg_sz - 64     用户数据
 */
typedef struct __unused_pager_info_
{
	unsigned char ver[16];
	unsigned char page_size[4];
	unsigned char first_free[4];
	unsigned char reserve[40];
}unused_pager_info;

/* 记录ver的偏移 */
#define PAGER_HDR_VER_OFFSET (((unused_pager_info *)0)->ver - (unsigned char *)0)
/* 记录ver的大小 */
#define PAGER_HDR_VER_LEN (sizeof(((unused_pager_info *)0)->ver))
/* page_size的偏移 */
#define PAGER_HDR_PAGE_SIZE_OFFSET (((unused_pager_info *)0)->page_size - (unsigned char *)0)
/* 记录第一个空闲页页号的偏移量 */
#define PAGER_HDR_FIRST_FREE_OFFSET (((unused_pager_info *)0)->first_free - (unsigned char *)0)
/* 记录第一个空闲页页号的外存空间长度 */
#define PAGER_HDR_FIRST_FREE_LEN (sizeof(((unused_pager_info *)0)->first_free))

/**
 * 判断这个主空闲页是否包含有子空闲页
 *
 * 空闲页:
 * 第一个空闲页
 * offset bytes   des
 * 0      4       总的空闲页数
 * 4      4       空闲页"链表"的下一个空闲页号
 * 8      4       当前页里记载了多少的空闲页号
 * 12             空闲数组开始,每4个字节表示一个空闲页号
 * 非第一个空闲页
 * 0      4       无意义
 * 4      4       空闲页"链表"的下一个空闲页号
 * 8      4       当前页里记载了多少的空闲页号
 * 12             空闲数组开始,每4个字节表示一个空闲页号
 */
typedef struct __unused_free_page_
{
	unsigned char total_free_count[4];
	unsigned char next_free_link[4];
	unsigned char free_count[4];
	unsigned char free_pgno_array[1];
}unused_free_page;

/* 存储总页数的偏移 */
#define PAGER_FREE_TOTAL_FREE_COUNT_OFFSET (((unused_free_page *)NULL)->total_free_count - (unsigned char *)NULL)
/* 下一个空闲页数组链接的偏移 */
#define PAGER_FREE_NEXT_FREE_LINK_OFFSET (((unused_free_page *)NULL)->next_free_link - (unsigned char *)NULL)
/* 存储本页空闲页数的偏移 */
#define PAGER_FREE_FREE_COUNT_OFFSET (((unused_free_page *)NULL)->free_count - (unsigned char *)NULL)
/* 空闲页数组的偏移 */
#define PAGER_FREE_FREE_PGNO_ARRAY_OFFSET (((unused_free_page *)NULL)->free_pgno_array - (unsigned char *)NULL)

/**
 * 每页用于管理的空间偏移定义
 * 0      1          1:表示此页被使用,0:表示此页空闲
 * 1      3          保留
 * 4      pg_sz - 4  供用户使用的空间
 */
typedef struct __unused_page_
{
	unsigned char in_user[1];
	unsigned char reserve[3];
}unused_page;

/* 每一页为了管理而预留的字节 */
#define BYTE_RESERVE_PER_PAGE sizeof(unused_page)
#define PAGER_PAGE_USER_SIZE(__pgr) (__pgr->page_size - BYTE_RESERVE_PER_PAGE)
/* 标识页处于使用中的变量位置偏移 */
#define PAGER_PAGE_IN_USE_OFFSET (((unused_page*)NULL)->in_user - (unsigned char *)NULL)

/**
 * journal日志文件头定义
 * offset bytes          des
 * 0      4              头的大小hd_sz
 * 4      16             格式版本
 * 20     4              日志里备份了几页
 * 24     4              校验和初始随机值
 * 28     4              备份日志前页文件的大小
 * 32     hd_sz-24       保留
 */
typedef struct __unused_journal_hdr_
{
	unsigned char sector_size[4];
	unsigned char magic[16];
	unsigned char rec_count[4];
	unsigned char cksum_init[4];
	unsigned char orig_page_count[4];
	unsigned char reserve[32];
}unused_journal_hdr;

/* 默认的扇区大小 */
#define PAGER_MIN_SECTOR_SIZE (sizeof(unused_journal_hdr))
#define JOURNAL_MAGIC_LEN (sizeof(((unused_journal_hdr *)0)->magic))
#define PAGER_JOURNAL_REC_COUNT_OFFSET (((unused_journal_hdr *)0)->rec_count - (unsigned char *)0)
#define PAGER_JOURNAL_REC_COUNT_LEN (sizeof(((unused_journal_hdr *)0)->rec_count))


struct __pghd_t_;

typedef struct __pager_t_
{
	/* 内存池句柄 */
	HMYMEMPOOL hm;

	/* 辅存文件名 */
	HMYBUFFER hb_file_name;
	/* 辅存的页文件句柄 */
	HOSFILE hf;
	/* 页文件的数据是否完整 */
	int page_file_is_integrity;
	/* 这个pager缓存管理被引用的次数 */
	unsigned int ref_count;

	/* journal文件名 */
	HMYBUFFER hb_jf_name;
	/* journal 文件句柄 */
	HOSFILE hjf;
	/* 当前日志文件的尾部 */
	int64 journal_end;
	/* 当前日志文件的最后一个头偏移(还未同步) */
	int64 jouranl_prev_hdr;
	/*
	* 校验和初始化的随机数
	* 每一个备份日志头,都记录一个随机数,用于计算对应于这个头的页的检验和
	* 防止因为断电出现垃圾数据
	* (垃圾数据有可能是一个被删除的正确的journal文件,这样,即使文件是错误的,但校验和却有可能是正确的)
	* 加上这个随机因子,降低这种情况带来的风险
	* 每个jf头的校验和初始随机值都是不一样的.
	*/
	unsigned int cur_cksum_init;
	/* 随机数发生器 */
	HMYRAND hrand;

	/* 记录空闲页链表,回收此链表里的页时,首先同步jfd,以保证数据的完整性 */
	struct __pghd_t_ * pg_free_first, * pg_free_last;

	/*
	* 记录已经同步过jf的空闲页链表,回收时,优先回收这个链表里的页 
	* 当获取空头页缓存时,如果该页处在这个链表里头,则应从链表里头将此页脱链
	*/
	struct __pghd_t_ * pg_syncjf_first, * pg_syncjf_last;

	/* 是否需要做了jfd同步的操作,重置时需要此标识置成0 */
	int need_sync_journal;
	/*
	* 是否已经做过jf同步操作
	* 用于判断当出现超出原始大小页写入情况时,是否需要做同步jf的操作
	* 如果同步jf的操作已经做过了,则此时不用再做了
	*/
	int bsync_journal_has_done;

	/* 记录有多少页进入jfd备份,即尚未同步jf的脏页 */
	unsigned int in_journal_pages_count;

	/* 记录页是否处journal状态的位着色表 */
	HMYHASHTABLE hash_pgno_journal;

	/* 记录所有的页 */
	struct __pghd_t_ * pg_all;

	/* 记录脏页,同步时,将这些页先同步至jfd,然后写入fd并同步 */
	struct __pghd_t_ * pg_dirty;

	/* 文件页号与页面缓存的哈希映射表 <pgno> - <pghd_t *> */
	HMYHASHTABLE hash_page;

	/* 记录外存文件有多少页 */
	unsigned int total_page_count;
	/* 记录开始备份jf时页文件有多少页 */
	unsigned int orig_pages_count;

	/* 缓存中的最大页数 */
	unsigned int max_cache_pages;

	/* 每页大小 */
	unsigned int page_size;

	/*
	* 所用系统的扇区大小,用于jf头的大小
	* 如果做过回滚操作,这个值将被修正
	*/
	unsigned int sector_size;

	/* 每页附加的用户扩展数据大小 */
	unsigned int extra_size;

	/* 页的加载与取消引用以及移动时的回调函数 */
	PAGE_RELEASE page_release_cb;
	PAGE_RELOAD page_reload_cb;
}pager_t;

typedef struct __pghd_t_
{
	/* which pager this page belong to */
	pager_t * pager;

	/* 页号 */
	unsigned int page_num;

	/* 页缓存引用记数 */
	unsigned int ref_count;

	/* 记录当前页缓存的状态 */
	int pg_state;

	/* 前一个与后一个空闲页,当页的引用计数为0时,应加入这个链表 */
	struct __pghd_t_ * pg_free_prev, * pg_free_next;

	/*
	* 记录已经同步过jf的空闲页链表,回收时,优先回收这个链表里的页 
	* 当获取空头页缓存时,如果该页处在这个链表里头,则应从链表里头将此页脱链
	* 当页的引用计数为零时,并且已经同步了hjf,应加入此链表
	*/
	struct __pghd_t_ * pg_syncjf_prev, * pg_syncjf_next;

	/* 后一个脏页,构成一个脏页缓冲链表 */
	struct __pghd_t_ * pg_dirty_prev, * pg_dirty_next;

	/* 后一个页,构成一个所有页的链表 */
	struct __pghd_t_ * pg_all_prev, * pg_all_next;

	/*
	* 用于页管理而预留的一些字节
	* 第一个字节表示该页是否为空闲页
	*/
	unsigned char pg_buf_start[BYTE_RESERVE_PER_PAGE];
}pghd_t;

typedef enum __page_event_
{
	/* 更改了页缓存,并在jf里做备份 */
	PAGE_EVT_WRITE_BUF_BK,

	/* 页缓存被写回,并同步了jf */
	PAGE_EVT_WRITE_BACK_SYNCJF,

	/* 页缓存被同步了 */
	PAGE_EVT_SYNC,

	/* 页缓存回滚了 */
	PAGE_EVT_ROLL_BACK,

	/* jf文件被同步了 */
	PAGE_EVT_SYNC_JOURNAL,

	/* jf文件被删除 */
	PAGE_EVT_DEL_JOURNAL,

	EVENT_END,
}PAGE_EVENT;

typedef enum __page_state_
{
	/* 页缓存被读出,但没有对页缓存做任何修改 */
	PAGE_CLEAN,

	/* 对页缓存做了修改,并在jf做了备份,但没有写入页文件 */
	PAGE_DIRTY_NO_SYNC_JF,

	/* 对页缓存的做改已经写回页文件,在jf里备份已经做了 */
	PAGE_CLEAN_SYNC,

	/* 页缓存写加页文件后,又对页缓存进行了修改,jf里的备份已经做了 */
	PAGE_DIRTY_SYNC_JF,

	/* 页处于一个错误的状态 */
	PAGE_STATE_ERR,
}PAGE_STATE;

/**
 * @brief 将某一个页缓存置成非dirty状态,并从dirty链表中脱离
 *
 * 页缓存状态变迁表
 *---------------------------------------------------------------------------------------------------------------------------------------------------------------------
 *| 状态\事件            |PAGE_EVT_WRITE_BUF_BK  |PAGE_EVT_WRITE_BACK_SYNCJF | PAGE_EVT_SYNC       | PAGE_EVT_ROLL_BACK | PAGE_EVT_SYNC_JOURNAL| PAGE_EVT_DEL_JOURNAL |
 *|----------------------|-----------------------|---------------------------|---------------------|--------------------|----------------------|----------------------|
 *| PAGE_CLEAN           |PAGE_DIRTY_NO_SYNC_JF  |    X                      |    X                | X                  | X                    |        X             |
 *|                      |                       |                           |                     |                    |                      |                      |
 *| PAGE_DIRTY_NO_SYNC_JF| X                     |  PAGE_CLEAN_SYNC          |  PAGE_CLEAN         | PAGE_CLEAN         | PAGE_DIRTY_SYNC_JF   |        X             |
 *|                      |                       |                           |                     |                    |                      |                      |
 *| PAGE_CLEAN_SYNC      |PAGE_DIRTY_SYNC_JF     |   X                       |  PAGE_CLEAN         | PAGE_CLEAN_SYNC    | X                    |PAGE_CLEAN            |
 *|                      |                       |                           |                     |                    |                      |                      |
 *| PAGE_DIRTY_SYNC_JF   | X                     |  PAGE_CLEAN_SYNC          |  PAGE_CLEAN         | PAGE_CLEAN_SYNC    | X                    |        X             |
 *---------------------------------------------------------------------------------------------------------------------------------------------------------------------
 *
 * 页缓存状态机变迁图              
 *                          syn/roll back                    syn jf
 *                       ___________________     ________________________________________________________________
 *              初态  <-/                   \   /                                                                \->
 *               PAGE_CLEAN ---write buf--> PAGE_DIRTY_NO_SYNC_JF --write back--> PAGE_CLEAN_SYNC --write buf--> PAGE_DIRTY_SYNC_JF
 *                 <-\   <-\______________________________________________________/   <-\_________________________/   /
 *                    \                 roll back                                             write back             /
 *                     \____________________________________________________________________________________________/
 *                                               sync / roll back
 */                      
static __INLINE__ int pager_change_page_state(pghd_t * pg, PAGE_EVENT evt)
{
#define X PAGE_STATE_ERR

	static PAGE_STATE __page_state_machine[PAGE_STATE_ERR][EVENT_END] = {
		{PAGE_DIRTY_NO_SYNC_JF, X,               X,              X         ,      X                 , X         },
		{X,                     PAGE_CLEAN_SYNC, PAGE_CLEAN,     PAGE_CLEAN,      PAGE_DIRTY_SYNC_JF, X         },
		{PAGE_DIRTY_SYNC_JF,    X,               PAGE_CLEAN,     PAGE_CLEAN_SYNC, X                 , PAGE_CLEAN},
		{X,                     PAGE_CLEAN_SYNC, PAGE_CLEAN,     PAGE_CLEAN_SYNC, X                 , X         },
	};

	assert(evt < (sizeof(__page_state_machine[0]) / sizeof(__page_state_machine[0][0])));
	assert(pg->pg_state < (sizeof(__page_state_machine) / sizeof(__page_state_machine[0])));

	pg->pg_state = __page_state_machine[pg->pg_state][evt];

	assert(PAGE_STATE_ERR != pg->pg_state);

	return 0;

#undef X
}

/**
 * @brief 判断一个页是否处在journal备份之中
 */
static __INLINE__ int pager_page_in_journal(pager_t * pager, unsigned int pgno)
{
	assert(pager && pager->hash_pgno_journal);

	if(MyHashTableSearch(pager->hash_pgno_journal, (void *)pgno))
		return 1;

	return 0;
}

/**
 * @brief 判断某一个是否为脏
 */
#define pager_page_is_dirty(__pg) (PAGE_DIRTY_SYNC_JF == __pg->pg_state || PAGE_DIRTY_NO_SYNC_JF == __pg->pg_state)

/**
 * @brief 是否在journal里做了备份
 */
#define pager_page_is_in_journal(__pg) (assert(pager_page_in_journal(__pg->pager, __pg->page_num) || PAGE_CLEAN == __pg->pg_state), PAGE_CLEAN != __pg->pg_state)

/**
 * @brief 是否做了journal同步
 */
#define pager_page_is_sync_journal(__pg) (PAGE_CLEAN_SYNC == __pg->pg_state || PAGE_DIRTY_SYNC_JF == __pg->pg_state)

/**
 * @brief 判断页缓存是否在提交前写回了页文件
 */
#define pager_page_is_written_before_page_sync(__pg) pager_page_is_sync_journal(__pg)

/**
 * @brief 判断页缓存是否在提交前写回了页文件
 */
#define pager_page_is_clean_not_in_jf(__pg) (PAGE_CLEAN == __pg->pg_state)

/**
 * @brief 获取有多少页被缓存
 */
#define pager_get_cached_pages_count(__pgr) MyHashTableGetElementCount(__pgr->hash_page)

/**
 * @brief 计算jf日志记录的长度
 */
#define pager_get_journal_rec_len(__pgr) (__pgr->page_size + sizeof(unsigned int) + sizeof(unsigned int))

/**
 * @brief 写一个无符号整理数至文件
 */
static __INLINE__ int pager_write_uint_to_file(HOSFILE hf, unsigned int uint_val)
{
	unsigned char acVal[sizeof(unsigned int)];

	assert(hf);

	uint_to_big_endian(uint_val, acVal, sizeof(acVal));

	if(0 != OsFileWrite(hf, acVal, sizeof(acVal), NULL))
		return -1;

	return 0;
}

/**
 * @brief 从文件中读出一个整形数
 */
static __INLINE__ int pager_read_uint_from_file(HOSFILE hf, unsigned int * uint_val)
{
	unsigned char acVal[sizeof(unsigned int)];

	assert(hf && uint_val);

	if(0 != OsFileRead(hf, acVal, sizeof(acVal), NULL))
		return -1;

	array_to_uint_as_big_endian(acVal, sizeof(acVal), *uint_val);

	return 0;
}

/**
 * @brief 在页缓存哈希表中查找
 */
static __INLINE__ pghd_t * pager_hash_lookup(pager_t * pager, unsigned int pgno)
{
	HMYHASHTABLE_ITER it = NULL;

	assert(pager && pager->hash_page);

	it = MyHashTableSearch(pager->hash_page, (void *)pgno);
	if(NULL == it)
		return NULL;

	assert(MyHashTableGetIterData(it));
	assert(((pghd_t *)MyHashTableGetIterData(it))->pager == pager);

	return (pghd_t *)MyHashTableGetIterData(it);
}

/**
 * @brief 判断某一个是否处于free_list
 */
#define pager_page_is_in_free_list(__pg) (__pg->pg_free_prev || __pg->pg_free_next || __pg == __pg->pager->pg_free_first)

/**
 * @brief 判断某一个是否处于sync_jf list
 */
#define pager_page_is_in_syncjf_list(__pg) (__pg->pg_syncjf_next || __pg->pg_syncjf_prev || __pg == __pg->pager->pg_syncjf_first)

/**
 * @brief 判断某一个是否处于dirty list中
 */
#define pager_page_is_in_dirty_list(__pg) (__pg->pg_dirty_next || __pg->pg_dirty_prev || __pg == __pg->pager->pg_dirty)

/**
 * @brief 判断某一个是否处于所有的页缓存链表中
 */
#define pager_page_is_in_all_list(__pg) (__pg->pg_all_next || __pg->pg_all_prev || __pg == __pg->pager->pg_all)

/**
 * @brief 将某一个页缓存置成非dirty状态,并从dirty链表中脱离
 */
static __INLINE__ int pager_page_out_of_dirty_list(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);
	assert(pager_page_is_in_dirty_list(pg));

	if(pg == pager->pg_dirty)
		pager->pg_dirty = pg->pg_dirty_next;

	if(pg->pg_dirty_prev)
		pg->pg_dirty_prev->pg_dirty_next = pg->pg_dirty_next;

	if(pg->pg_dirty_next)
		pg->pg_dirty_next->pg_dirty_prev = pg->pg_dirty_prev;

	pg->pg_dirty_prev = NULL;
	pg->pg_dirty_next = NULL;

	return 0;
}

/**
 * @brief 将某个页缓存加入dirty list
 */
static __INLINE__ int pager_add_page_to_dirty_list(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);
	assert(!pager_page_is_in_dirty_list(pg));
	assert(pager_page_is_dirty(pg));

	pg->pg_dirty_prev = NULL;

	pg->pg_dirty_next = pager->pg_dirty;
	if(pager->pg_dirty)
		pager->pg_dirty->pg_dirty_prev = pg;

	pager->pg_dirty = pg;

	return 0;
}

/**
 * @brief 将某一个页缓存置成dirty,并加入dirty链表
 */
static __INLINE__ int pager_make_page_dirty(pager_t * pager, pghd_t * pg, PAGE_EVENT evt)
{
	assert(!pager_page_is_dirty(pg));

	/* 修改页状态 */
	pager_change_page_state(pg, evt);

	pager_add_page_to_dirty_list(pager, pg);

	return 0;
}

/**
 * @brief 从sync jf list里脱链
 */
static __INLINE__ int pager_out_syncjf_list(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);
	assert(0 == pg->ref_count);

	if(pg == pager->pg_syncjf_first)
		pager->pg_syncjf_first = pg->pg_syncjf_next;

	if(pg == pager->pg_syncjf_last)
		pager->pg_syncjf_last = pg->pg_syncjf_prev;

	/* 从已同步过日志的链表中脱链 */
	if(pg->pg_syncjf_prev)
		pg->pg_syncjf_prev->pg_syncjf_next = pg->pg_syncjf_next;

	if(pg->pg_syncjf_next)
		pg->pg_syncjf_next->pg_syncjf_prev = pg->pg_syncjf_prev;

	pg->pg_syncjf_prev = NULL;
	pg->pg_syncjf_next = NULL;

	return 0;
}

/**
 * @brief 从free list里脱链,并从已同步jf页缓存链表中脱链
 */
static __INLINE__ int pager_out_free_list(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);
	assert(0 == pg->ref_count);

	if(pg == pager->pg_free_first)
		pager->pg_free_first = pg->pg_free_next;

	if(pg == pager->pg_free_last)
		pager->pg_free_last = pg->pg_free_prev;

	/* 从空闲页缓存链表中脱链 */
	if(pg->pg_free_prev)
		pg->pg_free_prev->pg_free_next = pg->pg_free_next;

	if(pg->pg_free_next)
		pg->pg_free_next->pg_free_prev = pg->pg_free_prev;

	pg->pg_free_prev = NULL;
	pg->pg_free_next = NULL;

	pager_out_syncjf_list(pager, pg);

	return 0;
}

/**
 * @brief 加入到空闲页缓存链表未尾
 */
static __INLINE__ int pager_add_to_free_list(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);

	assert((NULL == pg->pg_free_next && NULL == pg->pg_free_prev) || (pg->pg_free_next && pg->pg_free_prev));
	assert(0 == pg->ref_count);

	pg->pg_free_prev = pager->pg_free_last;
	if(pager->pg_free_last)
		pager->pg_free_last->pg_free_next = pg;

	pager->pg_free_last = pg;

	if(NULL == pager->pg_free_first)
		pager->pg_free_first = pg;

	return 0;
}

/**
 * @brief 将某个页缓存加入到已同步的jf的链表未尾,并加入到空闲页缓存链表未尾
 */
static __INLINE__ int pager_add_to_syncjf_list(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);

	assert(NULL == pg->pg_syncjf_next && NULL == pg->pg_syncjf_prev);
	assert(0 == pg->ref_count);

	pg->pg_syncjf_prev = pager->pg_syncjf_last;
	if(pager->pg_syncjf_last)
		pager->pg_syncjf_last->pg_syncjf_next = pg;

	if(NULL == pager->pg_syncjf_first)
		pager->pg_syncjf_first = pg;

	pager->pg_syncjf_last = pg;

	/* 加入到空闲页缓存链表 */
	if(!pager_page_is_in_free_list(pg))
		pager_add_to_free_list(pager, pg);

	assert(pager_page_is_in_free_list(pg));

	return 0;
}

/**
 * @brief 将某个页缓存加入页缓存哈希表和总的页缓存链表
 */
static __INLINE__ int pager_add_page_to_hash_and_list(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pg->pager == pager && pg->page_num);

	if(NULL == MyHashTableInsertUnique(pager->hash_page, (void *)pg->page_num, pg))
		return -1;

	pg->pg_all_next = pager->pg_all;
	if(pager->pg_all)
		pager->pg_all->pg_all_prev = pg;
	pager->pg_all = pg;

	return 0;
}

/**
 * @brief 将某个页缓存从页缓存哈希表和总的页缓存链表脱离
 */
static __INLINE__ int pager_outof_page_from_hash_and_list(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pg->pager == pager && pg->page_num);

	MyHashTableDelKey(pager->hash_page, (void *)(pg->page_num), NULL, NULL);

	if(pg == pager->pg_all)
		pager->pg_all = pg->pg_all_next;

	if(pg->pg_all_prev)
		pg->pg_all_prev->pg_all_next = pg->pg_all_next;

	if(pg->pg_all_next)
		pg->pg_all_next->pg_all_prev = pg->pg_all_prev;

	pg->pg_all_prev = NULL;
	pg->pg_all_next = NULL;

	return 0;
}

/**
 * @brief 增加指定页的引用计数
 */
static __INLINE__ int pager_ref_page(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg);
	assert(pg->pager == pager);

	/* 如果引用计数零,则从空闲链表中除去,引用计数加1 */
	if(0 == pg->ref_count)
	{
		//assert(pager_page_is_in_free_list(pg));
		pager_out_free_list(pager, pg);

		assert(!pager_page_is_in_free_list(pg));
		assert(!pager_page_is_in_syncjf_list(pg));
	}

	pg->ref_count ++;
	pager->ref_count ++;
	return 0;
}

/**
 * @brief 初始化页缓存
 */
static __INLINE__ int pager_init_page(pager_t * pager, pghd_t * pg, unsigned int pgno)
{
	assert(pager && pg && pgno != 0);

	memset(pg, 0, sizeof(*pg));

	/* 根据injournal着色图初始化相关标识 */
	if(pager_page_in_journal(pager, pgno))
		pg->pg_state = PAGE_CLEAN_SYNC;
	else
		pg->pg_state = PAGE_CLEAN;

	pg->pager = pager;

	pg->page_num = pgno;

	memset(pg->pg_buf_start + pager_get_journal_rec_len(pg->pager), 0, pager->extra_size);

	return 0;
}

/**
 * @brief 分配一个页缓存
 */
static __INLINE__ pghd_t * pager_allocate_page(pager_t * pager, unsigned int pgno)
{
	/* 页头信息存储空间 + 页缓存空间 + 用户数据空间 + 页号与校验和存储空间 */
	pghd_t * pg = (pghd_t *)MyMemPoolMalloc(pager->hm, sizeof(pghd_t) + pager->extra_size +
		pager_get_journal_rec_len(pager));

	if(NULL == pg)
		return NULL;

	pager_init_page(pager, pg, pgno);

	return pg;
}

/**
 * @brief 释放一个页缓存
 */
static __INLINE__ int pager_free_page(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);
	assert(0 == pg->ref_count);

	assert(!pager_page_is_in_free_list(pg));
	assert(!pager_page_is_in_syncjf_list(pg));
	assert(!pager_page_is_in_dirty_list(pg));

	pager_outof_page_from_hash_and_list(pager, pg);

	MyMemPoolFree(pager->hm, pg);

	return 0;
}

/**
 * @brief 同步备份日志至外存
 */
static __INLINE__ int pager_sync_journal(pager_t * pager)
{
	pghd_t * pg = NULL;

	assert(pager && pager->hjf);

	/* 如果不需要同步,直接返回 */
	if(!pager->need_sync_journal)
		return 0;

	/* 将日志头写入,主要为备份了多少页 */
	if(0 != OsFileSeek(pager->hjf, pager->jouranl_prev_hdr + PAGER_JOURNAL_REC_COUNT_OFFSET))
		return -1;

	if(0 != pager_write_uint_to_file(pager->hjf, pager->in_journal_pages_count))
		return -1;

	if(0 != OsFileSeek(pager->hjf, pager->journal_end))
		return -1;

	/* 调用os的file sync接口同步至外存 */
	if(0 != OsFileSyn(pager->hjf))
		return -1;

	pager->need_sync_journal = 0;
	pager->bsync_journal_has_done = 1;

	if(NULL == pager->pg_dirty)
		return 0;

	/* 将所有dirty页的need_sync_journal置成0 */
	pg = pager->pg_dirty;
	for(; pg; pg = pg->pg_dirty_next)
	{
		assert(pager_page_is_dirty(pg));

		if(!pager_page_is_sync_journal(pg))
			pager_change_page_state(pg, PAGE_EVT_SYNC_JOURNAL);

		if(pg->ref_count)
		{
			assert(!pager_page_is_in_free_list(pg));
			continue;
		}

		assert(pager_page_is_in_free_list(pg) && 0 == pg->ref_count);
	
		if(!pager_page_is_in_syncjf_list(pg))
			pager_add_to_syncjf_list(pager, pg);
	}

	return 0;
}

/**
 * @brief 获取外存文件包含有多少页
 */
static __INLINE__ unsigned int pager_get_total_pages_count(pager_t * pager)
{
	int64 file_size = 0;

	assert(pager && pager->hf);

	if(pager->total_page_count != (unsigned int)-1)
		return pager->total_page_count;

	if(0 != OsFileSize(pager->hf, &file_size))
		return -1;

	pager->total_page_count = (unsigned int)(file_size / pager->page_size);

	return pager->total_page_count;
}

/**
 * @brief 设置页文件的总页数
 */
#define pager_set_total_pages_count(__pgr, __pgno) do{ \
	unsigned int total_pages_count = pager_get_total_pages_count(__pgr);\
	assert(__pgno <= total_pages_count + 1);\
	if(total_pages_count < __pgno) \
		__pgr->total_page_count = __pgno;\
	}while(0)

/**
 * @brief 获取页缓冲的用户区首地址
 */
#define pager_get_page_bufdata(_pg) ((unsigned char *)(&(_pg[1])))

/**
 * @brief 计算指定页号的页首在页文件中的偏移
 */
#define pager_cal_page_hdr_offset(__pgr, __pgno) (__pgr->page_size * (__pgno - 1))

/**
 * @brief 根据页号,获取某一页的内容
 */
static __INLINE__ int pager_read_page_data(pager_t * pager, unsigned int pgno, unsigned char * buf, unsigned int buf_len)
{
	assert(pager && pgno && buf && pager->page_size && buf_len == pager->page_size);

	if(pgno > pager_get_total_pages_count(pager))
	{
		memset(buf, 0, pager->page_size);
		return 0;
	}

	if(0 != OsFileSeek(pager->hf, pager_cal_page_hdr_offset(pager, pgno)))
		return -1;

	if(0 != OsFileRead(pager->hf, buf, pager->page_size, NULL))
		return -1;

	return 0;
}

/**
 * @brief 根据页号,获取一页至缓存
 */
static __INLINE__ int pager_read_page(pager_t * pager, unsigned int pgno, pghd_t * pg)
{
	assert(pager && pgno && pg && pg->pager == pager);
	assert(pager->hf);

	if(0 != pager_read_page_data(pager, pgno, pg->pg_buf_start, pager->page_size))
		return -1;

	return 0;
}

/**
 * @brief 将指定的数据写进某一页
 */
static __INLINE__ int pager_write_page_data(pager_t * pager, unsigned char * buf, unsigned int buf_size, unsigned int pgno)
{
	assert(pager && buf && buf_size);
	assert(pager->hf);
	assert(buf_size <= pager->page_size);

	/* 将页写入外存页文件 */
	if(0 != OsFileSeek(pager->hf, pager_cal_page_hdr_offset(pager, pgno)))
		return -1;

	if(0 != OsFileWrite(pager->hf, buf, buf_size, NULL))
		return -1;

	return 0;
}

/**
 * @brief 将某一个数据写入页文件
 */
static __INLINE__ int pager_write_page(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);
	assert(pg->page_num);
	assert(pager->hf);

	assert(pager_page_is_dirty(pg));

	/* 将页写入外存页文件 */
	if(0 != pager_write_page_data(pager, pg->pg_buf_start, pager->page_size, pg->page_num))
		return -1;

	return 0;
}

/**
 * @brief 寻找jf头的位置
 * jf头的位置应对齐到sector_size的值,假如sector_size为512
 * 0 512 1024为jf头的起始位置
 */
#define pager_locate_jf_hdr_pos(__pgr) \
	((__pgr->journal_end % __pgr->sector_size)?(__pgr->journal_end + CAL_ALIGMENT(__pgr->journal_end, __pgr->sector_size)):__pgr->journal_end)

/**
 * @brief 写jf头
 */
static __INLINE__ int pager_reset_cksum(pager_t * pager)
{
	assert(pager && pager->hrand);

	pager->cur_cksum_init = (unsigned int)(myrandGetByte(pager->hrand)) << 24 | 
		(unsigned int)(myrandGetByte(pager->hrand)) << 16 |
		(unsigned int)(myrandGetByte(pager->hrand)) << 8 | 
		(unsigned int)(myrandGetByte(pager->hrand));

	return 0;
}

/**
 * @brief 写jf头
 */
static __INLINE__ int pager_write_jf_head(pager_t * pager)
{
	int ret = -1;
	unused_journal_hdr * hdr = MyMemPoolMalloc(pager->hm, pager->sector_size);
	if(NULL == hdr)
		goto pager_write_jf_head_end_;

	memset(hdr, 0, pager->sector_size);

	assert(pager && pager->hjf && pager->hrand && pager->sector_size >= sizeof(*hdr));

	/* 
	* journal日志文件头定义
	* offset bytes          des
	* 0      4              头的大小hd_sz
	* 4      16             格式版本
	* 20     4              日志里备份了几页
	* 24     4              校验和初始随机值
	* 28     4              备份日志前页文件的大小
	* 32     hd_sz-24       保留
	*/
	pager->jouranl_prev_hdr = pager_locate_jf_hdr_pos(pager);
	assert((pager->jouranl_prev_hdr % pager->sector_size) == 0);
	assert(pager->jouranl_prev_hdr >= pager->journal_end);

	if(0 != OsFileSeek(pager->hjf, pager->jouranl_prev_hdr))
		goto pager_write_jf_head_end_;

	/* 写入头的大小 */
	uint_to_big_endian(pager->sector_size, hdr->sector_size, sizeof(hdr->sector_size));

	/* 拷贝魔法数 */
	memcpy(hdr->magic, JOURNAL_MAGIC_STRING, sizeof(hdr->magic));

	/* 写记录数,初始应为零 */
	uint_to_big_endian(0, hdr->rec_count, sizeof(hdr->rec_count));

	/* 写入校验和初始随机值 */
	pager_reset_cksum(pager);
	uint_to_big_endian(pager->cur_cksum_init, hdr->cksum_init, sizeof(hdr->cksum_init));

	/* 写入备份jf前页文件有多少页,用于回滚时栽减 */
	uint_to_big_endian(pager->total_page_count, hdr->orig_page_count, sizeof(hdr->orig_page_count));

	if(0 != OsFileWrite(pager->hjf, hdr, pager->sector_size, NULL))
		goto pager_write_jf_head_end_;

	pager->journal_end = pager->jouranl_prev_hdr + pager->sector_size;

	ret = 0;

pager_write_jf_head_end_:

	if(hdr)
		MyMemPoolFree(pager->hm, hdr);

	return ret;
}

/**
 * @brief 读jf头
 * @return 0:成功 -1:失败 1:内容不正确,但是操作应结束了
 */
static __INLINE__ int pager_read_jf_head(pager_t * pager, 
										 int64 file_size,
										 unsigned int * pin_journal_count,
										 unsigned int * porig_pages_count,
										 unsigned int * pcksum_init)
{
	unsigned int sector_size = 0;
	char magic[JOURNAL_MAGIC_LEN];

	assert(pager && pager->hjf && pin_journal_count && porig_pages_count);

	/* 
	* journal日志文件头定义
	* offset bytes          des
	* 0      4              头的大小hd_sz
	* 4      16             格式版本
	* 20     4              日志里备份了几页
	* 24     4              校验和初始随机值
	* 28     4              备份日志前页文件的大小
	* 32     hd_sz-24       保留
	*/

	/* 定位jf头的偏移 */
	pager->journal_end = pager_locate_jf_hdr_pos(pager);

	if(pager->journal_end + PAGER_JOURNAL_REC_COUNT_LEN >= file_size)
		return RHAPSODY_DONE;

	if(OsFileSeek(pager->hjf, pager->journal_end))
		return RHAPSODY_FAIL;

	/* 读出页头的大小 */
	if(0 != pager_read_uint_from_file(pager->hjf, &sector_size))
		return RHAPSODY_FAIL;
	if(sector_size < PAGER_MIN_SECTOR_SIZE)
		return RHAPSODY_DONE;
	if(pager->journal_end + PAGER_JOURNAL_REC_COUNT_LEN >= file_size)
		return RHAPSODY_DONE;

	/* 读出magic字符串,判断它是否合法 */
	if(0 != OsFileRead(pager->hjf, magic, sizeof(magic), NULL))
		return RHAPSODY_FAIL;
	if(memcmp(magic, JOURNAL_MAGIC_STRING, JOURNAL_MAGIC_LEN) != 0)
		return RHAPSODY_DONE;

	/* 读出日志里备份了几页 */
	if(0 != pager_read_uint_from_file(pager->hjf, pin_journal_count))
		return RHAPSODY_FAIL;

	/* 读出校验和初始值 */
	if(0 != pager_read_uint_from_file(pager->hjf, pcksum_init))
		return RHAPSODY_FAIL;

	/* 读出备份前的页文件大小 */
	if(0 != pager_read_uint_from_file(pager->hjf, porig_pages_count))
		return RHAPSODY_FAIL;

	pager->journal_end += sector_size;

	/* 定位文件指针位置 */
	if(0 != OsFileSeek(pager->hjf, pager->journal_end))
		return RHAPSODY_FAIL;
	
	return RHAPSODY_OK;
}

/**
 * @brief 以只读的方式打开journal
 */
static __INLINE__ int pager_open_journal_readonly(pager_t * pager)
{
	assert(pager && NULL ==	pager->hjf);

	/* 打开jf文件 */
	pager->hjf = OsFileOpenReadOnly((char *)MyBufferGet(pager->hb_jf_name, NULL), pager->hm);
	if(NULL == pager->hjf)
		return -1;

	/* 清空原有的着色表 */
	assert(pager->hash_pgno_journal);
	MyHashTableClear(pager->hash_pgno_journal);

	pager->pg_syncjf_first = NULL;
	pager->pg_syncjf_last = NULL;

	pager->in_journal_pages_count = 0;
	pager->orig_pages_count = pager_get_total_pages_count(pager);
	pager->need_sync_journal = 0;
	pager->bsync_journal_has_done = 0;
	
	pager->jouranl_prev_hdr = 0;
	pager->journal_end = 0;

	return 0;
}

/**
 * @brief 打开jf
 */
static __INLINE__ int pager_open_journal(pager_t * pager)
{
	assert(pager && NULL ==	pager->hjf);

	/* 打开jf文件 */
	pager->hjf = OsFileOpenExclusive((char *)MyBufferGet(pager->hb_jf_name, NULL), pager->hm);
	if(NULL == pager->hjf)
		return -1;

	pager->jouranl_prev_hdr = 0;
	pager->journal_end = 0;

	/* 写入jf头,校验和初始化随机数值,初始数据库的大小 */
	if(0 != pager_write_jf_head(pager))
		return -1;

	/* 清空原有的着色表 */
	assert(pager->hash_pgno_journal);
	MyHashTableClear(pager->hash_pgno_journal);

	pager->pg_syncjf_first = NULL;
	pager->pg_syncjf_last = NULL;

	pager->in_journal_pages_count = 0;
	pager->orig_pages_count = pager_get_total_pages_count(pager);
	pager->need_sync_journal = 0;
	pager->bsync_journal_has_done = 0;

	return 0;
}

/**
 * @brief 关闭jf并删除jf文件,清空所有跟jf有关的
 */
static __INLINE__ int pager_close_and_del_journal(pager_t * pager)
{
	assert(pager && pager->hjf);

	if(0 != OsFileClose(pager->hjf))
		return -1;

	pager->hjf = NULL;

	if(0 != OsFileDel(MyBufferGet(pager->hb_jf_name, NULL)))
		return -1;

	assert(pager->hash_pgno_journal);
	MyHashTableClear(pager->hash_pgno_journal);

	assert(NULL == pager->pg_syncjf_first && NULL == pager->pg_syncjf_last);

	pager->in_journal_pages_count = 0;
	pager->orig_pages_count = (unsigned int)-1;
	pager->need_sync_journal = 0;
	pager->bsync_journal_has_done = 0;
	pager->jouranl_prev_hdr = 0;
	pager->journal_end = 0;

	return 0;
}

/**
 * @brief 计算一页数的校验和
 */
static __INLINE__ unsigned int pager_cal_page_cksum(unsigned char * pg_data, unsigned int pg_sz, unsigned int ckcsum_init)
{
	unsigned int i = pg_sz - 1;

	assert(pg_data && pg_sz);

	for(; i; i /= 2)
		ckcsum_init += *((unsigned char *)pg_data + i);

	return ckcsum_init;
}

/**
 * @brief 标记一页要jf里s
 */
static __INLINE__ unsigned int pager_mark_page_in_journal(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);
	assert(pg->page_num);

	if(NULL == MyHashTableInsertUnique(pager->hash_pgno_journal, (void *)pg->page_num, NULL))
		return -1;

	return 0;
}

/**
 * @brief 回收一页缓存
 */
static __INLINE__ pghd_t * pager_recycle_page(pager_t * pager)
{
	int need_sync = 0;
	pghd_t * pg = NULL;

	assert(pager);
	assert(pager->pg_free_first);

	/*
	* 如果存在引用计数为零的页缓存,则淘汰该页缓存,用于装载新的页,此时需要将此页写回fd,
	*　优先取那些同步过jfd的空闲页缓存,如果没有,则同步jfd,同时从哈希表里删除相应的记录
	*  如果页处在空闲链表中,脱链
	*  如果页处在同步过jfd的空闲链表中,脱链
	*/

	if(pager->pg_syncjf_first)
	{
		/* 优先取同步过jfd的空闲缓存页 */
		pg = pager->pg_syncjf_first;
		need_sync = 0;
	}
	else
	{
		/* 如果没有,则取没同步过的空闲页缓存 */
		pg = pager->pg_free_first;
		need_sync = 1;
	}

	assert(pg);

	if(pager_page_is_dirty(pg))
	{
		if(need_sync)
		{
			/* 同步jf */
			if(0 != pager_sync_journal(pager))
				return NULL;
			pager->in_journal_pages_count = 0;

			if(0 != pager_write_jf_head(pager))
				return NULL;
		}

		/* 写回fd */
		if(0 != pager_write_page(pager, pg))
			return NULL;

		/* 应记录已经有页缓存写入了页文件,但数据还不是完整的 */
		pager->page_file_is_integrity = 0;

		/* 将页置成干净,并从dirty链表中脱链 */
		pager_page_out_of_dirty_list(pager, pg);

		/* 将页的状态置成写回状态 */
		pager_change_page_state(pg, PAGE_EVT_WRITE_BACK_SYNCJF);
	}
	else
		assert(pager_page_is_clean_not_in_jf(pg) || (pager_page_in_journal(pager, pg->page_num) && pager_page_is_sync_journal(pg)));

	/* 如果在空闲链表中则脱链 */
	pager_out_free_list(pager, pg);
	assert(!pager_page_is_in_free_list(pg));
	assert(!pager_page_is_in_syncjf_list(pg));

	/* 从哈希表中删除,并从记录所有页缓存的链表中删除 */
	pager_outof_page_from_hash_and_list(pager, pg);

	return pg;
}

/**
 * @brief 回收一页缓存,并初始化
 */
static __INLINE__ pghd_t * pager_recycle_and_init_page(pager_t * pager, unsigned int pgno)
{
	pghd_t * pg = NULL;

	assert(pager && pgno);

	pg = pager_recycle_page(pager);
	if(NULL == pg)
		return NULL;

	assert(NULL == pg->pg_all_next && NULL == pg->pg_all_prev &&
		!pager_page_is_in_free_list(pg) &&
		!pager_page_is_in_syncjf_list(pg) &&
		!pager_page_is_in_dirty_list(pg) &&
		!pager_page_is_dirty(pg));

	pager_init_page(pager, pg, pgno);

	return pg;
}

/**
 * @brief 将某一个页缓存置成脏,并写入jf
 */
static __INLINE__ void * pager_make_page_writable(pghd_t * pg)
{
	pager_t * pager = NULL;
	void * buf_write = NULL;

	/*
	* 判断jfd机制是否已启用,如果未启用则应该打开jfd文件,并分配着色表
	* 判断此页是否已经处在jfd中了(从着色表里查),
	*    如果没有则需要写入jfd,先页号,再写页缓存内容,最后写入校验和,并且给该页缓存做上injournal标识,并在着色表相应的位置置标识
	*    将页缓存置成dirty,加入dirty链表.
	* 如果页号比当前总页数大,
	* 此时应将页文件的总页数增1
	* 返回页缓存的首地址供上层写入(注意越界检测代码加入)
	*/

	assert(pg && pg->pager);
	assert(pg->ref_count > 0);

	buf_write = pager_get_page_bufdata(pg);

	pager = pg->pager;

	/* 如果未启用jf机制,则应启用 */
	if(NULL == pager->hjf)
	{
		if(0 != pager_open_journal(pager))
			return NULL;
	}

	/* 如果此页已在jf里做备份(pg->page_state != PAGE_CLEAN) */
	if(pager_page_is_in_journal(pg))
	{
		assert(pg->page_num <= pager_get_total_pages_count(pager));

		/* 将页置成dirty,并加入dirty list */
		if(!pager_page_is_dirty(pg))
			pager_make_page_dirty(pager, pg, PAGE_EVT_WRITE_BUF_BK);

		return buf_write;
	}

	/* 如果(pg->page_state == PAGE_CLEAN)将调用以下代码 */
	assert(!pager_page_is_dirty(pg));

	assert(pager->orig_pages_count != (unsigned int)-1);

	if(pg->page_num > pager->orig_pages_count)
	{
		/*
		* 页在修改之前本身是不存在的,所以也不用真正地写入,做个标记即可
		* 这里应处理当有多个页的超出页文件大小的情况,因为超出文件大小情况下,只需要同步
		* jf头就可以了,这种情况只需要同步一次,即之后再发生这种情况,因为不存在写入的问题
		* jf是不用步的,因为已经同步过了
		*/
		pager_set_total_pages_count(pager, ((unsigned int)pg->page_num));

		pager_make_page_dirty(pager, pg, PAGE_EVT_WRITE_BUF_BK);

		/* 如果已经做过同步,则需要修改状态 */
		if(pager->bsync_journal_has_done)
			pager_change_page_state(pg, PAGE_EVT_SYNC_JOURNAL);
	}
	else
	{
		unsigned int cksum = pager_cal_page_cksum(pg->pg_buf_start, pager->page_size, pager->cur_cksum_init);
		/*
		* 如果没有则需要写入jfd, <页缓存内容,先页号,最后写入校验和>,并且给该页缓存做上injournal标识,并在着色表相应的位置置标识
		*/
		uint_to_big_endian((unsigned int)pg->page_num, (pg->pg_buf_start + pager->page_size), sizeof(unsigned int));
		uint_to_big_endian(cksum,
			pg->pg_buf_start + pager->page_size + sizeof(unsigned int),
			sizeof(unsigned int));

		if(0 != OsFileWrite(pager->hjf, pg->pg_buf_start, pager_get_journal_rec_len(pager), NULL))
			return NULL;

		pager->in_journal_pages_count ++;
		pager->journal_end += pager_get_journal_rec_len(pager);

		/* 将页置成dirty,并加入dirty list */
		pager_make_page_dirty(pager, pg, PAGE_EVT_WRITE_BUF_BK);

		assert(!pager_page_is_sync_journal(pg));
	}

	pager_mark_page_in_journal(pager, pg);

	if(!pager_page_is_sync_journal(pg))
		pager->need_sync_journal = 1;

	return buf_write;
}

/**
 * @brief 从jf里回滚
 */
static __INLINE__ int pager_file_truncate(pager_t * pager, unsigned int page_count)
{
	assert(pager && pager->hf);

	if(0 != OsFileTruncate(pager->hf, page_count * pager->page_size))
		return -1;

	return 0;
}

/**
 * @brief 从jf里回滚
 */
static __INLINE__ int pager_roll_back_one_page(pager_t * pager,
											   unsigned int orig_page_count,
											   unsigned int cksum_init,
											   unsigned char * data_buf,
											   const unsigned int data_buf_len)
{
	unsigned int cksum = 0;
	unsigned int pg_no = 0;
	pghd_t * pg = NULL;

	assert(pager && pager->hjf && pager->page_size);
	assert(pager_get_journal_rec_len(pager) == data_buf_len && data_buf);
	assert(pager_get_journal_rec_len(pager) >= pager->page_size);

	if(0 != OsFileRead(pager->hjf, data_buf, data_buf_len, NULL))
		return RHAPSODY_FAIL;

	pager->journal_end += data_buf_len;

	/* 计算校验和,判断此页是否是正确的 */
	array_to_uint_as_big_endian(data_buf + pager->page_size + sizeof(unsigned int), sizeof(unsigned int), cksum);
	if(cksum != pager_cal_page_cksum(data_buf, pager->page_size, cksum_init))
		return RHAPSODY_DONE;

	/* 在页缓存哈希表里查找 */
	array_to_uint_as_big_endian(data_buf + pager->page_size, sizeof(unsigned int), pg_no);
	pg = pager_hash_lookup(pager, pg_no);

	if(NULL == pg || pager_page_is_written_before_page_sync(pg))
	{
		/*
		* 找不到,需要写回文件
		* 找到,则判断是否在提交之前写回过文件,如果有也要写回文件
		*/
		if(0 != pager_write_page_data(pager, data_buf, pager->page_size, pg_no))
			return RHAPSODY_FAIL;
	}

	/* 重刷内存 */
	if(pg)
	{
		assert(PAGE_CLEAN != pg->pg_state);

		memcpy(pg->pg_buf_start, data_buf, pager->page_size);

		if(pager_page_is_dirty(pg))
		{
			assert(pager_page_is_in_dirty_list(pg));
			pager_page_out_of_dirty_list(pager, pg);
		}

		pager_change_page_state(pg, PAGE_EVT_ROLL_BACK);

		/* 回调用户的reload回调 */
		if(pager->page_reload_cb)
			pager->page_reload_cb(pg);
	}

	return RHAPSODY_OK;
}

/**
 * @brief 从jf里回滚
 */
static __INLINE__ int pager_roll_back_from_journal(pager_t * pager)
{
	int64 file_size = 0;
	unsigned int injournal_count = 0;
	unsigned int orig_page_count = 0;
	unsigned int cksum_init = 0;
	int ret = 0;
	int bfirst = 1;
	int i = 0;
	unsigned char * data_buf = NULL;
	unsigned int data_buf_len = 0;

	assert(pager && pager->hjf);

	/* 分配处理回滚过程中需要的内存 */
	data_buf_len = pager_get_journal_rec_len(pager);
	data_buf = MyMemPoolMalloc(pager->hm, data_buf_len);
	if(NULL == data_buf)
		return -1;

	if(0 != OsFileSize(pager->hjf, &file_size))
	{
		ret = -1;
		goto pager_roll_back_from_journal_end_;
	}

	if(0 != OsFileSeek(pager->hjf, 0))
	{
		ret = -1;
		goto pager_roll_back_from_journal_end_;
	}

	pager->journal_end = 0;

	while(1)
	{
		/* 先读取jf头 */
		ret = pager_read_jf_head(pager, file_size, &injournal_count, &orig_page_count, &cksum_init);
		if(RHAPSODY_OK != ret)
			break;

		if(bfirst)
		{
			/* 如果读出的第一个jf头,则要将页文件栽减成原来的大小 */
			if(0 != pager_file_truncate(pager, orig_page_count))
			{
				ret = -1;
				goto pager_roll_back_from_journal_end_;
			}

			pager->total_page_count = orig_page_count;

			bfirst = 0;
		}

		/* 如果是最后一个jf头,并且记录数显示为零,则有可能jf还未同步 */
		if(0 == injournal_count && pager->journal_end == (pager->jouranl_prev_hdr + pager->sector_size))
		{
			/* 说明此时有可能jf本身还同步,剩余的脏页可以直接于页文件里取,因为没有写回 */
			assert(pager->journal_end + pager->in_journal_pages_count * pager_get_journal_rec_len(pager) == file_size
				|| 0 == pager->ref_count);

			assert((pager->journal_end < file_size && pager->need_sync_journal && pager->pg_dirty) || 0 == pager->ref_count || NULL == pager->pg_dirty);

			/*
			* 此时外存对的页文件的页是干净的,但此时仍然从jf里取
			* 代价是额外的计算校验和的开销,
			* 如果改成直接从外页页文件中,则看外设的特点,以点os对文件的组织形式.
			* 如果外设是flash,此种方法效率较低.
			* 如果外设是机械磁盘,则要看页文件的大小,以及os对文件的组织.也许从jf里读,可以减少磁盘io时的寻道时间.
			*/
			injournal_count = pager->in_journal_pages_count;
		}

		/* 根据jf头,循环读出每一条备份记录 */
		for(i = injournal_count; i > 0; i --)
		{
			ret = pager_roll_back_one_page(pager, orig_page_count, cksum_init, data_buf, data_buf_len);
			if(RHAPSODY_OK != ret)
				break;
		}
	}

	{
		/*
		* 还需要还原内存中那些超过页文件大小的页缓存,
		* 将它们的数据刷回0
		* todo:优先此段代码
		*/
		pghd_t * pg = pager->pg_all;

		for(; pg; pg = pg->pg_all_next)
		{
			assert(!pager_page_is_dirty(pg));

			if(orig_page_count <= pg->page_num)
				memset(pg->pg_buf_start, 0, pager->page_size);

			if(pager_page_is_sync_journal(pg) && 0 == pg->ref_count)
			{
				assert(pager_page_is_in_free_list(pg));
				assert(pager_page_is_in_syncjf_list(pg));

				pager_out_syncjf_list(pager, pg);
			}

			assert(!pager_page_is_in_syncjf_list(pg));

			if(pager_page_is_dirty(pg))
			{
				assert(pager_page_is_in_dirty_list(pg));
				pager_page_out_of_dirty_list(pager, pg);
			}

			if(!pager_page_is_clean_not_in_jf(pg))
				pager_change_page_state(pg, PAGE_EVT_DEL_JOURNAL);

			assert(!pager_page_is_sync_journal(pg));
		}

		assert(NULL == pager->pg_syncjf_first && NULL == pager->pg_syncjf_last);
	}

	/* 如果ret != RHAPSODY_FAIL 删除备份日志文件 */
	if(RHAPSODY_FAIL != ret)
	{
		assert(NULL == pager->pg_dirty);
		pager->page_file_is_integrity = 1;

		if(0 != pager_close_and_del_journal(pager))
		{
			ret = -1;
			goto pager_roll_back_from_journal_end_;
		}

		ret = 0;
	}

pager_roll_back_from_journal_end_:

	MyMemPoolFree(pager->hm, data_buf);

	return ret;
}

/**
 * @brief 第一页获取页缓存引用,判断是否存在jf文件,存在则执行回滚操作
 */
static __INLINE__ int pager_check_and_roll_back(pager_t * pager)
{
	assert(pager && pager->hf);

	/* 判断jf文件是否存在,不存在,返回成功 */
	if(!OsFileExists((char *)MyBufferGet(pager->hb_jf_name, NULL)))
		return 0;

	/*
	* 存在,需要打开它进行回滚
	* 以只读方式打开jf文件
	*/
	if(0 != pager_open_journal_readonly(pager))
		return -1;

	/* 回滚 */
	if(0 != pager_roll_back_from_journal(pager))
		return -1;

	return 0;
}

/**
 * @brief 重新加载页缓存中的脏页
 */
static __INLINE__ int pager_reload_cache(pager_t * pager)
{
	unsigned char * buf = NULL;
	pghd_t * pg = NULL;

	assert(pager && pager->hf);
	assert(pager->pg_dirty);
	assert(pager->page_size);
	assert(pager->page_file_is_integrity);
	assert(NULL == pager->pg_syncjf_first && NULL == pager->pg_syncjf_last);

	buf = MyMemPoolMalloc(pager->hm, pager->page_size);
	if(NULL == buf)
		return -1;

	for(pg = pager->pg_dirty; pg; pg = pager->pg_dirty)
	{
		assert(pager_page_is_dirty(pg));
		assert(!pager_page_is_in_syncjf_list(pg));
		assert(!pager_page_is_sync_journal(pg));

		/* 从外存文件读取相应的页 */
		if(pg->page_num <= pager->orig_pages_count)
		{
			if(0 != pager_read_page_data(pager, pg->page_num, buf, pager->page_size))
				return -1;

			/* 拷贝至页缓存 */
			memcpy(pg->pg_buf_start, buf, pager->page_size);
		}
		else
			memset(pg->pg_buf_start, 0, pager->page_size);

		/* 改变页的状态 */
		pager_page_out_of_dirty_list(pager, pg);

		pager_change_page_state(pg, PAGE_EVT_ROLL_BACK);
		assert(PAGE_CLEAN == pg->pg_state);

		if(pager->page_reload_cb)
			pager->page_reload_cb(pg);
	}

	assert(buf);
	MyMemPoolFree(pager->hm, buf);

	assert(NULL == pager->pg_dirty);

	/* 删除备份的jf文件 */
	if(0 != pager_close_and_del_journal(pager))
		return -1;

	return 0;
}

/**
 * @brief 取消所有的页的更改
 */
static __INLINE__ int pager_rollback(pager_t * pager)
{
	int ret = -1;

	assert(pager);

	/*
	* 在PagerSyn之前,RollBack是有效的,PagerSyn之后,RollBack没有意义了.
	* RollBack判断页文件是否已经同步过jfd,并且向fd里写了数据.此时应从jfd里回滚所有页
	* 如果没有,则重新加载fd里相应的页至脏页缓存即可,
	* 删除份的jfd文件.
	* jfd着色表以及页的injournal标识均置成0,因为数据恢复成完整状态.
	*/

	if(!pager->page_file_is_integrity)/* 如果在提交之前,有部分页缓存有写回至页文件,从jf里还原 */
		ret = pager_roll_back_from_journal(pager);
	else if(NULL == pager->pg_dirty)/* 如果没有脏的缓存页,把jf删除即可 */
		ret = pager_close_and_del_journal(pager);
	else/* 如果在提交之前,没有页缓存写回页文件,重新将脏缓存加载一次即可 */
		ret = pager_reload_cache(pager);

	/* 完成回滚之后,jf应删除 */
	return ret;
}

/**
 * @brief 按页号递增进行排序的比较回调函数
 *  > 0  表示 key1 大于 key2
 *  == 0 表示 key1 等于 key2
 *  < 0  表示 key1 小于 key2
 *
 * @param context:用户自定义的上下文数据
 */
static int pager_sort_dirty_compare(const void * data1, const void * data2, const void * context)
{
	pghd_t * pg1 = *((pghd_t **)data1);
	pghd_t * pg2 = *((pghd_t **)data2);
	assert(pg1 && pg2);
	assert(pg2->page_num && pg1->page_num);

	/* 2008-8-22 页号按递增排,减少提交时所花的时间 */
	if(pg1->page_num <= pg2->page_num)
		return pg2->page_num - pg1->page_num;
	else
		return -1;
}

/**
 * @brief 同步所有脏缓存页
 */
static __INLINE__ int pager_sort_dirty_list(pager_t * pager)
{
	/*
	* sqlite有用的是归并排序
	* 对脏页进行排序,保证从最小的页开始写回外存
	* 在将脏页写入外存页文件之前,按页号进行排序是很重要的
	*
	* 分配一块内存数组
	* 将所有的页依次填写这个内存数组
	* 以页号为比较关键字对这个数组进行快速排序
	* 将排序好的数组按次序添加至dirty list
	*/

	pghd_t * pg = NULL;
	pghd_t ** pg_array = NULL;
	unsigned int dirty_pages_count = 0;
	unsigned int i = 0;
	assert(pager && pager->pg_dirty);

	for(pg = pager->pg_dirty; pg; pg = pg->pg_dirty_next)
		dirty_pages_count ++;

	if(dirty_pages_count <= 1)
		return 0;

	pg_array = MyMemPoolMalloc(pager->hm, sizeof(pg_array[0]) * dirty_pages_count);
	if(NULL == pg_array)
		return -1;

	for(i = 0, pg = pager->pg_dirty; pg; pg = pg->pg_dirty_next, i ++)
	{
		assert(pager_page_is_dirty(pg));
		assert(pg->page_num);

		assert(i < dirty_pages_count);
		pg_array[i] = pg;
	}

	pg = NULL;
	MyAlgQuickSort(pg_array, dirty_pages_count, sizeof(pg_array[0]), pager_sort_dirty_compare, NULL, NULL, NULL, NULL, &pg, sizeof(pg));
	pager->pg_dirty = NULL;

	for(i = 0; i < dirty_pages_count; i ++)
	{
		pg_array[i]->pg_dirty_next = NULL;
		pg_array[i]->pg_dirty_prev = NULL;

		pager_add_page_to_dirty_list(pager, pg_array[i]);
	}

	assert(pg_array);
	MyMemPoolFree(pager->hm, pg_array);

	return 0;
}

/**
 * @brief 同步所有脏缓存页
 */
static __INLINE__ int pager_syn(pager_t * pager)
{
	pghd_t * pg = NULL;

	/*
	* 首先将jfd文件同步至外存,将所有页缓存置成同步过jfd
	* 将同步过的空闲页缓存加入指定的链表(用于页缓存满时,换页优先从这个链表里取空闲页缓存)
	* 将所有的dirty页写入fd中,将dirty链表置空.每页的dirty标识也置空
	* 删除份的jfd文件.
	* jfd着色表以及页的injournal标识均置成0,因为一个完整提交已经完成了.
	*
	* 成功之后,将文件的完整性置成1
	*
	*/

	assert((pager->pg_dirty && pager->hjf) || 
		(!pager->page_file_is_integrity && pager->hjf) ||
		(!pager->hjf && NULL == pager->pg_dirty && pager->page_file_is_integrity));

	/* 如果不存在脏页,并且外页的数据是完整的,同步就不需要进行 */
	if(NULL == pager->pg_dirty && pager->page_file_is_integrity)
		return 0;

	/* 将jf同步至外存 */
	if(0 != pager_sync_journal(pager))
		return -1;

	/* 将脏页写回页文件 */
	if(pager->pg_dirty)
		pager_sort_dirty_list(pager);
	for(pg = pager->pg_dirty; pg; pg = pager->pg_dirty)
	{
		assert(pager_page_is_dirty(pg));

		/* 写页缓存写回页文件 */
		if(0 != pager_write_page(pager, pg))
			return -1;

		/* 从dirty list里脱链 */
		pager_page_out_of_dirty_list(pager, pg);

		/* 改变页状态 */
		pager_change_page_state(pg, PAGE_EVT_WRITE_BACK_SYNCJF);
	}

	assert(NULL == pager->pg_dirty);

	/* 同步页文件至外存 */
	if(0 != OsFileSyn(pager->hf))
		return -1;

	/*
	* 清除所有页的clean_sync标记
	* todo 优先此段代码
	*/
	for(pg = pager->pg_all; pg; pg = pg->pg_all_next)
	{
		assert(!pager_page_is_dirty(pg));

		if(0 == pg->ref_count)
		{
			assert(pager_page_is_in_free_list(pg));

			if(pager_page_is_sync_journal(pg))
			{
				assert(pager_page_is_in_syncjf_list(pg));
				pager_out_syncjf_list(pager, pg);
			}
			else
				assert(!pager_page_is_in_syncjf_list(pg));
		}
		else
		{
			assert(!pager_page_is_in_free_list(pg));
			assert(!pager_page_is_in_syncjf_list(pg));
		}

		assert(!pager_page_is_in_syncjf_list(pg));

		if(!pager_page_is_clean_not_in_jf(pg))
			pager_change_page_state(pg, PAGE_EVT_SYNC);

		assert(pager_page_is_clean_not_in_jf(pg));
	}

	assert(NULL == pager->pg_syncjf_first && NULL == pager->pg_syncjf_last);

	pager->page_file_is_integrity = 1;

	/* 关闭jouranl */
	if(0 != pager_close_and_del_journal(pager))
		return -1;

	return 0;
}

/**
 * @brief 销毁缓存管理
 */
static __INLINE__ int pager_destroy(pager_t * pager)
{
	assert(pager);

	/* 做回滚操作 */
	if(pager->hf && !pager->page_file_is_integrity)
		pager_rollback(pager);

	if(pager->hjf)
		pager_close_and_del_journal(pager);

	if(pager->hf)
		OsFileClose(pager->hf);

	if(pager->hash_page)
		MyHashTableDestruct(pager->hash_page);

	if(pager->hash_pgno_journal)
		MyHashTableDestruct(pager->hash_pgno_journal);

	if(pager->hrand)
		myrandDestruct(pager->hrand);

	if(pager->hb_file_name)
		MyBufferDestruct(pager->hb_file_name);

	if(pager->hb_jf_name)
		MyBufferDestruct(pager->hb_jf_name);

	{
		/* 依次释放页缓存 */
		pghd_t * pg = pager->pg_all;
		while(pg)
		{
			pghd_t * pg_temp = pg->pg_all_next;
			MyMemPoolFree(pager->hm, pg);
			pg = pg_temp;
		}
	}

	MyMemPoolFree(pager->hm, pager);

	return 0;
}

/**
 * @brief 页缓存哈希表哈希函数
 */
static size_t pg_hash_fun(const void * key)
{
	return (size_t)key;
}

/**
 * @brief 页缓存哈希表,比较键值是否相等 1:相等 0:不相等
 */
static int pg_hash_keyequl_fun(const void * key1, const void * key2)
{
	return key1 == key2;
}

/**
 * @brief 根据页号,获取一页缓存
 */
static __INLINE__ pghd_t * pager_get_page(pager_t * pager, unsigned int pgno)
{
	/*
	* 如果是第一次获取页,则应判断journal日志文是否存,如果存在则需要执行回滚操作
	* 将journal日志文件中的备份,依次读入,检查校验和,如果正确,写入页文件fd
	* 回滚结束后,应删除jfd文件
	* 首先从页缓存的哈希表里查找,如果找到,页引用计数加1,函数返回
	* 如果在缓存中找不到相应的页,则需要分配页缓存,从外存读取,初始化是否injournal的标识
	* 此时应注意页缓存满的情况:
	* 如果不存在引用计数为零的页缓存,则仍然分配页缓存.
	* 如果存在引用计数为零的页缓存,则淘汰该页缓存,用于装载新的页,此时需要将此页写回fd,
	*　优先取那些同步过jfd的空闲页缓存,如果没有,则同步jfd,同时从哈希表里删除相应的记录
	*  将页号与页缓存指针添加进哈希表
	*  如果页处在空闲链表中,脱链
	*  如果页处在同步过jfd的空闲链表中,脱链
	*  页引用计数加1
	*/

	pghd_t * pg = NULL;

	assert(pager && pager->hash_page);

	if(0 == pager->ref_count && NULL == pager->hjf)
	{
		/*
		* 如果是第一次获取页,则应判断journal日志文是否存,如果存在则需要执行回滚操作
		* 将journal日志文件中的备份,依次读入,检查校验和,如果正确,写入页文件fd
		* 回滚结束后,应删除jfd文件
		*/
		if(0 != pager_check_and_roll_back(pager))
			return NULL;
	}

	pg = pager_hash_lookup(pager,  pgno);

	if(NULL == pg)
	{
		if((pager->pg_free_first || pager->pg_syncjf_first) && pager_get_cached_pages_count(pager) >= pager->max_cache_pages)
			pg = pager_recycle_and_init_page(pager, pgno);/* 回收一页缓存 */
		else
			pg = pager_allocate_page(pager, pgno);/* 分配一页缓存 */

		if(NULL == pg)
			goto pager_get_page_err_;

		assert(pg->page_num == pgno);

		/* 读取页 */
		if(0 != pager_read_page(pager, pgno, pg))
			goto pager_get_page_err_;

		/* 进入哈希表,以及总的页缓存链表 */
		if(0 != pager_add_page_to_hash_and_list(pager, pg))
			goto pager_get_page_err_;
	}

	pager_ref_page(pager, pg);
	return pg;

pager_get_page_err_:

	/* 由于没有记录在案,此页缓存必需释放 */
	if(pg)
		pager_free_page(pager, pg);

	return NULL;
}

/**
 * @brief 取消对某个页缓存的引用
 */
static __INLINE__ int pager_release_page(pghd_t * pg)
{
	pager_t * pager = NULL; 

	assert(pg && pg->pager);
	assert(pg->ref_count > 0);

	pager = pg->pager;

	/*
	* 页引用计数减1,
	* 如果页的引用计数为零,则可以将它加入空闲页缓存链表,如果该页并且已经同步过jfd,则可以加入空闲的jfd缓存链表,
	*   并且需要调用用户的release回调函数通知上层
	*/

	/* 引用计数减1 */
	assert(pg->ref_count);
	pg->ref_count --;

	assert(pager->ref_count);
	pager->ref_count --;

	if(pg->ref_count > 0)
		return 0;

	assert(!pager_page_is_in_free_list(pg));
	assert(!pager_page_is_in_syncjf_list(pg));

	/* 如果引用计数为零 */
	/* 如果需要同步至jf,加入的free list */
	if(pager_page_is_sync_journal(pg))
		pager_add_to_syncjf_list(pager, pg);/* 如果不需要同步至jf,加入到syncjf_list,以便优先回收 */
	else
		pager_add_to_free_list(pager, pg);

	/* 调用用户的回调函数 */
	if(pager->page_release_cb)
		pager->page_release_cb(pg);

	return 0;
}

/**
 * @brief 写入页头信息
 */
static __INLINE__ int pager_write_file_page_info(pager_t * pager)
{
	/*
	* 页的第一页用于存放页文件本身的信息
	* 第一页
	*  offset bytes        des
	*  0      16           格式版本
	*  16     4            页大小
	*  20     4            第一个空闲页
	*  24     40           保留
	*  64   pg_sz - 64     用户数据     
	*/

	pghd_t * first_pg = NULL;

	assert(pager && pager->hf);
	assert(0 == pager->ref_count);

	first_pg = pager_get_page(pager, PAGER_FIRST_PAGE_NO);
	if(NULL == first_pg)
		return -1;

	if(NULL == pager_make_page_writable(first_pg))
		return -1;

	memcpy(first_pg->pg_buf_start + PAGER_HDR_VER_OFFSET, VERSION, PAGER_HDR_VER_LEN);

	uint_to_big_endian(pager->page_size, first_pg->pg_buf_start + PAGER_HDR_PAGE_SIZE_OFFSET, sizeof(unsigned int));
	uint_to_big_endian(0, first_pg->pg_buf_start + PAGER_HDR_FIRST_FREE_OFFSET, sizeof(unsigned int));

	pager_release_page(first_pg);

	assert(0 == pager->ref_count);
	pager_syn(pager);

	return 0;
}

/**
 * @brief 读取页文件信息,读取页大小,以及包含多少页,并判断文件版本
 */
static __INLINE__ int pager_read_file_page_info(pager_t * pager)
{
	/*
	* 页的第一页用于存放页文件本身的信息
	* 第一页
	*  offset bytes        des
	*  0      16           格式版本
	*  16     4            页大小
	*  20     4            第一个空闲页
	*  24     40           保留
	*  64   pg_sz - 64     用户数据     
	*/
	char ver[sizeof(VERSION)];
	unsigned int pg_sz = 0;
	int64 pg_file_sz = 0;

	assert(pager && pager->hf);

	/* 判断页文件大小是否满足一页,否则需要写入页文件头信息 */
	if(0 != OsFileSize(pager->hf, &pg_file_sz))
		return -1;

	if(pg_file_sz < sizeof(ver))
	{
		/* 无法读到足够的信息,则应认为此页文件是无效的,需要写入头信息 */
		if(0 != pager_write_file_page_info(pager))
			return -1;

		return 0;
	}

	if(0 != OsFileSeek(pager->hf, 0))
		return -1;

	if(0 != OsFileRead(pager->hf, ver, sizeof(ver), NULL))
		return -1;

	if(strncmp(ver, VERSION, strlen(VERSION)) != 0)
	{
		/* 页文件格式可能是当前程序不支持的 */
		LOG_WARN(("page file version err,this page maybe create by the later pager verion,[file version:%s], [program ver:%s]",
			ver, VERSION));

		return -1;
	}

	if(0 != OsFileSeek(pager->hf, PAGER_HDR_VER_LEN))
		return -1;

	/* 读出页的大小 */
	if(0 != pager_read_uint_from_file(pager->hf, &pg_sz))
		return -1;

	if(pg_file_sz < pg_sz)
	{
		/* 
		* 如果页文件的大小于一页的大小 
		* 认为是一个新的页文件,需要写入页文件头信息
		*/
		if(0 != pager_write_file_page_info(pager))
			return -1;
	}
	else
	{
		pager->page_size = pg_sz;
	}

	/* 获取当前有多少页 */
	pager->total_page_count = pager_get_total_pages_count(pager);

	return 0;
}


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
HPAGER PagerConstruct(HMYMEMPOOL hm,
					  const char * file_name,
					  PAGE_RELEASE page_release_cb,
					  PAGE_RELOAD page_reload_cb,
					  unsigned int extra_size,
					  unsigned int max_cache_pages,
					  unsigned int * page_size,
					  void * rand_seed,
					  unsigned int rand_seed_size,
					  unsigned int sector_size)
{
	pager_t * pager = NULL;

	if(NULL == page_size || NULL == file_name)
	{
		LOG_WARN(("param err,page_size:%x, file_name:%x", page_size, file_name));
		return NULL;
	}

	pager = MyMemPoolMalloc(hm, sizeof(*pager));
	if(NULL == pager)
	{
		LOG_WARN(("fail malloc"));
		return NULL;
	}

	memset(pager, 0 ,sizeof(*pager));

	pager->hm = hm;

	/* 保存日志文件名与外存文件名 */
	pager->hb_file_name = MyBufferConstruct(hm, strlen(file_name) + 1);
	pager->hb_jf_name = MyBufferConstruct(hm, strlen(file_name) + strlen(JOURNAL) + 1);
	pager->ref_count = 0;

	if(NULL == pager->hb_file_name || NULL == pager->hb_jf_name)
	{
		LOG_WARN(("fail malloc"));
		goto PagerConstructor_err_;
	}

	MyBufferSet(pager->hb_file_name, file_name, strlen(file_name));
	MyBufferAppend(pager->hb_file_name, "\0", 1);

	MyBufferSet(pager->hb_jf_name, file_name, strlen(file_name));
	MyBufferAppend(pager->hb_jf_name, JOURNAL, strlen(JOURNAL));
	MyBufferAppend(pager->hb_jf_name, "\0", 1);

	pager->hjf = NULL;
	/* 打开外存文件 */
	pager->hf = OsFileOpenReadWrite(file_name, hm);
	if(NULL == pager->hf)
	{
		LOG_WARN(("fail open file : %s", file_name));
		goto PagerConstructor_err_;
	}
	pager->page_file_is_integrity = 1;

	pager->journal_end = 0;
	pager->jouranl_prev_hdr = 0;

	/* 创建随机数发生器 */
	pager->hrand = myrandConstruct(hm, rand_seed, rand_seed_size, 0);
	if(NULL == pager->hrand)
		goto PagerConstructor_err_;
	pager->cur_cksum_init = 0;

	pager->pg_free_first = NULL;
	pager->pg_free_last = NULL;

	pager->pg_syncjf_first = NULL;
	pager->pg_syncjf_last = NULL;

	pager->orig_pages_count = (unsigned int)-1;
	pager->need_sync_journal = 0;
	pager->in_journal_pages_count = 0;
	pager->bsync_journal_has_done = 0;

	pager->pg_all = NULL;
	pager->pg_dirty = NULL;

	/* 页缓存哈希表初始化 */
	pager->hash_page = MyHashTableConstruct(hm, pg_hash_fun, pg_hash_keyequl_fun, 0);
	if(NULL == pager->hash_page)
		goto PagerConstructor_err_;

	if(max_cache_pages)
		pager->max_cache_pages = max_cache_pages;
	else
		pager->max_cache_pages = DEFAULT_CACHE_PAGES_COUNT;

	/* 页是否处于journal状态的位着色表 */
	pager->hash_pgno_journal = MyHashTableConstruct(hm, pg_hash_fun, pg_hash_keyequl_fun, 0);
	if(NULL == pager->hash_pgno_journal)
		goto PagerConstructor_err_;

	if(sector_size > PAGER_MIN_SECTOR_SIZE)
		pager->sector_size = sector_size;
	else
		pager->sector_size = PAGER_MIN_SECTOR_SIZE;

	pager->extra_size = extra_size;

	/* 页的加载与取消引用时的回调函数 */
	pager->page_release_cb = page_release_cb;
	pager->page_reload_cb = page_reload_cb;

	/* 每页大小 */
	assert(page_size);
	pager->page_size = *page_size;
	if(pager->page_size < MIN_PAGE_SIZE)
		pager->page_size = MIN_PAGE_SIZE;
	else if(pager->page_size > MAX_PAGE_SIZE)
		pager->page_size = MAX_PAGE_SIZE;

	/* 获取页的大小,以及有多少页 */
	pager->total_page_count = (unsigned int)-1;
	if(-1 == pager_read_file_page_info(pager))
		goto PagerConstructor_err_;

	/* 修正用户传进来的页大小 */
	*page_size = PAGER_PAGE_USER_SIZE(pager);

	assert(pager_get_total_pages_count(pager) >= PAGER_FIRST_PAGE_NO);

	return pager;

PagerConstructor_err_:

	if(pager)
		pager_destroy(pager);

	return NULL;
}

/**
 * @brief 销毁一个缓存页管理器
 */
void PagerDestruct(HPAGER hpgr)
{
	if(hpgr)
		pager_destroy(hpgr);
}

/**
 * @brief 同步所有脏缓存页
 */
int PagerSyn(HPAGER hpgr)
{
	if(NULL == hpgr)
		return -1;

	if(0 != pager_syn(hpgr))
		return -1;

	return 0;
}

/**
 * @brief 取消所有的页的更改
 */
int PagerRollBack(HPAGER hpgr)
{
	if(NULL == hpgr)
		return -1;

	return pager_rollback(hpgr);
}

/**
 * @brief 判断某一页是否为空闲,防止重复释放
 * @return 0:成功, -1:失败
 * @param pbfree:返回值 1:表示空闲, 0:表示正在使用
 */
static __INLINE__ int pager_judge_page_is_free(pager_t * pager, unsigned int pgno, int * pbfree)
{
	pghd_t * pg = NULL;
	assert(pbfree);

	assert(pager);

	pg = pager_get_page(pager, pgno);
	if(NULL == pg)
		return -1;

	*pbfree = *(pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET) ? 0: 1;

	pager_release_page(pg);

	assert(NULL != pager_hash_lookup(pager,  pgno) && pager_page_is_in_all_list(pg));

	return 0;
}

/**
 * @brief 将某一页做标记,空闲还是非空闲
 */
#define pager_write_page_free_flag(__pg, __f) do{\
		(__pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET)[0] = (__f);\
		(__pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET)[1] = 0xff;\
		(__pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET)[2] = 0xff;\
		(__pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET)[3] = 0xff;\
	}while(0)

/**
 * @brief 将某一页做标记,空闲还是非空闲
 */
static int pager_mark_page_is_free_or_no(pager_t * pager, unsigned int pgno, int bfree)
{
	int ret = 0;
	pghd_t * pg = NULL;

	assert(pager);

	pg = pager_get_page(pager, pgno);
	if(NULL == pg)
		return -1;

	assert((bfree && *(pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET)) || !bfree);
	assert((!*(pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET) && !bfree) || bfree);

	if(pager_make_page_writable(pg))
		pager_write_page_free_flag(pg, (bfree ? 0x00: 0xff));
	else
		ret = -1; 

	pager_release_page(pg);

	assert(NULL != pager_hash_lookup(pager,  pgno) && pager_page_is_in_all_list(pg));

	return ret;
}

/**
 * @brief 申请一个页号
 */
static unsigned int pager_get_page_no(pager_t * pager)
{
	unsigned int first_free_no = 0;
	unsigned int ret_pg_no = 0;
	pghd_t * first_pg = NULL;

	/*
	* 首先从页文件的第一页寻找第一个空闲页的页号.
	* 如果空闲页存在,则取出第一个空闲页(如果用于管理的空闲页没有任子页,则更改第一个空闲页的索引至下一个空闲页),
	* 相应的页应要做写入操作(备份至jfd,等提交时真正写入),第一页(有可能要写),第一个主空闲页(一定要写)
	* 这些页需要被加载进缓存
	* 如果不存在任何空闲页,意味着页文件需要增长了,计算页文件总共有多少页,其值加1则为新的页号
	* 找到页号的缓存与页缓存进入哈希表(如果不存在),并将缓存的内容置成0
	* 将此页的第一个字节置成0xff表示此页已被使用.
	*
	* todo 如果此页,当前处于jf机制中,则需要判断是否需要记录超过原始页文件大小的页
	*/

	assert(pager && pager->hf);

	/*
	* 外存页文件格式描述
	* 第一页
	*  offset bytes        des
	*  0      16           格式版本
	*  16     4            页大小
	*  20     4            第一个空闲页
	*  24     40           保留
	*  64   pg_sz - 64     用户数据
	*/

	/* 获取第一页 */
	first_pg = pager_get_page(pager, PAGER_FIRST_PAGE_NO);
	if(NULL == first_pg)
		return 0;

	array_to_uint_as_big_endian(first_pg->pg_buf_start + PAGER_HDR_FIRST_FREE_OFFSET,
		PAGER_HDR_FIRST_FREE_LEN, 
		first_free_no);

	assert(PAGER_FIRST_PAGE_NO != first_free_no);

	if(0 == first_free_no)
	{
		/*
		* 没有空闲页
		* 取页文件总页数加1
		*/
		ret_pg_no = pager_get_total_pages_count(pager) + 1;

		/* 标记成非空闲页 */
		if(0 != pager_mark_page_is_free_or_no(pager, ret_pg_no, 0))
			ret_pg_no = 0;
		else
			assert(ret_pg_no == pager_get_total_pages_count(pager));
	}
	else/* 如果空闲页存在 */
	{
		/*
		* 空闲页:
		* 第一个空闲页
		* offset bytes   des
		* 0      4       总的空闲页数
		* 4      4       空闲页"链表"的下一个空闲页号
		* 8      4       当前页里记载了多少的空闲页号
		* 12             空闲数组开始,每4个字节表示一个空闲页号
		* 非第一个空闲页
		* 0      4       无意义
		* 4      4       空闲页"链表"的下一个空闲页号
		* 8      4       当前页里记载了多少的空闲页号
		* 12             空闲数组开始,每4个字节表示一个空闲页号
		*/

		unsigned int free_count = 0;
		unsigned int total_free_count = 0;
		pghd_t * pg_first_free = pager_get_page(pager, first_free_no);

		if(NULL == pg_first_free)
		{
			ret_pg_no = 0;
			goto pager_get_page_no_end_;
		}

		array_to_uint_as_big_endian((unsigned char *)pager_get_page_bufdata(pg_first_free) + PAGER_FREE_TOTAL_FREE_COUNT_OFFSET,
			sizeof(unsigned int),
			total_free_count);

		array_to_uint_as_big_endian((unsigned char *)pager_get_page_bufdata(pg_first_free) + PAGER_FREE_FREE_COUNT_OFFSET,
			sizeof(unsigned int),
			free_count);

		if(free_count)
		{
			/* 空闲页数组里有元素,从中取一个分配给用户 */
			array_to_uint_as_big_endian(pager_get_page_bufdata(pg_first_free) + PAGER_FREE_FREE_PGNO_ARRAY_OFFSET + (free_count - 1) * sizeof(unsigned int),
				sizeof(ret_pg_no),
				ret_pg_no);

			assert(ret_pg_no);

			if(NULL == pager_make_page_writable(pg_first_free))
			{
				pager_release_page(pg_first_free);
				ret_pg_no = 0;
				goto pager_get_page_no_end_;
			}

			/* 将得到的空闲页标记成非空闲 */
			if(0 != pager_mark_page_is_free_or_no(pager, ret_pg_no, 0))
			{
				pager_release_page(pg_first_free);
				ret_pg_no = 0;
				goto pager_get_page_no_end_;
			}

			/* 改变当前空闲页数组的大小 */
			free_count -= 1;
			uint_to_big_endian(free_count, pager_get_page_bufdata(pg_first_free) + PAGER_FREE_FREE_COUNT_OFFSET, sizeof(unsigned int));

			assert(total_free_count);
			total_free_count -= 1;
			uint_to_big_endian(total_free_count, pager_get_page_bufdata(pg_first_free) + PAGER_FREE_TOTAL_FREE_COUNT_OFFSET, sizeof(unsigned int));
		}
		else
		{
			/* 没有元素,要将此页分配给用户 */

			unsigned int next_free_link = 0;

			ret_pg_no = first_free_no;

			array_to_uint_as_big_endian(pager_get_page_bufdata(pg_first_free) + PAGER_FREE_NEXT_FREE_LINK_OFFSET,
				sizeof(next_free_link),
				next_free_link);

			if(0 == next_free_link)
			{
				/* 要在外存的第一页里改写第一个空闲页的页号置成0 */
				if(NULL == pager_make_page_writable(first_pg))
				{
					pager_release_page(pg_first_free);
					ret_pg_no = 0;
					goto pager_get_page_no_end_;
				}

				/* 将得到的空闲页标记成非空闲 */
				if(0 != pager_mark_page_is_free_or_no(pager, ret_pg_no, 0))
				{
					pager_release_page(pg_first_free);
					ret_pg_no = 0;
					goto pager_get_page_no_end_;
				}

				uint_to_big_endian(0, first_pg->pg_buf_start + PAGER_HDR_FIRST_FREE_OFFSET, PAGER_HDR_FIRST_FREE_LEN);
			}
			else
			{
				pghd_t * pg_next_free_lnk = NULL;

				if(NULL == pager_make_page_writable(first_pg))
				{
					pager_release_page(pg_first_free);
					ret_pg_no = 0;
					goto pager_get_page_no_end_;
				}

				/* 在新的第一个空闲页里记录总共有多少空闲页 */
				pg_next_free_lnk = pager_get_page(pager, next_free_link);
				if(NULL == pg_next_free_lnk)
				{
					pager_release_page(pg_first_free);
					pager_release_page(pg_next_free_lnk);
					ret_pg_no = 0;
					goto pager_get_page_no_end_;
				}

				if(NULL == pager_make_page_writable(pg_next_free_lnk))
				{
					pager_release_page(pg_first_free);
					pager_release_page(pg_next_free_lnk);
					ret_pg_no = 0;
					goto pager_get_page_no_end_;
				}

				/* 将页置成非空闲 */
				if(0 != pager_mark_page_is_free_or_no(pager, ret_pg_no, 0))
				{
					pager_release_page(pg_first_free);
					pager_release_page(pg_next_free_lnk);
					ret_pg_no = 0;
					goto pager_get_page_no_end_;
				}

				assert(total_free_count);
				total_free_count -= 1;
				uint_to_big_endian(total_free_count, pager_get_page_bufdata(pg_next_free_lnk) + PAGER_FREE_TOTAL_FREE_COUNT_OFFSET, sizeof(unsigned int));

				/* 要在外存的第一页里改写第一个空闲页的页号置成新的第一个空闲页页号 */
				uint_to_big_endian(next_free_link, first_pg->pg_buf_start + PAGER_HDR_FIRST_FREE_OFFSET, PAGER_HDR_FIRST_FREE_LEN);

				pager_release_page(pg_next_free_lnk);
			}
		}

		pager_release_page(pg_first_free);
	}

pager_get_page_no_end_:

	pager_release_page(first_pg);

	return ret_pg_no;
}

/**
 * @brief 申请一个页号
 * @return 返回一个可用的空闲页号
 */
unsigned int PagerGetPageNo(HPAGER hpgr)
{
	if(NULL == hpgr)
		return 0;

	return pager_get_page_no(hpgr);
}

/**
 * @brief 计算一页可以容纳多少个页号
 */
#define pager_cal_pgno_count(__pgr) ((PAGER_PAGE_USER_SIZE(__pgr) - PAGER_FREE_FREE_PGNO_ARRAY_OFFSET)/sizeof(unsigned int))

/**
 * @brief 将一个空闲页加入空闲页"链表"的"头部"
 */
static __INLINE__ int pager_add_free_to_list(pager_t * pager,
											 pghd_t * first_page,
											 unsigned int pgno,
											 unsigned int current_first_free,
											 unsigned int current_total_free)
{
	int ret = 0;
	pghd_t * pg_release = NULL;

	assert(pager && first_page);
	assert((current_first_free == 0 && current_total_free == 0) || (current_first_free && current_total_free));

	pg_release = pager_get_page(pager, pgno);
	if(NULL ==	pg_release)
		return -1;

	if(NULL == pager_make_page_writable(pg_release))
	{
		ret = -1;
		goto pager_add_free_to_list_end_;
	}

	/* 将此页作为第一个空闲页号写入第一页 */
	if(NULL == pager_make_page_writable(first_page))
	{
		ret = -1;
		goto pager_add_free_to_list_end_;
	}

	uint_to_big_endian(pgno,
		first_page->pg_buf_start + PAGER_HDR_FIRST_FREE_OFFSET,
		sizeof(unsigned int));

	/* 将相关的信息写入此空闲页 */
	assert(*(pg_release->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET));
	pager_write_page_free_flag(pg_release, 0x00);

	uint_to_big_endian(current_total_free + 1,
		pager_get_page_bufdata(pg_release) + PAGER_FREE_TOTAL_FREE_COUNT_OFFSET,
		sizeof(unsigned int));

	uint_to_big_endian(current_first_free,
		pager_get_page_bufdata(pg_release) + PAGER_FREE_NEXT_FREE_LINK_OFFSET,
		sizeof(unsigned int));

	uint_to_big_endian(0,
		pager_get_page_bufdata(pg_release) + PAGER_FREE_FREE_COUNT_OFFSET,
		sizeof(unsigned int));

pager_add_free_to_list_end_:

	pager_release_page(pg_release);

	return ret;
}

/**
 * @brief 释放一个页号,将页号置入空闲页,但不对空闲页页号进行排序
 */
static __INLINE__ int pager_release_pgno_no_sort(pager_t * pager, unsigned int pgno)
{
	/*
	* 如果没有,则将空闲页号填入主空闲页即可,如果主空闲页满了,则扩展空闲页"链表",改变主空闲页链表
	* 主空闲需要写入(如果未满), 如果主空闲页满了:第一页需要写入,空闲页->主空闲页(即空闲页本身需要写入)
	*/

	int ret = 0;
	unsigned int first_free_no = 0;
	pghd_t * first_page = NULL;

	assert(pager && pgno);
	assert(pgno <= pager_get_total_pages_count(pager));

	first_page = pager_get_page(pager, PAGER_FIRST_PAGE_NO);
	if(NULL == first_page)
		return -1;

	array_to_uint_as_big_endian(first_page->pg_buf_start + PAGER_HDR_FIRST_FREE_OFFSET,
		PAGER_HDR_FIRST_FREE_LEN, 
		first_free_no);

	/* 如果空闲页不存在,将此页作为第一个空闲页 */
	if(0 == first_free_no)
	{
		ret = pager_add_free_to_list(pager, first_page, pgno, 0, 0);
	}
	else
	{
		unsigned int free_count = 0;
		unsigned int total_free_count = 0;

		pghd_t * pg_first_free = pager_get_page(pager, first_free_no);

		array_to_uint_as_big_endian(pager_get_page_bufdata(pg_first_free) + PAGER_FREE_TOTAL_FREE_COUNT_OFFSET,
			sizeof(unsigned int),
			total_free_count);

		if(NULL == pg_first_free)
		{
			ret = -1;
			goto pager_release_pgno_no_sort_end_;
		}

		array_to_uint_as_big_endian(pager_get_page_bufdata(pg_first_free) + PAGER_FREE_FREE_COUNT_OFFSET,
			sizeof(unsigned int),
			free_count);

		if(free_count < pager_cal_pgno_count(pager))
		{
			if(NULL == pager_make_page_writable(pg_first_free))
			{
				pager_release_page(pg_first_free);
				ret = -1;
				goto pager_release_pgno_no_sort_end_;
			}

			if(0 != pager_mark_page_is_free_or_no(pager, pgno, 1))
			{
				pager_release_page(pg_first_free);
				ret = -1;
				goto pager_release_pgno_no_sort_end_;
			}

			/* 将页号写进主空闲页数组 */
			uint_to_big_endian(pgno,
				pager_get_page_bufdata(pg_first_free) + PAGER_FREE_FREE_PGNO_ARRAY_OFFSET + free_count * sizeof(unsigned int),
				sizeof(unsigned int));

			/* 主空闲页的数组个数加1 */
			free_count ++;
			uint_to_big_endian(free_count,
				pager_get_page_bufdata(pg_first_free) + PAGER_FREE_FREE_COUNT_OFFSET,
				sizeof(unsigned int));

			/* 空闲页总数加1 */
			total_free_count ++;
			uint_to_big_endian(total_free_count,
				pager_get_page_bufdata(pg_first_free) + PAGER_FREE_TOTAL_FREE_COUNT_OFFSET,
				sizeof(unsigned int));
		}
		else
		{
			ret = pager_add_free_to_list(pager, first_page, pgno, first_free_no, total_free_count);
		}

		pager_release_page(pg_first_free);
	}

pager_release_pgno_no_sort_end_:

	pager_release_page(first_page);

	return ret;
}

/**
 * @brief 释放一个页号,将置成空闲页
 */
static __INLINE__ int pager_release_pgno(pager_t * pager, unsigned int pgno)
{
	/*
	* 将页的第一个字节置成0,表示此页不再使用了
	* 判断是否打开的自去除空闲页的机制
	*  如果没有,则将空闲页号填入主空闲页即可,如果主空闲页满了,则扩展空闲页"链表",改变主空闲页链表
	*      主空闲需要写入(如果未满), 如果主空闲页满了:第一页需要写入,空闲页->主空闲页(即空闲页本身需要写入)
	*  如果有,
	*      则需要对空闲页进行页号排序.保证页号小的空闲页排在前面,保证空闲页整理的效率.当释放一页时,沿着"链表"查找,要合适的页添加
	*      如果页已满,应按页号跨度最大的原则分裂该页.
	*         每释放一页,空闲页计数应增加一,并写回外存页文件.
	*         当设置了自动碎片整理时,用于管理的空闲页的页号应是它所记载的空闲页中最大的(在分配置时可以减少写入,会导致释放时多读一次"链表").
	*      未满:页号添加至的那个空闲页, 如已满:还需写分裂后新的承载页
	* 如果设置了自动truncate,此时判断是否达到了百分比,如果达到了,truncate开始执行.
	*/
	int bfree = 0;

	assert(pager && pgno);
	assert(pgno != PAGER_FIRST_PAGE_NO);
	assert(pgno <= pager_get_total_pages_count(pager));

	if(0 != pager_judge_page_is_free(pager, pgno, &bfree))
		return -1;

	if(bfree)
		return 0;

	if(0 != pager_release_pgno_no_sort(pager, pgno))
		return -1;

	return 0;
}

/**
 * @brief 释放一个页号,将置成空闲页
 */
int PagerReleasePageNo(HPAGER hpgr, unsigned int pgno)
{
	if(NULL == hpgr || 0 == pgno)
		return -1;

	if(pgno > pager_get_total_pages_count(hpgr) || pgno == PAGER_FIRST_PAGE_NO)
		return -1;

	return pager_release_pgno(hpgr, pgno);
}

/**
 * @brief 获取当前页文件中有多少页
 */
unsigned int PagerGetTotalPagesCount(HPAGER hpgr)
{
	if(NULL == hpgr)
		return 0;

	return pager_get_total_pages_count(hpgr);
}

/**
 * @brief 获取当前页文件中有多少空闲页
 */
unsigned int PagerGetFreePages(HPAGER hpgr)
{
	/**
	* 第一页
	*  offset bytes        des
	*  0      16           格式版本
	*  16     4            页大小
	*  20     4            第一个空闲页
	*  24     40           保留
	*  64   pg_sz - 64     用户数据     
	*
	* 空闲页:
	* 第一个空闲页
	* offset bytes   des
	* 0      4       总的空闲页数
	* 4      4       空闲页"链表"的下一个空闲页号
	* 8      4       当前页里记载了多少的空闲页号
	* 12             空闲数组开始,每4个字节表示一个空闲页号
	*/

	pghd_t * first_pg = NULL;
	pghd_t * first_free_pg = NULL;
	unsigned int first_free_no = 0;
	unsigned int free_pg_count = 0;

	if(NULL == hpgr)
		return 0;

	first_pg = pager_get_page(hpgr, PAGER_FIRST_PAGE_NO);
	if(NULL == first_pg)
		return 0;

	array_to_uint_as_big_endian(first_pg->pg_buf_start + PAGER_HDR_FIRST_FREE_OFFSET,
		PAGER_HDR_FIRST_FREE_LEN, 
		first_free_no);

	if(0 == first_free_no)
		goto PagerGetFreePages_end_;

	first_free_pg = pager_get_page(hpgr, first_free_no);
	if(NULL == first_free_pg)
		goto PagerGetFreePages_end_;

	array_to_uint_as_big_endian((unsigned char *)pager_get_page_bufdata(first_free_pg) + PAGER_FREE_TOTAL_FREE_COUNT_OFFSET,
		sizeof(unsigned int),
		free_pg_count);

PagerGetFreePages_end_:

	if(first_pg)
		pager_release_page(first_pg);

	if(first_free_pg)
		pager_release_page(first_free_pg);

	return free_pg_count;
}

/**
 * @brief 从页缓存管理中获取一页 与PagerReleasePage是对偶的操作
 */
HPAGE_HEAD PagerGetPage(HPAGER hpgr, unsigned int pgno)
{
	if(NULL == hpgr || pgno == 0 /*|| pgno <= PAGER_FIRST_PAGE_NO*/)
		return NULL;

	if(pgno > pager_get_total_pages_count(hpgr))
		return NULL;

	return pager_get_page(hpgr, pgno);
}

/**
 * @brief 对一个页缓存引用计数增加,需要用PagerReleasePage降低引用引数
 */
int PageHeadRef(HPAGE_HEAD pg)
{
	if(NULL == pg)
		return -1;

	assert(pg->pager);

	pager_ref_page(pg->pager, pg);

	return 0;
}

/**
 * @brief 从缓存里引用指定页号的页缓存,如该页不在缓存里,返回NULL
 */
HPAGE_HEAD PagerGetCachedPage(HPAGER hpgr, unsigned int pgno)
{
	pghd_t * pg = NULL;

	if(NULL == hpgr || pgno == 0 /*|| pgno <= PAGER_FIRST_PAGE_NO*/)
		return NULL;

	if(pgno > pager_get_total_pages_count(hpgr))
		return NULL;

	pg = pager_hash_lookup(hpgr,  pgno);
	if(NULL == pg)
		return NULL;

	pager_ref_page(hpgr, pg);

	return pg;
}

/**
 * @brief 取消对当前页的引用 与PagerGetPage是对偶的操作
 */
int PagerReleasePage(HPAGE_HEAD pg)
{
	if(NULL == pg || NULL == pg->pager)
		return -1;

	if(pg->ref_count)
		return pager_release_page(pg);
	
	return 0;
}

/**
 * @brief 获取页内容的缓冲区,用于写入
 */
void * PageHeadMakeWritable(HPAGE_HEAD pg)
{
	if(NULL == pg || NULL == pg->pager)
		return NULL;

	return pager_make_page_writable(pg);
}

/**
 * @brief 获取某一个页内容,用于读取
 */
const void * PageHeadMakeReadable(HPAGE_HEAD pg)
{
	if(NULL == pg || NULL == pg->pager)
		return NULL;

	return pager_get_page_bufdata(pg);
}

/**
 * @brief 获取某一页的用户数据
 */
void * PageHeadGetUserData(HPAGE_HEAD pg)
{
	if(NULL == pg)
		return NULL;

	return pg->pg_buf_start + pager_get_journal_rec_len(pg->pager);
}

/**
 * @brief 获取某一页的用户数据的长度
 */
unsigned PageHeadGetUserDataSize(HPAGE_HEAD pg)
{
	if(NULL == pg || NULL == pg->pager)
		return 0;

	return pg->pager->extra_size;
}

/**
 * @brief 检查页缓存的状态
 */
unsigned int PagerGetRefCount(HPAGER hpgr)
{
	assert(hpgr);

	return hpgr->ref_count;
}

/**
 * @brief 获取某一页缓存的引用计数
 */
unsigned int PagerGetPageRefCount(HPAGE_HEAD pg)
{
	assert(pg);

	return pg->ref_count;
}

/**
 * @brief 检查某个指定是否越界了
 */
int PagePtrIsInpage(HPAGE_HEAD pg, const void * ptr)
{
	if(NULL == pg)
		return 0;

	assert((unsigned char *)ptr >= pager_get_page_bufdata(pg) && (unsigned char *)ptr <= (pg->pg_buf_start + pg->pager->page_size));
	return ((unsigned char *)ptr >= pager_get_page_bufdata(pg) && (unsigned char *)ptr <= (pg->pg_buf_start + pg->pager->page_size));
}


/**
 * @brief for debug,检查jf文件是否合理
 */
void pager_check_jf(HPAGER hpgr, void * buf, unsigned int buf_sz)
{
#define temp_pager_locate_jf_hdr_pos(jend, ssz) \
	((jend % ssz)?(jend + CAL_ALIGMENT(jend, ssz)):jend)

	unsigned int i = 0;
	unsigned int data_buf_len = 0;
	unsigned char * data_buf = NULL;
	unsigned int total_in_jouranl_count = 0;
	int64 jsz = 0;

	if(NULL == hpgr->hjf)
		return;

	assert(hpgr);

	OsFileSize(hpgr->hjf, &jsz);
	assert(hpgr->journal_end == jsz);
	assert(hpgr->jouranl_prev_hdr % hpgr->sector_size == 0);

	data_buf = malloc(pager_get_journal_rec_len(hpgr));
	data_buf_len = pager_get_journal_rec_len(hpgr);

	while(i < hpgr->journal_end)
	{
		unsigned int in_journal_count = 0;
		unsigned int cksum_init = 0;
		unsigned int orig_pages_count = 0;
		unsigned int sector_size = 0;
		unsigned char magic[JOURNAL_MAGIC_LEN] = {0};
		unsigned int j = 0;

		int64 offset = temp_pager_locate_jf_hdr_pos(i, hpgr->sector_size);

		if(i + PAGER_JOURNAL_REC_COUNT_LEN >= jsz)
			break;

		assert(0 == OsFileSeek(hpgr->hjf, offset));

		/* 读出页头的大小 */
		assert(0 == pager_read_uint_from_file(hpgr->hjf, &sector_size));

		assert(sector_size == hpgr->sector_size);

		if(sector_size < PAGER_MIN_SECTOR_SIZE)
			break;

		if(i + PAGER_JOURNAL_REC_COUNT_LEN >= jsz)
			break;

		/* 读出magic字符串,判断它是否合法 */
		assert(0 == OsFileRead(hpgr->hjf, magic, sizeof(magic), NULL));

		assert(memcmp(magic, JOURNAL_MAGIC_STRING, JOURNAL_MAGIC_LEN) == 0);

		/* 读出日志里备份了几页 */
		assert(0 == pager_read_uint_from_file(hpgr->hjf, &in_journal_count));


		/* 读出校验和初始值 */
		assert(0 == pager_read_uint_from_file(hpgr->hjf, &cksum_init));

		/* 读出备份前的页文件大小 */
		assert(0 == pager_read_uint_from_file(hpgr->hjf, &orig_pages_count));

		i = (unsigned int)(offset + sector_size);

		/* 定位文件指针位置 */
		assert(0 == OsFileSeek(hpgr->hjf, i));

		/* 如果是最后一个jf头,并且记录数显示为零,则有可能jf还未同步 */
		if(0 == in_journal_count && i == (hpgr->jouranl_prev_hdr + hpgr->sector_size))
		{
			/* 说明此时有可能jf本身还同步,剩余的脏页可以直接于页文件里取,因为没有写回 */
			assert(i + hpgr->in_journal_pages_count * pager_get_journal_rec_len(hpgr) == jsz);

			if(i < jsz)
				assert(hpgr->need_sync_journal);

			in_journal_count = hpgr->in_journal_pages_count;
		}

		for(j = 0; j < in_journal_count; j ++)
		{
			unsigned int cksum = 0;
			unsigned int pg_no;

			assert(0 == OsFileRead(hpgr->hjf, data_buf, data_buf_len, NULL));

			total_in_jouranl_count ++ ;
			i += data_buf_len;

			/* 计算校验和,判断此页是否是正确的 */
			array_to_uint_as_big_endian(data_buf + hpgr->page_size + sizeof(unsigned int), sizeof(unsigned int), cksum);
			assert(cksum == pager_cal_page_cksum(data_buf, hpgr->page_size, cksum_init));

			/* 在页缓存哈希表里查找 */
			array_to_uint_as_big_endian(data_buf + hpgr->page_size, sizeof(unsigned int), pg_no);
			assert(MyHashTableSearch(hpgr->hash_pgno_journal, (void *)pg_no));

			/*
			* 验证页里的内容是否是原版的,呵呵
			* 测试请写入page,否则这里通不过哦
			*/
			if(pg_no != PAGER_FIRST_PAGE_NO)
			{
				if(buf && buf_sz)
					assert(memcmp(data_buf + 4, buf, buf_sz) == 0);
			}
			else
				assert(memcmp(data_buf, "Rhapsody", 8) == 0);
		}
	}

	OsFileSeek(hpgr->hjf, hpgr->journal_end);

	free(data_buf);
}

/**
 * @brief 检查外存空闲页"链表"是否正确
 */
void pager_check_free(HPAGER hpgr)
{
	unsigned int first_free = 0;
	HPAGE_HEAD pg1 = NULL;

	assert(hpgr && hpgr->hf);

	pg1 = pager_get_page(hpgr, PAGER_FIRST_PAGE_NO);
	assert(pg1);

	array_to_uint_as_big_endian(pg1->pg_buf_start + PAGER_HDR_FIRST_FREE_OFFSET,
		sizeof(unsigned int),
		first_free);

	if(0 == first_free)
	{
		unsigned int i = 0;
		unsigned int total_page_count = PagerGetTotalPagesCount(hpgr);

		/* 每一页外存的used标识应为0xff */
		for(i = 1; i <= total_page_count; i ++)
		{
			HPAGE_HEAD pg = NULL;

			if(i == PAGER_FIRST_PAGE_NO)
				continue;

			pg = PagerGetPage(hpgr, i);
			assert(pg);

			/* 检查空闲标识位 */
			assert(*(pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET) == 0xff);

			PagerReleasePage(pg);
		}
	}
	else
	{
		unsigned int total_free_pages_count = 0;/* 从第一个空闲页里读取出来的空闲页总数 */
		unsigned int lnk_free_pages_count = 0;/* 从空闲页表里统计出来的空闲页总数 */
		unsigned int real_free_pages_count = 0;/* 从外存页文件里统计出来的空闲页总数 */

		unsigned int current_free = first_free;

		HPAGE_HEAD pg_first_free = PagerGetPage(hpgr, current_free);
		assert(pg_first_free);
		array_to_uint_as_big_endian(pager_get_page_bufdata(pg_first_free) + PAGER_FREE_TOTAL_FREE_COUNT_OFFSET,
			sizeof(unsigned int),
			total_free_pages_count);
		PagerReleasePage(pg_first_free);

		while(current_free)
		{
			unsigned int current_node_free_count = 0;
			unsigned int i = 0;
			HPAGE_HEAD pg = PagerGetPage(hpgr, current_free);
			assert(pg);
			array_to_uint_as_big_endian(pager_get_page_bufdata(pg) + PAGER_FREE_FREE_COUNT_OFFSET,
				sizeof(unsigned int),
				current_node_free_count);

			lnk_free_pages_count += (current_node_free_count + 1);

			for(i = 0; i < current_node_free_count; i ++)
			{
				HPAGE_HEAD pg_free_in_node = NULL;
				unsigned int free_pgno_in_node = 0;
				array_to_uint_as_big_endian(pager_get_page_bufdata(pg) + PAGER_FREE_FREE_PGNO_ARRAY_OFFSET + i * sizeof(unsigned int),
					sizeof(unsigned int),
					free_pgno_in_node);

				pg_free_in_node = PagerGetPage(hpgr, free_pgno_in_node);
				assert(pg_free_in_node);

				/* 检查空闲标识位 */
				assert(!*(pg_free_in_node->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET));

				PagerReleasePage(pg_free_in_node);
			}

			array_to_uint_as_big_endian(pager_get_page_bufdata(pg) + PAGER_FREE_NEXT_FREE_LINK_OFFSET,
				sizeof(unsigned int),
				current_free);

			PagerReleasePage(pg);
		}
		assert(lnk_free_pages_count == total_free_pages_count);

		{
			unsigned int i = 0;
			unsigned int total_pages_count = pager_get_total_pages_count(hpgr);
			/* 循环统计所有的外存页文件 */
			for(i = 1; i <= total_pages_count; i ++)
			{
				HPAGE_HEAD pg = NULL;

				if(i == PAGER_FIRST_PAGE_NO)
					continue;

				pg = PagerGetPage(hpgr, i);
				assert(pg);

				/* 检查空闲标识位 */
				if(!*(pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET))
					real_free_pages_count ++;

				PagerReleasePage(pg);
			}

			assert(real_free_pages_count == total_free_pages_count);
		}
	}

	pager_release_page(pg1);
}

/**
 * @brief 检查页缓存的状态
 */
void PagerExamin(HPAGER hpgr, int check_jf, int check_free, void * buf, unsigned int buf_sz)
{
	pghd_t * pg = NULL;
	unsigned int pg_count = 0;
	unsigned int ref_count = 0;

	assert(hpgr);

	/* 检查free链表 */
	for(pg = hpgr->pg_free_first; pg; pg = pg->pg_free_next)
	{
		assert(0 == pg->ref_count);
		assert(pager_page_is_in_all_list(pg));
	}

	/* 检查synjf链表 */
	for(pg = hpgr->pg_syncjf_first; pg; pg = pg->pg_syncjf_next)
	{
		assert(pager_page_is_sync_journal(pg));
		assert(0 == pg->ref_count);
		assert(pager_page_is_in_all_list(pg));
		assert(MyHashTableSearch(hpgr->hash_pgno_journal, (void *)pg->page_num));
	}

	/* 检查dirty链表 */
	for(pg = hpgr->pg_dirty; pg ; pg = pg->pg_dirty_next)
	{
		assert(pager_page_is_in_all_list(pg));
		assert(pager_page_is_dirty(pg));

		assert(MyHashTableSearch(hpgr->hash_pgno_journal, (void *)pg->page_num));
	}

	/* 检查all链表 */
	for(pg = hpgr->pg_all; pg; pg = pg->pg_all_next)
	{
		pg_count ++;
		ref_count += pg->ref_count;

		if(0 == pg->ref_count)
		{
			assert(pager_page_is_in_free_list(pg));
			if(pager_page_is_sync_journal(pg))
				assert(pager_page_is_in_syncjf_list(pg));
			else
				assert(!pager_page_is_in_syncjf_list(pg));
		}
		else
		{
			assert(!pager_page_is_in_free_list(pg));
		}

		if(pager_page_is_dirty(pg))
			assert(pager_page_is_in_dirty_list(pg));
		else
			assert(!pager_page_is_in_dirty_list(pg));

		if(pager_page_is_sync_journal(pg))
			assert(MyHashTableSearch(hpgr->hash_pgno_journal, (void *)pg->page_num));

		if(pager_page_is_clean_not_in_jf(pg))
			assert(!MyHashTableSearch(hpgr->hash_pgno_journal, (void *)pg->page_num));

		assert(MyHashTableSearch(hpgr->hash_page, (void *)pg->page_num));
	}

	/* 检查哈希缓存里的元素个数与all链表里的个数相同 */
	assert(MyHashTableGetElementCount(hpgr->hash_page) == pg_count);
	assert(ref_count == hpgr->ref_count);

	/* 检查jf机制 */
	if(hpgr->pg_dirty)
		assert(hpgr->hjf);

	/* 如果同步过,则表示有写回至外存 */
	if(hpgr->bsync_journal_has_done && hpgr->hjf)
		assert(0 == hpgr->page_file_is_integrity);

	/* 检查jf */
	if(check_jf)
		pager_check_jf(hpgr, buf, buf_sz);

	/* 检查外存空闲页"链"表 */
	/* 空闲页总数,每页对应的标识,空闲页链表,轮循所有页,不空闲标识 */
	if(check_free)
		pager_check_free(hpgr);
}




















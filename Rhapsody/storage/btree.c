/**
 * @file btree.c 描述b树算法 2008-03-03 23:26
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.
 *        描述b树算法,基于页缓存机制pager基础上构建
 * 
 * btree page head define
 *   OFFSET   SIZE     DESCRIPTION
 *      0       1      Flags. 1: intkey, 2: hasdata, 4: leaf
 *      1       1      记录碎片的大小
 *      2       2      byte offset to the first freeblock
 *      4       2      number of cells on this page
 *      6       2      first byte of the cell content area
 *      8       4      Right child (the Ptr(N+1) value).  reserve on leaves.
 *
 * 一条记录的分布如下
 *    SIZE    DESCRIPTION
 *      4     Page number of the left child. Omitted if leaf flag is set.
 *      4     Number of bytes of data. Omitted if the zerodata flag is set.
 *      4     Number of bytes of key. Or the key itself if intkey flag is set.
 *      4     First page of the overflow chain.  Omitted if no overflow
 *      *     Payload
 *
 * 溢出页的格式定义
 *    SIZE    DESCRIPTION
 *      4     Page number of next overflow page
 *      *     Data
 *
 * 页内空闲块定义
 *    SIZE    DESCRIPTION
 *      2     Byte offset of the next freeblock
 *      2     Bytes in this freeblock
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

#include "btree.h"

#include <string.h>
#include <stdio.h>

#include "pager.h"
#include "mylist.h"
#include "mylog.h"
#include "mybuffer.h"
#include "MyAlgorithm.h"


/* 一页分成10份 */
#define BTREE_PERCENT 10

/* 溢出时,最大本页存储为1/10 */
#define BTREE_MAX_LOCAL 1

/* 每页的最小填充率4/10 */
#define BTREE_FILL_RATE 4

/* 每个cell"指针"的大小 */
#define BTREE_CELL_PTR_SIZE 2

/* master表的根节点 */
#define BTREE_MASTER_ROOT 2


/**
 * 一条记录的分布如下
 *    SIZE    DESCRIPTION
 *      4     Page number of the left child. Omitted if leaf flag is set.
 *      4     Number of bytes of data. Omitted if the zerodata flag is set.
 *      4     Number of bytes of key. Or the key itself if intkey flag is set.
 *      4     First page of the overflow chain.  Omitted if no overflow
 *      *     Payload
 */
typedef struct __btree_cell_info_t_
{
	/* cell内容的起始 */
	unsigned char * cell;
	/* 整个cell的总大小 */
	unsigned int cell_sz;

	/* payload之前的字节数 */
	unsigned int head_sz;

	/* key的大小,或者就是key本身(如果key是个整形数的话) */
	unsigned int key_sz;

	/* data的大小 */
	unsigned int data_sz;

	/* 本地存了多少payload,如果有溢出的话 */
	unsigned int local_sz;

	/* payload的总大小 */
	unsigned int payload_sz;

	/* 记录左孩子的页号(如果是非叶节点) */
	unsigned int lchild_pgno;
	/* 首个overflow的页号 */
	unsigned int overflow_pgno;
}btree_cellinfo;

/* 此结构体的存储空间附加的页缓存后面,所有权归页缓存管理所有 */
typedef struct __btree_node_
{
	/* 记录是否已经初始化过了 */
	int init_flag;

	/* 父节点 */
	struct __btree_node_ * parent;

	/* 索引数组是否有改变 */
	unsigned int idx_changed;

	/* 对应父节点中的cell的索引 */
	unsigned int idx_in_parent_cellarray;

	/* 页标识 */
	unsigned char node_flag;

	/* cell个数 */
	unsigned int ncell;

	/* 空闲存储空间大小 */
	unsigned int nfree;

	/* 页缓存指针 */
	HPAGE_HEAD pg;
	/* 记录用于读取的页缓存指针 */
	unsigned char * read_buf;
	/* 当前节点对应的页号 */
	unsigned int pg_no;

	/* 此节点归属于哪个btree管理器 */
	struct __btree_mgr_t_ * btree_mgr;

	/*
	* 溢出的cell,这里的溢出不是指记录超出本页最大存储量而产生的,
	* 而是指在添加记录时该页已经没有多余空间了,打包后的新记录暂时存放在这里
	* 之后在balance函数中再放入合适的位置(能过分裂节点而产生富余的存储空间)
	*/
	struct cell_overflow
	{
		HMYBUFFER cell_buf;
		unsigned int idx;
	}cell_ovfl;
	/* 标识,1:表示cell_ovfl里填充的内容是有效的 */
	int has_cell_ovfl;
}btree_node;

typedef struct __btree_cursor_
{
	/* 此游标对应btree的root page num */
	unsigned int root_pgno;

	/* 当前游标所处的页节点 */
	btree_node * cur_node;
	/* 当前游标在节点cell数组的位置 */
	unsigned int idx;

	/* 比较回调函数,用户的上下文信息 */
	XCOMPARE key_cmp;
	void * context;
	unsigned int context_sz;

	/* 此游标的归属于哪个btree管理 */
	struct __btree_mgr_t_ * btree_mgr;

	/* 游标链表 */
	struct __btree_cursor_ * cursor_prev, * cursor_next;

	///* cell信息的临时存储区 */
	//btree_cellinfo cell_info;

	/* 错误码,在处理过程中是否发生错误 */
	int err;
}btree_cursor;

typedef struct __btree_mgr_t_
{
	/* 内存池句柄 */
	HMYMEMPOOL hm;

	/* 页管理器 */
	HPAGER hpgr;
	/* 页的大小 */
	unsigned int pg_sz;

	/* 记录溢出时,在本页存储的大小上限与下限 */
	unsigned int max_local;
	unsigned int min_local;

	/* 保存所有的游标 */
	btree_cursor * cursor_lst;

	/* 保存临界填充率对应的填充值,填充率,扣除了每个节点头空间的存储信息的大小 */
	unsigned int pg_min_fill_sz;
}btree_mgr_t;


/**
 * btree page head define
 *   OFFSET   SIZE     DESCRIPTION
 *      0       1      Flags. 1: intkey, 2: hasdata, 4: leaf
 *      1       1      记录碎片的大小
 *      2       2      byte offset to the first freeblock
 *      4       2      number of cells on this page
 *      6       2      first byte of the cell content area
 *      8       4      Right child (the Ptr(N+1) value).  reserve on leaves.
 */
typedef struct __unused_btree_head_
{
	char nodeflag[1];
	char nfrag[1];
	char first_free[2];
	char ncell[2];
	char first_content[2];
	char most_right_child[4];
}unused_btree_head;

/* 页标识的偏移 */
#define BTREE_HDR_PAGEFLAG_OFFSET (((unused_btree_head *)NULL)->pageflag - (char *)NULL)

/* 空闲碎片的总数 */
#define BTREE_HDR_NFRAG_OFFSET (((unused_btree_head *)NULL)->nfrag - (char *)NULL)

/* 节点标识的偏移 */
#define BTREE_HDR_NODEFLAG_OFFSET (((unused_btree_head *)NULL)->nodeflag - (char *)NULL)

/* 节点里cell值的偏移 */
#define BTREE_HDR_NCELL_OFFSET (((unused_btree_head *)NULL)->ncell - (char *)NULL)
/* 节点里存cell值空间的大小 */
#define BTREE_HDR_NCELL_SZ (sizeof(((unused_btree_head *)NULL)->ncell))

/* 节点里第一个空闲存储块偏移的值 */
#define BTREE_HDR_FIRST_FREE_OFFSET (((unused_btree_head *)NULL)->first_free - (char *)NULL)

/* 节点content的起始 */
#define BTREE_HDR_FIRST_CONTENT (((unused_btree_head *)NULL)->first_content - (char *)NULL)

/* 记录节点最右边子节点的偏移 */
#define BTREE_HDR_MOST_RIGHT_CHILD_OFFSET (((unused_btree_head *)NULL)->most_right_child - (char *)NULL)

/* 节点的头信息的最大值 */
#define BTREE_HDR_SZ (sizeof(unused_btree_head))

/* cell偏移数组的超始偏移 */
#define BTREE_CELL_PTR_OFFSET (sizeof(unused_btree_head))

/* 空闲碎片的上限 */
#define BTREE_MAX_FRAG 128

/* 一个节点的承载量 */
#define BTREE_NODE_PAYLOAD(_btree_mgr) (_btree_mgr->pg_sz - BTREE_HDR_SZ)


/**
 * 溢出页的格式定义
 *    SIZE    DESCRIPTION
 *      4     Page number of next overflow page
 *      *     Data
 */
typedef struct __unused_overflow_head_
{
	char next[4];
	char data[1];
}unused_overflow_head;

/* 溢出页头信息的大小 */
#define BTREE_OVERFLOW_HDR_SZ (((unused_overflow_head *)NULL)->data - (char *)NULL)

/* 溢出页的承载信息量 */
#define BTREE_OVERFLOW_PAYLOAD(__btree_mgr) ((__btree_mgr)->pg_sz - BTREE_OVERFLOW_HDR_SZ)


/*
 * 页内空闲块定义
 *    SIZE    DESCRIPTION
 *      2     Byte offset of the next freeblock
 *      2     Bytes in this freeblock
 */
typedef struct __unused_free_block_
{
	char next[2];
	char sz[2];
}unused_free_block;

/* 下一个空间块的偏移 */
#define BTREE_FREE_BLOCK_NEXT_OFFSET (((unused_free_block*)NULL)->next - (char *)NULL)

/* 本空闲块的大小 */
#define BTREE_FREE_BLOCK_SZ_OFFSET (((unused_free_block*)NULL)->sz - (char *)NULL)

/* 空闲块的最小值 */
#define BTREE_FREE_BLOCK_MIN_SZ (sizeof(unused_free_block))


/*
 * 一条记录的分布如下
 *    SIZE    DESCRIPTION
 *      4     Page number of the left child. Omitted if leaf flag is set.
 *      4     Number of bytes of data. Omitted if the zerodata flag is set.
 *      4     Number of bytes of key. Or the key itself if intkey flag is set.
 *      4     First page of the overflow chain.  Omitted if no overflow
 *      *     Payload
 */
typedef struct __unused_cell_
{
	unsigned char lchild[4];
	unsigned char data_sz[4];
	unsigned char key_sz[4];
	unsigned char overflow[4];
}unused_cell;

/* cell头信息最大长度 */
#define BTREE_CELL_HDR_SZ (sizeof(unused_cell))

/* cell的最小值 */
#define BTREE_CELL_MIN_SZ BTREE_CELL_HDR_SZ

/* lchild的偏移 */
#define BTREE_CELL_LCHILD_OFFSET (((unused_cell *)NULL)->lchild - (unsigned char *)NULL)
/* lchild的大小 */
#define BTREE_CELL_LCHILD_SZ (sizeof(((unused_cell *)NULL)->lchild))


/**
 * @brief 计算一个cell总会占去多少空间
 */
#define btree_mgr_cal_cell_real_fill_sz(__c_sz) (__c_sz + BTREE_CELL_PTR_SIZE)

/**
 * @brief 设置某个节点的属性
 */
static __INLINE__ int btree_mgr_set_nodeflag(btree_node * node, unsigned char nodeflag)
{
	unsigned char * wb = NULL;

	assert(node && node->pg);

	wb = PageHeadMakeWritable(node->pg);
	if(NULL == wb)
		return -1;

	node->node_flag = nodeflag;

	wb[BTREE_HDR_NODEFLAG_OFFSET] = nodeflag;

	return 0;
}

/**
 * @brief 增加某个节点承载页的引用计数
 */
#define btree_mgr_ref_node(_n) do{\
		assert(_n && _n->pg); \
		PageHeadRef(_n->pg);\
	}while(0)

/**
 * @brief 初始化一个节点
 * @param bnew:是否是一个新分配节点
 */
static __INLINE__ int btree_mgr_init_node(btree_node * node, int bnew)
{
	/*
	* btree page head define
	*   OFFSET   SIZE     DESCRIPTION
	*      0       1      Flags. 1: intkey, 2: hasdata, 4: leaf
	*      1       1      记录碎片的大小
	*      2       2      byte offset to the first freeblock
	*      4       2      number of cells on this page
	*      6       2      first byte of the cell content area
	*      8       4      Right child (the Ptr(N+1) value).  reserve on leaves.
	*/

	unsigned int cell_content = 0;
	unsigned int free_buf = 0;

	const unsigned char * data = node->read_buf;

	assert(node->read_buf == PageHeadMakeReadable(node->pg));
	assert(data);

	if(bnew)
	{
		unsigned char * wb = PageHeadMakeWritable(node->pg);
		if(NULL == wb)
			return -1;

		memset(wb, 0, BTREE_HDR_SZ);

		node->ncell = 0;
		node->nfree = node->btree_mgr->pg_sz - BTREE_CELL_PTR_OFFSET;
		node->node_flag = 0;
		node->has_cell_ovfl = 0;
		node->idx_changed = 0;
		node->cell_ovfl.cell_buf = NULL;
		node->cell_ovfl.idx = 0;

		return 0;
	}
	else
	{
		node->node_flag = data[BTREE_HDR_NODEFLAG_OFFSET];

		get_2byte(&(data[BTREE_HDR_NCELL_OFFSET]), BTREE_HDR_NCELL_SZ, node->ncell);
		get_2byte(&(data[BTREE_HDR_FIRST_CONTENT]), BTREE_CELL_PTR_SIZE, cell_content);

		if(0 == cell_content)
		{
			/* 说明这是个节点此时没有任何存储 */
			node->nfree = node->btree_mgr->pg_sz - BTREE_CELL_PTR_OFFSET;
		}
		else
		{
			/* 空闲存储空间大小 */
			node->nfree = cell_content - (2 * node->ncell + BTREE_CELL_PTR_OFFSET) +
				data[BTREE_HDR_NFRAG_OFFSET];

			get_2byte(&(data[BTREE_HDR_FIRST_FREE_OFFSET]), BTREE_CELL_PTR_SIZE, free_buf);

			/* 顺着空闲链表做统计 */
			while(free_buf)
			{
				unsigned int sz = 0;
				unsigned int next_free = 0;

				get_2byte(&(data[free_buf + BTREE_FREE_BLOCK_SZ_OFFSET]), sizeof(unsigned short), sz);
				node->nfree += sz;

				get_2byte(&(data[free_buf + BTREE_FREE_BLOCK_NEXT_OFFSET]), BTREE_CELL_PTR_SIZE, next_free);

				/* 空闲块按偏移的升序排列,并且不可能连续(释放时要处理合并连续空闲块的情况) */
				assert(next_free > free_buf + BTREE_FREE_BLOCK_MIN_SZ || 0 == next_free);

				free_buf = next_free;
			}
		}

		node->idx_changed = 0;

		return 0;
	}
}

/**
 * @brief 取消对某一页的引用,但不销毁这个节点.
 */
static __INLINE__ int btree_mgr_release_node(btree_node * node)
{
	assert(node);

	return  PagerReleasePage(node->pg);
}

/**
 * @brief 获取一个节点
 * @param b_in_cache:是否只从缓存里获取
 * @param b_new:是否是新的节点
 */
static __INLINE__ btree_node * btree_mgr_get_node_aux(btree_mgr_t * btree_mgr,
													  unsigned int pgno,
													  btree_node * parent,
													  int idx_in_parent,
													  int b_only_in_cache,
													  int b_new)
{
	HPAGE_HEAD pg = NULL;
	btree_node * node = NULL;

	assert(btree_mgr && pgno);

	if(b_only_in_cache)
		pg = PagerGetCachedPage(btree_mgr->hpgr, pgno);
	else
		pg = PagerGetPage(btree_mgr->hpgr, pgno);

	if(NULL == pg)
		return NULL;

	node = (btree_node *)PageHeadGetUserData(pg);

	assert((!node->init_flag && !node->parent) || node->init_flag);

	if(parent)
	{
		assert(parent->init_flag);

		if(parent != node->parent)
		{
			if(node->parent)
				btree_mgr_release_node(node->parent);

			btree_mgr_ref_node(parent);

			node->parent = parent;
		}

		node->idx_in_parent_cellarray = idx_in_parent;
	}
	else
	{
		assert(!(node->node_flag & e_btree_page_flag_leaf) || NULL == node->parent);

		if(node->parent)
			btree_mgr_release_node(node->parent);

		node->idx_in_parent_cellarray = 0;
		node->parent = NULL;
	}

	assert(PageHeadGetUserDataSize(pg) == sizeof(*node));

	/* 如果已经初始化过了,不用再做这些操作 */
	if(node->init_flag)
		return node;

	node->pg = pg;
	node->pg_no = pgno;

	node->read_buf = (unsigned char *)PageHeadMakeReadable(pg);
	node->btree_mgr = btree_mgr;

	assert(node->read_buf);
	assert(PagePtrIsInpage(node->pg, node->read_buf));

	btree_mgr_init_node(node, b_new);

	node->init_flag = 1;

	return node;
}

/**
 * @brief 获取一个节点
 */
#define btree_mgr_get_node_new(__btr_mgr, __pgno, __parent, __idx_in_parent) \
	btree_mgr_get_node_aux(__btr_mgr, __pgno, __parent, __idx_in_parent, 0, 1)

/**
 * @brief 获取一个节点
 */
#define btree_mgr_get_node(__btr_mgr, __pgno, __parent, __idx_in_parent) \
	btree_mgr_get_node_aux(__btr_mgr, __pgno, __parent, __idx_in_parent, 0, 0)

/**
 * @brief 获取一个缓存中节点,不在缓存中,则失败返回
 */
#define btree_mgr_get_cached_node(__btr_mgr, __pgno, __parent, __idx_in_parent) \
	btree_mgr_get_node_aux(__btr_mgr, __pgno, __parent, __idx_in_parent, 1, 0)

/**
 * @brief 销毁一个节点,对这个节点的承载页引用计数减少1
 */
static __INLINE__ int btree_mgr_delete_node(btree_node * node)
{
	unsigned int pgno = 0;
	assert(node && node->pg_no);
	assert(node->btree_mgr && node->btree_mgr->hpgr);

	pgno = node->pg_no;
	btree_mgr_release_node(node);

	if(0 != PagerReleasePageNo(node->btree_mgr->hpgr, pgno))
		return -1;

	return 0;
}

/**
 * @brief 向pager申请一页,创建一个新的节点
 */
static __INLINE__ btree_node * btree_mgr_new_node(btree_mgr_t * btree_mgr, btree_node * parent, int idx_in_parent, unsigned char node_flag)
{
	unsigned int pgno = 0;
	btree_node * node = NULL;

	assert(btree_mgr && btree_mgr->hpgr);

	pgno = PagerGetPageNo(btree_mgr->hpgr);
	if(0 == pgno)
		return NULL;

	node = btree_mgr_get_node_new(btree_mgr, pgno, parent, idx_in_parent);

	assert(btree_mgr->pg_sz - BTREE_HDR_SZ == node->nfree);
	assert(0 == node->ncell);

	if(NULL == node)
	{
		PagerReleasePageNo(btree_mgr->hpgr, pgno);
		return NULL;
	}

	if(0 != btree_mgr_set_nodeflag(node, node_flag))
	{
		btree_mgr_delete_node(node);
		return NULL;
	}

	return node;
}

/**
 * @brief 将游标加入链表
 */
static __INLINE__ int btree_mgr_add_to_cursor_list(btree_mgr_t * btree_mgr, btree_cursor * cursor)
{
	assert(btree_mgr && cursor);
	assert(btree_mgr == cursor->btree_mgr);
	assert(NULL == cursor->cursor_next && NULL == cursor->cursor_prev);

	if(NULL == btree_mgr->cursor_lst)
	{
		btree_mgr->cursor_lst = cursor;
		return 0;
	}

	cursor->cursor_next = btree_mgr->cursor_lst;
	btree_mgr->cursor_lst->cursor_prev = cursor;

	btree_mgr->cursor_lst = cursor;

	return 0;
}

/**
 * @brief 将游标从链表中脱离
 */
static __INLINE__ int btree_mgr_out_of_cursor_list(btree_mgr_t * btree_mgr, btree_cursor * cursor)
{
	assert(btree_mgr && cursor);
	assert(btree_mgr == cursor->btree_mgr);
	assert(cursor->cursor_next || cursor->cursor_prev || cursor == btree_mgr->cursor_lst);

	if(cursor == btree_mgr->cursor_lst)
		btree_mgr->cursor_lst = cursor->cursor_next;

	if(cursor->cursor_prev)
		cursor->cursor_prev->cursor_next = cursor->cursor_next;

	if(cursor->cursor_next)
		cursor->cursor_next->cursor_prev = cursor->cursor_prev;

	cursor->cursor_prev = NULL;
	cursor->cursor_next = NULL;

	return 0;
}

/**
 * @brief 创建的一个游标
 */
static __INLINE__ int btree_mgr_release_cursor(btree_cursor * cursor)
{
	assert(cursor && cursor->btree_mgr);

	if(cursor->cur_node)
		btree_mgr_release_node(cursor->cur_node);

	/* 从链表中脱离 */
	assert(cursor->cursor_next || cursor->cursor_prev || cursor == cursor->btree_mgr->cursor_lst);
	btree_mgr_out_of_cursor_list(cursor->btree_mgr, cursor);

	MyMemPoolFree(cursor->btree_mgr->hm, cursor);

	return 0;
}

/**
 * @brief 创建的一个游标
 */
static __INLINE__ btree_cursor * btree_mgr_alloc_cursor(btree_mgr_t * btree_mgr,
														XCOMPARE cmp,
														const void * context, unsigned int context_sz)
{
	btree_cursor * cursor = NULL;
	assert(btree_mgr);

	cursor = MyMemPoolMalloc(btree_mgr->hm, sizeof(*cursor));
	if(NULL == cursor)
		return NULL;

	cursor->btree_mgr = btree_mgr;

	cursor->context = (void *)context;
	cursor->context_sz = context_sz;
	cursor->key_cmp = cmp;
	assert(cursor->key_cmp);

	cursor->cursor_next = NULL;
	cursor->cursor_prev = NULL;

	cursor->root_pgno = 0;
	cursor->idx = 0;
	cursor->cur_node = NULL;

	cursor->err = 0;

	/* 加入游标链表 */
	btree_mgr_add_to_cursor_list(btree_mgr, cursor);

	return cursor;
}

/**
 * @brief 将游标移至根节点
 */
static int btree_mgr_move_to_root(btree_cursor * cursor)
{
	btree_node * root_node = NULL;

	assert(cursor && cursor->btree_mgr);
	assert(cursor->root_pgno && cursor->cur_node);

	if(cursor->root_pgno == cursor->cur_node->pg_no)
	{
		assert(NULL == cursor->cur_node->parent);
		assert(cursor->cur_node->init_flag);

		return 0;
	}

	root_node = btree_mgr_get_node(cursor->btree_mgr, cursor->root_pgno, NULL, 0);
	if(NULL == root_node)
		return -1;

	btree_mgr_release_node(cursor->cur_node);

	cursor->cur_node = root_node;

	cursor->idx = 0;

	return 0;
}

/**
 * @brief 销毁btree管理器
 */
static int btree_mgr_destroy(btree_mgr_t * btree_mgr)
{
	btree_cursor * cursor;

	assert(btree_mgr);

	/* 释放所有游标 */
	cursor = btree_mgr->cursor_lst;
	while(cursor)
	{
		btree_mgr_release_cursor(cursor);
		cursor = btree_mgr->cursor_lst;
	}

	/* 销毁pager */
	if(btree_mgr->hpgr)
		PagerDestruct(btree_mgr->hpgr);

	/* 释放自己 */
	MyMemPoolFree(btree_mgr->hm, btree_mgr);

	return 0;
}

/**
 * @brief 去除ovfl信息
 */
#define btree_mgr_clear_ovfl(__n) do{\
		(__n)->has_cell_ovfl = 0;\
		MyBufferDestruct((__n)->cell_ovfl.cell_buf);\
		(__n)->cell_ovfl.cell_buf = NULL;\
	}while(0)

/**
 * @brief 页析构回调函数 
 */
static int btree_page_release(HPAGE_HEAD pg)
{
	btree_node * node = NULL;

	/* LOG_DEBUG(("btree_page_release")); */

	/* 引用计数为一定为零 */
	assert(PagerGetPageRefCount(pg) == 0);
	assert(sizeof(*node) == PageHeadGetUserDataSize(pg));

	node = PageHeadGetUserData(pg);
	assert(node);

	if(!node->init_flag)
	{
		assert(NULL == node->parent);
		assert(NULL == node->cell_ovfl.cell_buf);
		return 0;
	}

	/* 取消对父节点的引用 */
	if(node->parent)
	{
		btree_node * parent = node->parent;
		node->parent = NULL;
		btree_mgr_release_node(parent);
	}

	/* cell_ovfl里的buffer要释放,因为没有引用了,否则会造成内存泄漏 */
	btree_mgr_clear_ovfl(node);

	/* 初节点的初始化标志置成0 */
	node->init_flag = 0;

	return 0;
}

/**
 * @brief 页重新载入回调函数
 */
static int btree_page_reload(HPAGE_HEAD pg)
{
	btree_node * node = NULL;

	LOG_DEBUG(("btree_page_reload"));

	assert(sizeof(*node) == PageHeadGetUserDataSize(pg));

	node = PageHeadGetUserData(pg);

	assert(node);

	/* 将节点页里的信息重新读一遍 */
	if(node->init_flag)
		btree_mgr_init_node(node, 0);

	return 0;
}


/**
 * @brief 创建btree
 * @param pg_sz:btree 的 page size,可为0
 * @param cache_pg_count:btree页缓存最大值,可为0;
 * @param sector_sz:OS扇区的大小,可为0
 * @param rand_seed:随机数初始化种子
 * @param rand_seed_sz:rand_seed所指缓冲区的大小
 * @param user_rate:页文件使用率,可为0
 */
HBTREE_MGR btreeMgrConstruct(HMYMEMPOOL hm,
							 const char * page_file_name,
							 unsigned int pg_sz,
							 unsigned int cache_pg_count,
							 unsigned int sector_sz,
							 void * rand_seed,
							 unsigned int rand_seed_sz)
{
	btree_mgr_t * btree_mgr = NULL;
	if(NULL == page_file_name)
		return NULL;

	btree_mgr = MyMemPoolMalloc(hm, sizeof(*btree_mgr));
	if(NULL == btree_mgr)
		return NULL;

	memset(btree_mgr, 0, sizeof(*btree_mgr));

	btree_mgr->hm = hm;
	btree_mgr->pg_sz = pg_sz;
	btree_mgr->hpgr = PagerConstruct(hm,
		page_file_name,
		btree_page_release, btree_page_reload,
		sizeof(btree_node), cache_pg_count, &(btree_mgr->pg_sz),
		rand_seed, rand_seed_sz,
		sector_sz);

	if(NULL == btree_mgr->hpgr)
		goto btreeMgrConstruct_end_;

	/*
	* 记录溢出时,在本页存储的大小上限与下限
	* max_local应为page_sz的1/10,
	*/
	assert(btree_mgr->pg_sz / BTREE_PERCENT > (BTREE_CELL_PTR_SIZE + BTREE_CELL_HDR_SZ));

	btree_mgr->max_local = (((btree_mgr->pg_sz - BTREE_HDR_SZ) / BTREE_PERCENT 
		- (BTREE_CELL_PTR_SIZE + BTREE_CELL_HDR_SZ)) / SYS_ALIGNMENT) * SYS_ALIGNMENT;
	btree_mgr->min_local = ((btree_mgr->max_local / 2) / SYS_ALIGNMENT) * SYS_ALIGNMENT;

	btree_mgr->pg_min_fill_sz = ((btree_mgr->pg_sz - BTREE_HDR_SZ) * BTREE_FILL_RATE) / BTREE_PERCENT;
	assert(btree_mgr->pg_min_fill_sz > BTREE_HDR_SZ + btree_mgr->max_local);

	assert(btree_mgr->max_local >= 2 * BTREE_CELL_MIN_SZ);
	assert(btree_mgr->min_local >= 2 * BTREE_CELL_MIN_SZ);

	/* 游标链表头置空 */
	btree_mgr->cursor_lst = NULL;
	return btree_mgr;

btreeMgrConstruct_end_:

	btree_mgr_destroy(btree_mgr);
	return NULL;
}

/**
 * @brief 销毁btree
 */
int btreeMgrDestruct(HBTREE_MGR hbtreeMgr)
{
	if(hbtreeMgr)
		return btree_mgr_destroy(hbtreeMgr);

	return -1;
}

/**
 * @brief 释放游标 与btreeMgrGetCursor是对偶操作
 */
int btreeMgrReleaseCursor(HBTREE_CURSOR hcur)
{
	if(NULL == hcur || NULL == hcur->btree_mgr)
		return -1;

	btree_mgr_release_cursor(hcur);
	return 0;
}

/**
 * @brief 从cell里取出data
 */
static __INLINE__ int btree_mgr_cell_get_data(btree_node * node,
											  btree_cellinfo * cell_info,
											  unsigned char * data, unsigned int data_sz)
{
	unsigned int key_sz_left = 0;
	unsigned int pgno_ovfl = 0;
	HPAGE_HEAD pg_ovfl = NULL;
	const unsigned char * rb = NULL;

	assert(cell_info);

	if(!(node->node_flag & e_btree_page_flag_hasdata))
		return 0;

	assert(data && data_sz >= cell_info->data_sz);
	data_sz = cell_info->data_sz;

	/* 如果没有发生溢出,拷贝返回 */
	if(cell_info->local_sz == cell_info->payload_sz)
	{
		assert(((sizeof(unsigned int)) * ((node->node_flag & e_btree_page_flag_leaf) ? 0 : 1 ) + 
			4 * ((node->node_flag & e_btree_page_flag_hasdata) ? 1 : 0) + 4) == cell_info->head_sz);

		memcpy(data, 
			cell_info->cell + cell_info->head_sz + cell_info->key_sz * ((node->node_flag & e_btree_page_flag_intkey) ? 0 : 1),
			cell_info->data_sz);

		return 0;
	}

	assert(node && node->btree_mgr);

	if(cell_info->key_sz > cell_info->local_sz && !(node->node_flag & e_btree_page_flag_intkey))
	{
		key_sz_left = cell_info->key_sz - cell_info->local_sz;

		pgno_ovfl = cell_info->overflow_pgno;

		/* 跳过key所占有的区域 */
		while(key_sz_left)
		{
			assert(NULL == pg_ovfl);
			pg_ovfl = PagerGetPage(node->btree_mgr->hpgr, pgno_ovfl);
			if(NULL == pg_ovfl)
				goto btree_mgr_cell_get_data_err_;
			
			rb = PageHeadMakeReadable(pg_ovfl);
			if(NULL == rb)
				goto btree_mgr_cell_get_data_err_;

			if(key_sz_left > BTREE_OVERFLOW_PAYLOAD(node->btree_mgr))
			{
				array_to_uint_as_big_endian(rb, BTREE_OVERFLOW_HDR_SZ, pgno_ovfl);
				PagerReleasePage(pg_ovfl);
				pg_ovfl = NULL;

				key_sz_left -= BTREE_OVERFLOW_PAYLOAD(node->btree_mgr);
			}
			else
			{
				if(key_sz_left < BTREE_OVERFLOW_PAYLOAD(node->btree_mgr))
				{
					assert(data_sz >= key_sz_left);

					memcpy(data, rb + BTREE_OVERFLOW_HDR_SZ + key_sz_left, key_sz_left);
					data += key_sz_left;
					data_sz -= key_sz_left;
				}

				PagerReleasePage(pg_ovfl);
				pg_ovfl = PagerGetPage(node->btree_mgr->hpgr, pgno_ovfl);
				if(NULL == pg_ovfl)
					goto btree_mgr_cell_get_data_err_;

				rb = PageHeadMakeReadable(pg_ovfl);
				if(NULL == rb)
					goto btree_mgr_cell_get_data_err_;

				break;
			}
		}
	}
	else if(cell_info->key_sz <= cell_info->local_sz || (node->node_flag & e_btree_page_flag_intkey))
	{
		unsigned int copy_sz = cell_info->local_sz - cell_info->key_sz * ((node->node_flag & e_btree_page_flag_intkey) ? 0 : 1);

		assert(data_sz > copy_sz);

		/* 说明data在本页也有部分存储 */

		if(copy_sz)
		{
			memcpy(data,
				cell_info->cell + cell_info->head_sz + cell_info->key_sz * ((node->node_flag & e_btree_page_flag_intkey) ? 0 : 1),
				copy_sz);

			data += copy_sz;
			data_sz -= copy_sz;
		}

		pg_ovfl = PagerGetPage(node->btree_mgr->hpgr, cell_info->overflow_pgno);
		if(NULL == pg_ovfl)
			goto btree_mgr_cell_get_data_err_;

		rb = PageHeadMakeReadable(pg_ovfl);
		if(NULL == rb)
			goto btree_mgr_cell_get_data_err_;

		rb = PageHeadMakeReadable(pg_ovfl);
		if(NULL == rb)
			goto btree_mgr_cell_get_data_err_;
	}
	else
		assert(0);

	assert(pg_ovfl && PageHeadMakeReadable(pg_ovfl) == rb);

	while(data_sz)
	{
		assert(pg_ovfl);

		if(data_sz > BTREE_OVERFLOW_PAYLOAD(node->btree_mgr))
		{
			memcpy(data, rb + BTREE_OVERFLOW_HDR_SZ, BTREE_OVERFLOW_PAYLOAD(node->btree_mgr));

			data += BTREE_OVERFLOW_PAYLOAD(node->btree_mgr);
			data_sz -= BTREE_OVERFLOW_PAYLOAD(node->btree_mgr);

			PagerReleasePage(pg_ovfl);

			array_to_uint_as_big_endian(rb, BTREE_OVERFLOW_HDR_SZ, pgno_ovfl);
			pg_ovfl = PagerGetPage(node->btree_mgr->hpgr, pgno_ovfl);
			if(NULL == pg_ovfl)
				goto btree_mgr_cell_get_data_err_;
			rb = PageHeadMakeReadable(pg_ovfl);
			if(NULL == rb)
				goto btree_mgr_cell_get_data_err_;
		}
		else
		{
			memcpy(data, rb + BTREE_OVERFLOW_HDR_SZ, data_sz);
			data_sz = 0;
		}
	}

	return 0;

btree_mgr_cell_get_data_err_:

	if(pg_ovfl)
		PagerReleasePage(pg_ovfl);

	return -1;
}

/**
 * @brief 从cell里取出key
 */
static __INLINE__ int btree_mgr_cell_get_key(btree_node * node,
											 btree_cellinfo * cell_info,
											 unsigned char * key, unsigned int key_sz)
{
	assert(cell_info);

	/* 如果是整形key,赋值返回 */
	if(node->node_flag & e_btree_page_flag_intkey)
		return 0;

	/* 如果没有发生溢出,拷贝返回 */
	if(cell_info->local_sz == cell_info->payload_sz)
	{
		assert(key && key_sz >= cell_info->key_sz);
		assert(((sizeof(unsigned int)) * ((node->node_flag & e_btree_page_flag_leaf) ? 0 : 1 ) + 
			4 * ((node->node_flag & e_btree_page_flag_hasdata) ? 1 : 0) + 4) == cell_info->head_sz);

		memcpy(key, 
			cell_info->cell + cell_info->head_sz,
			cell_info->key_sz);

		return 0;
	}

	assert(node && node->btree_mgr);

	{
		btree_mgr_t * btree_mgr = node->btree_mgr;

		assert(key && key_sz >= cell_info->key_sz);

		/* 如果key都在本节点页存储,拷贝返回 */
		if(cell_info->key_sz <= cell_info->local_sz)
		{
			assert((unsigned int)((4 * ((node->node_flag & e_btree_page_flag_leaf) ? 0 : 1) + 
				4 * ((node->node_flag & e_btree_page_flag_hasdata) ? 1 : 0) + 4 + 4)) == cell_info->head_sz);

			memcpy(key,
				cell_info->cell + cell_info->head_sz,
				cell_info->key_sz);

			return 0;
		}
		else
		{
			/* 如果部key在溢出页中也有存储,将key_sz的内容全部拷贝出来 */
			unsigned int left = cell_info->key_sz - cell_info->local_sz;
			unsigned int pgno_ovfl = cell_info->overflow_pgno;
			HPAGE_HEAD pg_ovfl = NULL;

			memcpy(key,
				cell_info->cell + 4 * ((node->node_flag & e_btree_page_flag_leaf) ? 0 : 1) + 
				4 * ((node->node_flag & e_btree_page_flag_hasdata) ? 1 : 0) + 4 + 4,
				cell_info->local_sz);

			key += cell_info->local_sz;

			assert(pgno_ovfl);
			assert(left);

			while(left)
			{
				const unsigned char * rb = NULL;
				assert(NULL == pg_ovfl);

				pg_ovfl = PagerGetPage(btree_mgr->hpgr, pgno_ovfl);
				if(NULL == pg_ovfl)
					return -1;

				rb = PageHeadMakeReadable(pg_ovfl);
				assert(rb);

				if(left > BTREE_OVERFLOW_PAYLOAD(btree_mgr))
				{
					memcpy(key, &rb[BTREE_OVERFLOW_HDR_SZ], BTREE_OVERFLOW_PAYLOAD(btree_mgr));

					key += BTREE_OVERFLOW_PAYLOAD(btree_mgr);
					left -= BTREE_OVERFLOW_PAYLOAD(btree_mgr);

					PagerReleasePage(pg_ovfl);
					pg_ovfl = NULL;

					array_to_uint_as_big_endian(rb, sizeof(unsigned int), pgno_ovfl);
				}
				else
				{
					memcpy(key, &rb[BTREE_OVERFLOW_HDR_SZ], left);
					left = 0;

					PagerReleasePage(pg_ovfl);
					pg_ovfl = NULL;

					break;
				}
			}

			assert(!left);

			return 0;
		}
	}
}

/**
 * @brief 解析一个打包好的cell
 */
static __INLINE__ int btree_mgr_parse_cell(btree_node * node,
										   const unsigned char * cell, const unsigned int cell_buf_sz,
										   btree_cellinfo * cell_info)
{
	/**
	 * 一条记录的分布如下
	 *    SIZE    DESCRIPTION
	 *      4     Page number of the left child. Omitted if leaf flag is set.
	 *      4     Number of bytes of data. Omitted if the zerodata flag is set.
	 *      4     Number of bytes of key. Or the key itself if intkey flag is set.
	 *      4     First page of the overflow chain.  Omitted if no overflow
	 *      *     Payload
	 */

	unsigned int payload_sz = 0;
	unsigned int cell_sz = 0;

	assert(node && cell_info && cell);

	memset(cell_info, 0, sizeof(*cell_info));
	cell_info->cell = (unsigned char *)cell;

	if(!(node->node_flag & e_btree_page_flag_leaf))
	{
		/* 如果不是叶节点 */
		array_to_uint_as_big_endian(cell, sizeof(unsigned int), cell_info->lchild_pgno);
		cell_sz += sizeof(unsigned int);
		cell += sizeof(unsigned int);
	}

	if(node->node_flag & e_btree_page_flag_hasdata)
	{
		/* 如果节点含有数据域 */
		array_to_uint_as_big_endian(cell, sizeof(unsigned int), cell_info->data_sz);
		cell_sz += sizeof(unsigned int);
		cell += sizeof(unsigned int);
		payload_sz += cell_info->data_sz;
	}

	/* 解析key域 */
	array_to_uint_as_big_endian(cell, sizeof(unsigned int), cell_info->key_sz);
	cell_sz += sizeof(unsigned int);
	cell += sizeof(unsigned int);

	/* 如果不是整形的key */
	if(!(node->node_flag & e_btree_page_flag_intkey))
		payload_sz += cell_info->key_sz;

	/* 根据payload_sz的值,判断当前cell是否是溢出的 */
	if(payload_sz > node->btree_mgr->max_local)
	{
		unsigned int surplus = payload_sz % BTREE_OVERFLOW_PAYLOAD(node->btree_mgr);
		if(surplus <= node->btree_mgr->max_local)
			cell_info->local_sz = surplus;
		else
			cell_info->local_sz = node->btree_mgr->min_local;

		/* 查找overflow页号 */
		array_to_uint_as_big_endian(cell, sizeof(unsigned int), cell_info->overflow_pgno);
		cell_sz += sizeof(unsigned int);
		cell += sizeof(unsigned int);
	}
	else
		cell_info->local_sz = payload_sz;

	cell_info->head_sz = cell_sz;
	cell_sz += cell_info->local_sz;

	cell_info->payload_sz = payload_sz;
	cell_info->cell_sz = cell_sz;

	return 0;
}

/**
 * @brief 将cell"指针"指向的内容解析出来
 */
static __INLINE__ int btree_mgr_parse_cellptr(btree_node * node, unsigned int cell_ptr, btree_cellinfo * cell_info)
{
	assert(cell_info);
	assert(node && node->pg && node->read_buf);
	assert(node->read_buf == PageHeadMakeReadable(node->pg));

	{
		const unsigned char * cell = &node->read_buf[cell_ptr];

		assert(PagePtrIsInpage(node->pg, cell));

		btree_mgr_parse_cell(node, cell, node->btree_mgr->pg_sz - (cell - node->read_buf), cell_info);		
	}
	
	return 0;	
}

/**
 * @brief 获取指定idx的cell"指针"
 */
#define btree_mgr_find_cellptr(_n, _i, _cp) do{\
		assert((_n) && (_n)->pg && (_n)->pg_no);\
		assert((_n)->read_buf);\
		assert((_n)->read_buf == PageHeadMakeReadable((_n)->pg));\
		assert((_i) < (_n)->ncell);\
		get_2byte(&(_n)->read_buf[BTREE_CELL_PTR_OFFSET + (_i) * BTREE_CELL_PTR_SIZE], BTREE_CELL_PTR_SIZE, (_cp));\
		assert((_cp));\
	}while(0)

/**
 * @brief 解析相应idx对应cell的信息
 */
#define btree_mgr_parse_idx(_n, _i, _ci) do{\
		unsigned int __cell_ptr = 0;\
		assert((_n) && (_ci));\
		assert((_i) < _n->ncell);\
		btree_mgr_find_cellptr((_n), (_i), __cell_ptr);\
		btree_mgr_parse_cellptr((_n), __cell_ptr, (_ci));\
	}while(0)

/**
 * @brief 取得某个节点指定idx的lchild
 */
#define btree_mgr_get_idx_lchild(_n, _i, _lchild) do{\
		btree_cellinfo __ci;\
		assert((_n));\
		assert((_i) < (_n)->ncell);\
		btree_mgr_parse_idx(_n, _i, &__ci);\
		_lchild = __ci.lchild_pgno;\
	}while(0)

/**
 * @brief 填充cell info信息,计算cell_sz的大小(包含cell头大于,以及local payload,不包含溢出的payload)
 */
static __INLINE__ unsigned int btree_mgr_fill_cellinfo(btree_node * node,
													   unsigned int key_sz, unsigned data_sz,
													   btree_cellinfo * cell_info)
{
	/* key是一定存在的,其它根据页的属性以及payload的值有可能被忽略 */
	unsigned int cell_sz = sizeof(unsigned int);
	unsigned int payload_sz = 0;

	assert(node && node->btree_mgr);
	assert(cell_info);

	memset(cell_info, 0, sizeof(*cell_info));
	cell_info->key_sz = key_sz;

	/* 是否是叶子节点 */
	if(!(node->node_flag & e_btree_page_flag_leaf))
		cell_sz += sizeof(unsigned int);

	/* 关键字是否为整形 */
	if(!(node->node_flag & e_btree_page_flag_intkey))
		payload_sz += key_sz;

	/* 是否包含数据域 */
	if(node->node_flag & e_btree_page_flag_hasdata)
	{
		cell_info->data_sz = data_sz;
 
		cell_sz += sizeof(unsigned int);
		payload_sz += data_sz;
	}

	/* payload是否溢出 */
	if(payload_sz <= node->btree_mgr->max_local)
	{
		cell_info->head_sz = cell_sz;
		cell_sz += payload_sz;
		cell_info->local_sz = payload_sz;
	}
	else
	{
		/* todo:此段逻辑与btree_mgr_parse_cellptr中的对应的部分可以再做一次提炼 */
		unsigned surplus = payload_sz % BTREE_OVERFLOW_PAYLOAD(node->btree_mgr);

		/* 加入溢出页页号存储空间 */
		cell_sz += sizeof(unsigned int);
		cell_info->head_sz = cell_sz;

		if(surplus <= node->btree_mgr->max_local)
			cell_info->local_sz = surplus;
		else
			cell_info->local_sz = node->btree_mgr->min_local;

		cell_sz += cell_info->local_sz;
	}
	
	cell_info->payload_sz = payload_sz;
	return cell_info->cell_sz = cell_sz;
}

/**
 * @brief 打包用户的数据,形成一个cell
 */
static __INLINE__ int btree_mgr_pack_cell(btree_node * node,
										  btree_cellinfo * cell_info,
										  unsigned char * cell, const unsigned int cell_sz,
										  const void * key, const unsigned int key_sz,
										  const void * data, const unsigned int data_sz)
{
	/**
	 * 一条记录的分布如下
	 *    SIZE    DESCRIPTION
	 *      4     Page number of the left child. Omitted if leaf flag is set.
	 *      4     Number of bytes of data. Omitted if the zerodata flag is set.
	 *      4     Number of bytes of key. Or the key itself if intkey flag is set.
	 *      4     First page of the overflow chain.  Omitted if no overflow
	 *      *     Payload
	 */

	int ret = 0;

	btree_mgr_t * btree_mgr = NULL;

	/* 记录溢出节点的临时指针 */
	HPAGE_HEAD pg_ovfl = NULL;

	/* 记录payload的大小 */
	unsigned int payload_sz = 0;

	/* 临时指针,指向接收payload的存储空间 */
	unsigned char * pPayload = NULL;

	/* 临时存储当前用于存储payload的空间剩余大小 */
	unsigned int space_left = 0;

	/* 临时指针,指向为溢出页号预留的存储空间 */
	unsigned char * pPrior = NULL;

	/* 临时指针,初始值指向key,如果key为整形,则指向data(如果有data的话) */
	unsigned char * pSrc = NULL;
	unsigned int Src_sz = 0;

	assert(node && node->btree_mgr);
	assert(cell && cell_sz && cell_info);
	assert(cell_sz == cell_info->cell_sz);
	assert(data_sz == cell_info->data_sz);
	assert(key_sz == cell_info->key_sz);

	cell_info->cell = cell;

	/* 如果不是叶节点,要打包左孩子的页索引 */
	if(!(node->node_flag & e_btree_page_flag_leaf))
	{
		assert(cell_info->lchild_pgno);

		uint_to_big_endian(cell_info->lchild_pgno, cell, sizeof(unsigned int));
		cell += sizeof(sizeof(unsigned int));

		assert((unsigned int)(cell - cell_info->cell) <= cell_sz);
	}

	/* 如果含有数据域,要打包数据域的长度 */
	if(node->node_flag & e_btree_page_flag_hasdata)
	{
		assert(data && data_sz);

		uint_to_big_endian(data_sz, cell, sizeof(unsigned int));
		cell += sizeof(sizeof(unsigned int));

		assert((unsigned int)(cell - cell_info->cell) < cell_sz);
	}

	/* 打包key_sz */
	uint_to_big_endian(key_sz, cell, sizeof(unsigned int));
	cell += sizeof(sizeof(unsigned int));
	assert((unsigned int)(cell - cell_info->cell) <= cell_sz);

	if(!(node->node_flag & e_btree_page_flag_intkey))
	{
		pSrc = (unsigned char *)key;
		Src_sz = key_sz;
	}
	else if(node->node_flag & e_btree_page_flag_hasdata)
	{
		pSrc = (unsigned char *)data;
		Src_sz = data_sz;
	}
	else
	{
		assert(0 == cell_info->payload_sz);
		assert((unsigned int)(cell - cell_info->cell) == cell_sz);

		/* 打包结束 */
		return 0;
	}

	payload_sz = cell_info->payload_sz;
	if(payload_sz > node->btree_mgr->max_local)
	{
		assert(cell_info->local_sz < payload_sz);

		/* 如果有溢出,为首个溢出页的页号预留存储空间 */
		pPrior = cell;
		pPayload = cell + sizeof(unsigned int);
	}
	else
		pPayload = cell;

	/* 开始接受payload空间剩余为local_sz */
	space_left = cell_info->local_sz;
	btree_mgr = node->btree_mgr;
	assert(btree_mgr);

	/* 拷贝key(如果需要)与data(如果需要)到pPayload指向的存储空间 */
	while(payload_sz)
	{
		unsigned int n = 0;
		if(0 == space_left)
		{
			/* 如果空间不够,分配一页over flow */
			unsigned int pgno = PagerGetPageNo(btree_mgr->hpgr);
			if(0 == pgno)
			{
				ret = -1;
				goto btree_mgr_pack_cell_end_;/* 取消对某些节点的引用 */
			}

			if(pg_ovfl)
				PagerReleasePage(pg_ovfl);

			pg_ovfl = PagerGetPage(btree_mgr->hpgr, pgno);
			if(NULL == pg_ovfl)
			{
				ret = -1;
				PagerReleasePageNo(btree_mgr->hpgr, pgno);
				goto btree_mgr_pack_cell_end_;
			}

			pPayload = (unsigned char *)PageHeadMakeWritable(pg_ovfl);
			if(NULL == pPayload)
			{
				ret = -1;
				PagerReleasePageNo(btree_mgr->hpgr, pgno);
				goto btree_mgr_pack_cell_end_;
			}

			assert(pPrior);
			/* 将溢出页号写入预留的内存空间 */
			uint_to_big_endian(pgno, pPrior, sizeof(unsigned int));
			pPrior = pPayload;

			pPayload += BTREE_OVERFLOW_HDR_SZ;

			space_left = btree_mgr->pg_sz - BTREE_OVERFLOW_HDR_SZ;
		}

		if(Src_sz > space_left)
			n = space_left;
		else
			n = Src_sz;

		memcpy(pPayload, pSrc, n);

		space_left -= n;
		pPayload += n;

		Src_sz -= n;
		pSrc += n;

		payload_sz -= n;

		if(0 == Src_sz && payload_sz)
		{
			Src_sz = data_sz;
			pSrc = (unsigned char *)data;
		}
	}

btree_mgr_pack_cell_end_:

	if(pg_ovfl)
		PagerReleasePage(pg_ovfl);

	if(pPrior && -1 != ret)
	{
		assert(pPrior != cell);
		uint_to_big_endian(0, pPrior, sizeof(unsigned int));
	}

	return ret;	
}

/**
 * @brief 游标当前所指的位置的cell信息
 */
static __INLINE__ int btree_mgr_get_cell(btree_cursor * cursor, btree_cellinfo * cellinfo)
{
	assert(cursor && cursor->cur_node && cursor->root_pgno && cellinfo);
	assert(cursor->cur_node->pg && cursor->cur_node->pg_no);

	/* 取出cell"指针" */
	/* 根据cell"指针"取出cell的内容 */
	btree_mgr_parse_idx(cursor->cur_node, cursor->idx, cellinfo);

	return 0;
}

/**
 * @brief 查找cell的回调函数
 *  > 0  表示 key 大于 data
 *  == 0 表示 key 等于 data
 *  < 0  表示 key 小于 data
 *
 * @param key:关键字
 * @param key_sz:关键字的缓冲区大小
 * @param context:用户自定义的上下文数据
 * @param context_sz:用户自定义的上下文数据的长度
 */
static int binarysearch_compare(const void * key, unsigned int key_sz,
								const void * data,
								const void * context, unsigned int context_sz)
{
	unsigned char * cell_key = NULL;

	btree_cursor * cursor = (btree_cursor *)context;
	unsigned int cell_ptr = 0;
	btree_cellinfo cell_info;

	assert(cursor && context_sz == sizeof(*cursor));
	assert(PagePtrIsInpage(cursor->cur_node->pg, data));

	get_2byte((unsigned char *)data, BTREE_CELL_PTR_SIZE, cell_ptr);

	btree_mgr_parse_cellptr(cursor->cur_node, cell_ptr, &cell_info);

	if(cursor->cur_node->node_flag & e_btree_page_flag_intkey)
	{
		btree_mgr_cell_get_key(cursor->cur_node, &cell_info, NULL, 0);

		return cursor->key_cmp(key, key_sz,
			NULL, cell_info.key_sz,
			cursor->context, cursor->context_sz);
	}
	else
	{
		int ret = 0;
		cell_key = MyMemPoolMalloc(cursor->btree_mgr->hm, cell_info.key_sz);
		if(0 != btree_mgr_cell_get_key(cursor->cur_node, &cell_info, cell_key, cell_info.key_sz))
		{
			cursor->err = -1;
			ret = 0;
		}
		else
		{
			ret = cursor->key_cmp(key, key_sz, 
				cell_key, cell_info.key_sz,
				cursor->context, cursor->context_sz);
		}

		MyMemPoolFree(cursor->btree_mgr->hm, cell_key);

		return ret;
	}
}

/**
 * @brief 获取right most
 */
#define btree_mgr_get_rmost(_n, _rm) do{\
		assert(_n);\
		assert(!(_n->node_flag & e_btree_page_flag_leaf));\
		assert(_n->read_buf == PageHeadMakeReadable(_n->pg));\
		array_to_uint_as_big_endian(&_n->read_buf[BTREE_HDR_MOST_RIGHT_CHILD_OFFSET], sizeof(unsigned int), _rm);\
		assert(_rm);\
	}while(0)

/**
 * @brief 查找关键值
 * @param pres:0表示找到了关键字
 *         大于零:表示没找到关键字
 *         小于零:表示没找到关键字
 *         如果 序列为 1 3 4 查找 2 
 */
static __INLINE__ int btree_mgr_search(btree_cursor * cursor, const void * key, const unsigned int key_sz, int * pres)
{
	int ret = 0;
	unsigned int pgno_child = 0;
	btree_mgr_t * btree_mgr = NULL;
	btree_node * sub_node = NULL;
	btree_cellinfo cell_info;

	assert(cursor && cursor->btree_mgr && cursor->cur_node);
	assert(cursor->cur_node->btree_mgr == cursor->btree_mgr);
	assert(pres);

	btree_mgr = cursor->btree_mgr;

	/* 将游标移至根节点 */
	if(0 != btree_mgr_move_to_root(cursor))
		return -1;

	assert(cursor->cur_node->init_flag);
	assert(cursor->cur_node->pg_no == cursor->root_pgno);

	while(1)
	{
		/* 在当前节点里做二分查找 */
		const unsigned char * rbuf = cursor->cur_node->read_buf;

		assert(PagePtrIsInpage(cursor->cur_node->pg, &rbuf[BTREE_CELL_PTR_OFFSET]));
		assert(PagePtrIsInpage(cursor->cur_node->pg, 
			rbuf + BTREE_CELL_PTR_OFFSET + cursor->cur_node->ncell * BTREE_CELL_PTR_SIZE - 1));

		if(0 == cursor->cur_node->ncell)
		{
			/* 如果没有cell,直接返回即可 */
			assert(NULL == cursor->cur_node->parent);
			assert(cursor->cur_node->node_flag & e_btree_page_flag_leaf);

			*pres = 1;
			return 0;
		}

		/* 如果找到,返回 */
		ret = MyBinarySearch(&rbuf[BTREE_CELL_PTR_OFFSET], cursor->cur_node->ncell, BTREE_CELL_PTR_SIZE,
			key, key_sz, binarysearch_compare,
			&cursor->idx, cursor, sizeof(*cursor));

		if(0 != cursor->err)
		{
			/* 如果出错 */
			cursor->err = 0;
			return -1;
		}

		if(0 == ret)
		{
			/* 如果找到 */
			*pres = 0;
			return 0;
		}

		if(cursor->cur_node->node_flag & e_btree_page_flag_leaf)
		{
			/* 如果当前已经是叶节点,则返回 */
			*pres = ret;
			return 0;
		}

		/* 根据"指针"去取cell信息 */
		if(cursor->idx < cursor->cur_node->ncell)
		{
			btree_mgr_get_cell(cursor, &cell_info);
			pgno_child = cell_info.lchild_pgno;
		}
		else
			btree_mgr_get_rmost(cursor->cur_node, pgno_child);

		assert(pgno_child);

		/* 找不到,进入相应的子分支继续查找 */
		sub_node = btree_mgr_get_node(cursor->btree_mgr,
			pgno_child,
			cursor->cur_node,
			cursor->idx);

		assert(sub_node->parent == cursor->cur_node);

		if(NULL == sub_node)
			return -1;

		btree_mgr_release_node(cursor->cur_node);
		cursor->cur_node = sub_node;

		assert(PagerGetPageRefCount(cursor->cur_node->pg) && PagerGetPageRefCount(cursor->cur_node->parent->pg));
	}

	*pres = ret;
	return 0;
}

/**
 * @brief 整理某一个节点的空闲碎片
 */
static __INLINE__ int btree_mgr_defrag_node(btree_node * node)
{
	/* 记录备份的content存储区域的指针 */
	unsigned char * content_new = NULL;
	unsigned int new_cell_ptr = 0;
	/* 记录整理碎片前的content起始 */
	unsigned int content_begin = 0;
	/* 在回拷过程中的临时变量,记录cell的"指针" */
	unsigned int cell_ptr = 0;
	/* 临时存储某个cell的信息 */
	btree_cellinfo cell_info;

	unsigned char * write_buf = NULL;
	unsigned int i = 0;

	assert(node);

	/*
	* 将节点内的cell content备份起来
	* 轮循cell ptr数组,将里头的内容顺次解析拷贝.
	* 重置相应的值first_free,cell_content,nfrag
	*/

	write_buf = PageHeadMakeWritable(node->pg);
	if(NULL == write_buf)
		return -1;

	get_2byte(&(write_buf[BTREE_HDR_FIRST_CONTENT]), BTREE_CELL_PTR_SIZE, content_begin);

	content_new = MyMemPoolMalloc(node->btree_mgr->hm, node->btree_mgr->pg_sz);
	if(NULL == content_new)
		return -1;

	/* 开始循环 */
	content_new += node->btree_mgr->pg_sz;
	new_cell_ptr = node->btree_mgr->pg_sz;
	for(i = 0; i < node->ncell; i ++)
	{
		btree_mgr_find_cellptr(node, i, cell_ptr);
		assert(cell_ptr);

		/* 解析相应的cell信息 */
		btree_mgr_parse_cellptr(node,cell_ptr, &cell_info);

		assert(new_cell_ptr > (BTREE_CELL_PTR_OFFSET + node->ncell * BTREE_CELL_PTR_SIZE + node->nfree));

		/* 拷贝回节点页缓存 */
		content_new -= cell_info.cell_sz;
		new_cell_ptr -= cell_info.cell_sz;

		assert(new_cell_ptr);
		assert(cell_ptr >= content_begin);

		memcpy(content_new, &write_buf[cell_ptr], cell_info.cell_sz);

		/* 更新相应的cell ptr */
		put_2byte(new_cell_ptr, &write_buf[BTREE_CELL_PTR_OFFSET + i * BTREE_CELL_PTR_SIZE], BTREE_CELL_PTR_SIZE);
	}

	content_begin = new_cell_ptr;

	/* 所有的free空间应都集中在gap里 */
	assert((content_begin - node->nfree) == BTREE_CELL_PTR_OFFSET + node->ncell * BTREE_CELL_PTR_SIZE);
	assert(content_begin <= node->btree_mgr->pg_sz);

	memcpy(&write_buf[content_begin], content_new, node->btree_mgr->pg_sz - content_begin);

	/*
	* 重置相应的值first_free,cell_content,nfrag
	* btree page head define
	*   OFFSET   SIZE     DESCRIPTION
	*      0       1      Flags. 1: intkey, 2: hasdata, 4: leaf
	*      1       1      记录碎片的大小
	*      2       2      byte offset to the first freeblock
	*      4       2      number of cells on this page
	*      6       2      first byte of the cell content area
	*      8       4      Right child (the Ptr(N+1) value).  reserve on leaves.
	*/
	write_buf[BTREE_HDR_NFRAG_OFFSET] = 0;
	put_2byte(0, &write_buf[BTREE_HDR_FIRST_FREE_OFFSET], BTREE_CELL_PTR_SIZE);
	put_2byte(content_begin,
		&write_buf[BTREE_HDR_FIRST_CONTENT],
		BTREE_CELL_PTR_SIZE);

	assert(content_new);
	MyMemPoolFree(node->btree_mgr->hm, content_new - content_begin);

	return 0;
}

/**
 * @brief 当指定偏移的页内存储空间归还,与btree_mgr_alloc_space是对偶操作
 */
static __INLINE__ int btree_mgr_free_space(btree_node * node, unsigned int ptr, const unsigned int sz)
{
	/*
	* 如果ptr恰好紧跟在gap之后,则不需要加入free"链表",
	* 顺着链表前进,直到链表时的free_ptr大于ptr为止,将ptr加入链表,此时要注意判断是否可以合差相邻的合闲块
	* 更新nfree值,
	* 根据需要更新first content与first free
	*/
	unsigned int content = 0;
	unsigned int cur_free = 0;
	unsigned int pre_free_store = 0;
	unsigned int next_sz = 0;
	unsigned char * wb = PageHeadMakeWritable(node->pg);

	assert(node && ptr && sz);

	if(NULL == wb)
		return -1;

	get_2byte(&wb[BTREE_HDR_FIRST_CONTENT], BTREE_CELL_PTR_SIZE, content);

	/* 如果ptr紧挨着content,则将它并入content */
	if(ptr == content)
	{
		content = ptr + sz;

		/* 看看是否还可以合并更多的合闲块 */
		get_2byte(&wb[BTREE_HDR_FIRST_FREE_OFFSET], BTREE_CELL_PTR_SIZE, cur_free);
		while(cur_free)
		{
			unsigned int temp;
			unsigned int temp_sz;
			if(content != cur_free)
				break;

			get_2byte(&wb[cur_free + BTREE_FREE_BLOCK_SZ_OFFSET], sizeof(unsigned short), temp_sz);
			content = cur_free + temp_sz;

			get_2byte(&wb[cur_free + BTREE_FREE_BLOCK_NEXT_OFFSET], sizeof(unsigned short), temp);
			cur_free = temp;
		}

		put_2byte(content, &wb[BTREE_HDR_FIRST_CONTENT], BTREE_CELL_PTR_SIZE);
		put_2byte(cur_free, &wb[BTREE_HDR_FIRST_FREE_OFFSET], BTREE_CELL_PTR_SIZE);

		goto btree_mgr_free_space_end_;
	}

	get_2byte(&wb[BTREE_HDR_FIRST_FREE_OFFSET], BTREE_CELL_PTR_SIZE, cur_free);

	/* 寻找ptr应加入的位置 */
	pre_free_store = 0;
	while(cur_free)
	{
		unsigned int temp;
		if(cur_free > ptr)
			break;

		pre_free_store = cur_free + BTREE_FREE_BLOCK_NEXT_OFFSET;
		get_2byte(&wb[cur_free + BTREE_FREE_BLOCK_NEXT_OFFSET], BTREE_CELL_PTR_SIZE, temp);

		cur_free = temp;
	}

	/* 将ptr加入"链表" */
	if(pre_free_store)
	{
		unsigned int pre_sz = 0;
		unsigned int total_sz = 0;
		get_2byte(&wb[pre_free_store + BTREE_FREE_BLOCK_SZ_OFFSET], sizeof(unsigned short), pre_sz);

		if(pre_free_store + pre_sz == ptr)
		{
			/* 可以与前一块合并 */
			total_sz = pre_sz + sz;

			/* 是否可以三块都合并 */
			if(ptr + sz == cur_free && cur_free)
			{
				/* 记录合并后的下一个空闲块的索引 */
				unsigned int cur_free_next = 0;

				get_2byte(&wb[cur_free + BTREE_FREE_BLOCK_SZ_OFFSET], sizeof(unsigned short), next_sz);
				total_sz += next_sz;

				get_2byte(&wb[cur_free + BTREE_FREE_BLOCK_NEXT_OFFSET], sizeof(unsigned short), cur_free_next);
				put_2byte(cur_free_next, &wb[pre_free_store + BTREE_FREE_BLOCK_NEXT_OFFSET], sizeof(unsigned short));
			}

			put_2byte(total_sz, &wb[pre_free_store + BTREE_FREE_BLOCK_SZ_OFFSET], sizeof(unsigned short));

			goto btree_mgr_free_space_end_;
		}
		else
		{
			/* 前一块空闲内存next指向ptr */
			put_2byte(ptr, &wb[pre_free_store + BTREE_FREE_BLOCK_NEXT_OFFSET], BTREE_CELL_PTR_SIZE);
		}
	}
	else
	{
		put_2byte(ptr, &wb[BTREE_HDR_FIRST_FREE_OFFSET], BTREE_CELL_PTR_SIZE);
	}

	if(ptr + sz == cur_free)
	{
		/* 如果可以与下一个空闲块合并 */
		unsigned int cur_next = 0;
		get_2byte(&wb[cur_free + BTREE_FREE_BLOCK_SZ_OFFSET], sizeof(unsigned short), next_sz);
		get_2byte(&wb[cur_free + BTREE_FREE_BLOCK_NEXT_OFFSET], BTREE_CELL_PTR_SIZE, cur_next);
		
		/* 填入下块空闲块,本空闲块的大小 */
		put_2byte(cur_next, &wb[ptr + BTREE_FREE_BLOCK_NEXT_OFFSET], BTREE_CELL_PTR_SIZE);
		put_2byte(sz + next_sz, &wb[ptr + BTREE_FREE_BLOCK_SZ_OFFSET], sizeof(unsigned short));
	}
	else
	{
		/* 不能合并,则ptr的next域指向cur_free, 填入本空闲块的大小 */
		put_2byte(cur_free, &wb[ptr + BTREE_FREE_BLOCK_NEXT_OFFSET], BTREE_CELL_PTR_SIZE);
		put_2byte(sz, &wb[ptr + BTREE_FREE_BLOCK_SZ_OFFSET], sizeof(unsigned short));
	}

btree_mgr_free_space_end_:

#ifdef _DEBUG
memset(&wb[ptr] + BTREE_FREE_BLOCK_MIN_SZ, 0, sz - BTREE_FREE_BLOCK_MIN_SZ);
#endif

	node->nfree += btree_mgr_cal_cell_real_fill_sz(sz);

	assert(node->nfree <= node->btree_mgr->pg_sz - BTREE_CELL_PTR_OFFSET);

	return 0;
}

/**
 * @brief 将一个node所存储的cell置成空
 */
static __INLINE__ int btree_mgr_clear_node(btree_node * node)
{
	/* 将frag置成0,将ncell置成0,将content置到底部,将first free置成零 */
	/* right most置成0 */

	unsigned char * wb = NULL;

	assert(node && node->btree_mgr);

	wb = PageHeadMakeWritable(node->pg);
	if(NULL == wb)
		return -1;

	node->ncell = 0;
	node->nfree = node->btree_mgr->pg_sz - BTREE_CELL_PTR_OFFSET;
	node->idx_changed = 1;
	
	memset(wb, 0, node->btree_mgr->pg_sz);

	wb[BTREE_HDR_NODEFLAG_OFFSET] = node->node_flag;

	btree_mgr_clear_ovfl(node);

	return 0;
}

/**
 * @brief 在页缓存内分配指定大小的存储空间,返回的时空间距离用户页缓存可用地址起始的偏移
 */
static __INLINE__ unsigned int btree_mgr_alloc_space(btree_node * node, unsigned int sz)
{
	/*
	* btree page head define
	*   OFFSET   SIZE     DESCRIPTION
	*      0       1      Flags. 1: intkey, 2: hasdata, 4: leaf
	*      1       1      记录碎片的大小
	*      2       2      byte offset to the first freeblock
	*      4       2      number of cells on this page
	*      6       2      first byte of the cell content area
	*      8       4      Right child (the Ptr(N+1) value).  reserve on leaves.
	*/

	/*
	* 页内空闲块定义
	*    SIZE    DESCRIPTION
	*      2     Byte offset of the next freeblock
	*      2     Bytes in this freeblock
	*/

	unsigned int free_idx = 0;
	unsigned int pc_prev = 0;
	unsigned int block_sz = 0;
	unsigned int ret_idx = 0;

	/* cell"指针"的结束 */
	unsigned int cell_ptr_end = 0;
	/* cell content的起始 */
	unsigned int cell_content = 0;
	unsigned char * write_buf = NULL;

	assert(node && sz);
	assert(node->nfree >= sz + BTREE_CELL_PTR_SIZE);
	assert(0 == node->has_cell_ovfl);
	assert(node->read_buf == PageHeadMakeReadable(node->pg));

	write_buf = PageHeadMakeWritable(node->pg);
	if(NULL == write_buf)
		return 0;

	/*
	* 如果存在空间块,则从空间链表里搜索
	* 如果不存在空闲块,则从cell指针数组的未尾与 cell content之间的那段区域获取(以下简称gap)
	* 如果仍然获取不到,则应合并空闲碎片,
	* 从gap里分配需要的存储空间
	*/

	/* 至gap里分配,如果仍然分配不到,此时就需要进行空闲碎片整理 */
	cell_ptr_end = BTREE_CELL_PTR_OFFSET + BTREE_CELL_PTR_SIZE * node->ncell;
	get_2byte(&(write_buf[BTREE_HDR_FIRST_CONTENT]), BTREE_CELL_PTR_SIZE, cell_content);

	/* cell_content为0,说明是一个新的节点 */
	if(0 == cell_content)
		cell_content = node->btree_mgr->pg_sz;

	assert(cell_content >= cell_ptr_end);

	/* 为cell ptr数组的增长预留字节 */
	if((cell_content - cell_ptr_end) < BTREE_CELL_PTR_SIZE)
	{
		if(0 != btree_mgr_defrag_node(node))
			return 0;

		get_2byte(&(write_buf[BTREE_HDR_FIRST_CONTENT]), BTREE_CELL_PTR_SIZE, cell_content);
	}
	else
	{
		get_2byte(&write_buf[BTREE_HDR_FIRST_FREE_OFFSET], sizeof(unsigned short), free_idx);
		pc_prev = (unsigned int)BTREE_HDR_FIRST_FREE_OFFSET;

		assert(((cell_content < node->btree_mgr->pg_sz) && free_idx) || free_idx == 0);

		/* 如果空闲碎片小于BTREE_MAX_FRAG */
		if(write_buf[BTREE_HDR_NFRAG_OFFSET] < BTREE_MAX_FRAG)
		{
			while(free_idx)
			{
				get_2byte(&(write_buf[free_idx + BTREE_FREE_BLOCK_SZ_OFFSET]),
					sizeof(unsigned short),
					block_sz);

				assert(block_sz >= BTREE_FREE_BLOCK_MIN_SZ);

				if(block_sz >= sz)
				{
					if(block_sz < sz + BTREE_FREE_BLOCK_MIN_SZ)
					{
						/*
						* 如果剩余的内存不足4字节,无法形成一个空闲块"链表"节点
						* 则整个空闲块都分配出去,但空闲空间却只扣减相应的sz值,并相应增加nfrag的值
						* 当空闲碎片达到上限,此时进行碎片整理,释放出所有的空闲碎片,供分配使用
						*/
						memcpy(&(write_buf[pc_prev]), 
							&(write_buf[free_idx + BTREE_FREE_BLOCK_NEXT_OFFSET]),
							sizeof(unsigned short));

						assert((block_sz - sz) <= BTREE_FREE_BLOCK_MIN_SZ);

						assert(write_buf[BTREE_HDR_NFRAG_OFFSET] <= BTREE_MAX_FRAG);

						write_buf[BTREE_HDR_NFRAG_OFFSET] += (block_sz - sz);
					}
					else
					{
						/* 扣减相应的sz,返回 */
						put_2byte(block_sz - sz,
							&(write_buf[free_idx + BTREE_FREE_BLOCK_SZ_OFFSET]),
							sizeof(unsigned short));
					}

					/* 从空闲块的底部拿出一块来 */
					ret_idx = free_idx + block_sz - sz;
					goto btree_mgr_alloc_space_end_;
				}

				/* 循环继续 */
				pc_prev = free_idx + BTREE_FREE_BLOCK_NEXT_OFFSET;
				get_2byte(&(write_buf[free_idx + BTREE_FREE_BLOCK_NEXT_OFFSET]),
					sizeof(unsigned short),
					free_idx);
			}
		}
		else
		{
			/* 碎片超过上限了,进行碎片整理 */
			if(0 != btree_mgr_defrag_node(node))
				return 0;

			get_2byte(&(write_buf[BTREE_HDR_FIRST_CONTENT]), BTREE_CELL_PTR_SIZE, cell_content);
		}

		/* 如果仍然分配不到,需要进行空闲碎片整理 */
		if((cell_content - cell_ptr_end) < btree_mgr_cal_cell_real_fill_sz(sz))
		{
			if(0 != btree_mgr_defrag_node(node))
				return 0;

			get_2byte(&(write_buf[BTREE_HDR_FIRST_CONTENT]), BTREE_CELL_PTR_SIZE, cell_content);
		}
	}

	assert((cell_content - cell_ptr_end) >= btree_mgr_cal_cell_real_fill_sz(sz));

	ret_idx = cell_content - sz;
	put_2byte(ret_idx, &(write_buf[BTREE_HDR_FIRST_CONTENT]), BTREE_CELL_PTR_SIZE);

btree_mgr_alloc_space_end_:

	node->nfree -= btree_mgr_cal_cell_real_fill_sz(sz);
	return ret_idx;
}

/**
 * @brief 将一个cell吃进来
 */
static __INLINE__ int btree_mgr_eat_cell(btree_node * node,
										 unsigned int idx,
										 unsigned char * cell, unsigned int cell_sz)
{
	unsigned int ptr = 0;
	unsigned char * wb = NULL;
	unsigned int i = 0;
	unsigned int end = 0;
	unsigned int ins = 0;

	assert(node && cell && cell_sz);
	assert(idx <= node->ncell);
	assert(node->nfree >= (cell_sz + BTREE_CELL_PTR_SIZE));

	/* 空间足够,分配空间,存储 */
	ptr = btree_mgr_alloc_space(node, cell_sz);
	if(0 == ptr)
		return -1;

	wb = PageHeadMakeWritable(node->pg);
	if(NULL == wb)
		return -1;

	/* 存储cell内容 */
	memcpy(&wb[ptr], cell, cell_sz);

	/* 更改cell ptr数组 */
	end = BTREE_CELL_PTR_OFFSET + node->ncell * BTREE_CELL_PTR_SIZE;
	ins = BTREE_CELL_PTR_OFFSET + idx * BTREE_CELL_PTR_SIZE;
	for(i = end; i > ins; i -= BTREE_CELL_PTR_SIZE)
	{
		wb[i] = wb[i - 2];
		wb[i + 1] = wb[i - 1];
	}

	put_2byte(ptr, &wb[ins], BTREE_HDR_NCELL_SZ);

	/* 更改标识位idx_change */
	node->idx_changed = 1;

	return 0;
}

/**
 * @brief 将一个打包好的cell添加到指定的位置
 */
static __INLINE__ int btree_mgr_insert_cell(btree_node * node,
											unsigned int idx,
											unsigned char * cell, unsigned int cell_sz)
{
	/*
	* 如果该节点使用的页上的存储空间足够,
	* 则将新的cell拷进去
	* 如果不够,就挂在附加空间上,在之后的平衡过程里再拷贝进去
	*/

	unsigned char * wb = NULL;

	assert(node && cell && cell_sz);
	assert(0 == node->has_cell_ovfl);
	assert(idx <= node->ncell);

	wb = PageHeadMakeWritable(node->pg);
	if(NULL == wb)
		return -1;

	/* 空间不够,挂在cell_ovfl信息里,待balance函数之后再转存至正常的页缓存里 */
	if(node->nfree < (cell_sz + BTREE_CELL_PTR_SIZE))
	{
		if(NULL == node->cell_ovfl.cell_buf)
			node->cell_ovfl.cell_buf = MyBufferConstruct(node->btree_mgr->hm, cell_sz);

		MyBufferSet(node->cell_ovfl.cell_buf, cell, cell_sz);
		node->cell_ovfl.idx = idx;
		node->has_cell_ovfl = 1;

		/* 更改标识位idx_change */
		node->idx_changed = 1;

		assert(node->cell_ovfl.idx <= node->ncell);

		goto btree_mgr_insert_cell_end_;
	}

	if(0 != btree_mgr_eat_cell(node, idx, cell, cell_sz))
		return -1;

btree_mgr_insert_cell_end_:

	/* 更新ncell的值+1 */
	node->ncell += 1;
	put_2byte(node->ncell, &wb[BTREE_HDR_NCELL_OFFSET], BTREE_HDR_NCELL_SZ);

	return 0;
}

/**
 * @brief 将溢出的cell去除,ncell要减1
 */
static __INLINE__ int btree_mgr_throwup_cell_ovfl(btree_node * node)
{
	unsigned char * wb = NULL;

	assert(node->has_cell_ovfl && node->cell_ovfl.cell_buf);
	assert(node && node->ncell);

	wb = PageHeadMakeWritable(node->pg);
	if(NULL == wb)
		return -1;

	btree_mgr_clear_ovfl(node);

	node->ncell -= 1;
	put_2byte(node->ncell, &wb[BTREE_HDR_NCELL_OFFSET], BTREE_HDR_NCELL_SZ);

	return 0;
}

/**
 * @brief 将溢出的cell吃回来
 */
static __INLINE__ int btree_mgr_eat_cell_ovfl(btree_node * node)
{
	unsigned char * cell = NULL;
	unsigned int cell_sz = 0;
	HMYBUFFER hbuf = NULL;
	int ret = 0;

	assert(node);
	assert(node->has_cell_ovfl && node->cell_ovfl.cell_buf);

	hbuf = MyBufferConstruct(node->btree_mgr->hm, 0);

	MyBufferRef(node->cell_ovfl.cell_buf, hbuf);
	btree_mgr_clear_ovfl(node);

	cell = MyBufferGet(hbuf, (size_t *)&cell_sz);

	ret = btree_mgr_eat_cell(node, node->cell_ovfl.idx, cell, cell_sz);

	MyBufferDestruct(hbuf);

	return ret;
}

/**
 * @brief 从节点中删除一个cell
 */
static __INLINE__ int btree_mgr_drop_cell(btree_node * node, unsigned int idx, btree_cellinfo * cell_info)
{
	unsigned int cell_ptr = 0;
	unsigned int cell_sz = 0;
	unsigned char * wb = NULL;
	unsigned int ncell_in = 0;

	assert(node && idx < node->ncell);

	wb = PageHeadMakeWritable(node->pg);
	if(NULL == wb)
		return -1;

	btree_mgr_find_cellptr(node, idx, cell_ptr);

	if(NULL == cell_info)
	{
		btree_cellinfo temp_cell_info;
		btree_mgr_parse_cellptr(node, cell_ptr, &temp_cell_info);
		cell_sz = temp_cell_info.cell_sz;
	}
	else
	{

#ifdef _DEBUG
btree_cellinfo temp_cellinfo;
btree_mgr_parse_idx(node, idx, &temp_cellinfo);
assert(0 == memcmp(&temp_cellinfo, cell_info, sizeof(temp_cellinfo)));
#endif

		assert(PageHeadMakeReadable(node->pg) == node->read_buf);
		assert((unsigned int)(cell_info->cell - node->read_buf) == cell_ptr);
		cell_sz = cell_info->cell_sz;
	}

	/* ncell计数减1,并调整cell ptr数组,idx change标识置1 */
	if(0 != btree_mgr_free_space(node, cell_ptr, cell_sz))
		return -1;

	if(node->has_cell_ovfl)
		ncell_in = node->ncell - 1;
	else
		ncell_in = node->ncell;

	/* 将后继的ptr往前整 */
	if(idx < ncell_in - 1)
	{
		unsigned int i;
		unsigned char * ptr;

		ptr = &wb[BTREE_CELL_PTR_OFFSET + idx * BTREE_CELL_PTR_SIZE];
		for(i = 0; i < ncell_in - idx - 1; i ++)
		{
			ptr[i * BTREE_CELL_PTR_SIZE] = ptr[i * BTREE_CELL_PTR_SIZE + 2];
			ptr[i * BTREE_CELL_PTR_SIZE + 1] = ptr[i * BTREE_CELL_PTR_SIZE + 3];
		}
	}

	/* ncell计数减1 */
	node->ncell -= 1;
	put_2byte(node->ncell, &wb[BTREE_HDR_NCELL_OFFSET], BTREE_HDR_NCELL_SZ);

	/* idx change标识置1 */
	node->idx_changed = 1;

	return 0;
}

/**
 * @brief 从节点中删除一条记录,放弃本节点的cell,以及后继的ovfl页(如果有)
 */
static __INLINE__ int btree_mgr_delete_record(btree_node * node, unsigned int idx, btree_cellinfo * cell_info)
{
	unsigned int pgno_ovfl = 0;
	HPAGER hpgr = NULL;

	assert(node && idx < node->ncell);

	if(cell_info)
	{
#ifdef _DEBUG
btree_cellinfo temp_cellinfo;
btree_mgr_parse_idx(node, idx, &temp_cellinfo);
assert(0 == memcmp(&temp_cellinfo, cell_info, sizeof(temp_cellinfo)));
#endif
		pgno_ovfl = cell_info->overflow_pgno;

		if(0 != btree_mgr_drop_cell(node, idx, cell_info))
			return -1;

		assert(((cell_info->data_sz + cell_info->key_sz * ((node->node_flag & e_btree_page_flag_intkey ? 0 : 1))) ==
			cell_info->local_sz &&
			0 == pgno_ovfl) || 
			(pgno_ovfl && (cell_info->data_sz + cell_info->key_sz * ((node->node_flag & e_btree_page_flag_intkey ? 0 : 1))) >
			node->btree_mgr->max_local));
	}
	else
	{
		btree_cellinfo temp_cell_info;
		btree_mgr_parse_idx(node, idx, &temp_cell_info);

		pgno_ovfl = temp_cell_info.overflow_pgno;

		if(0 != btree_mgr_drop_cell(node, idx, &temp_cell_info))
			return -1;

		assert(((temp_cell_info.data_sz + temp_cell_info.key_sz * ((node->node_flag & e_btree_page_flag_intkey ? 0 : 1))) ==
			temp_cell_info.local_sz &&
			0 == pgno_ovfl) || 
			(pgno_ovfl && (temp_cell_info.data_sz + temp_cell_info.key_sz * ((node->node_flag & e_btree_page_flag_intkey ? 0 : 1))) >
			node->btree_mgr->max_local));
	}

	assert(node->btree_mgr);
	hpgr = node->btree_mgr->hpgr;
	assert(hpgr);

	/* 如果有溢出页 */
	while(pgno_ovfl)
	{
		unsigned int temp_pgno = pgno_ovfl;
		const unsigned char * rb = NULL;
		HPAGE_HEAD pg_ovfl = PagerGetPage(hpgr, pgno_ovfl);

		if(NULL == pg_ovfl)
			return -1;

		rb = PageHeadMakeReadable(pg_ovfl);
		if(NULL == rb)
		{
			PagerReleasePage(pg_ovfl);
			return -1;
		}

		array_to_uint_as_big_endian(rb, sizeof(unsigned int), pgno_ovfl);

		if(0 != PagerReleasePage(pg_ovfl))
			return -1;

		if(0 != PagerReleasePageNo(hpgr, temp_pgno))
			return -1;
	}

	return 0;
}

/**
 * @brief 更新idx下标所指节点的lchild,如果起过ncell,则表示更新right most child
 */
static __INLINE__ int btree_mgr_update_idx_lchild(btree_node * node, unsigned int idx, unsigned int pgno_child)
{
	unsigned char * wb = NULL;
	unsigned int cell_ptr = 0;
	btree_cellinfo cell_info;

	assert((node && pgno_child) || ((node->node_flag & e_btree_page_flag_leaf) && 0 == pgno_child && idx == node->ncell));
	assert(idx <= node->ncell);
	assert(!(node->node_flag & e_btree_page_flag_leaf) || (0 == pgno_child && idx == node->ncell));
	assert(!node->has_cell_ovfl && NULL == node->cell_ovfl.cell_buf);

	wb = PageHeadMakeWritable(node->pg);
	if(NULL == wb)
		return -1;

	if(idx == node->ncell)
	{
		/* 更新right most即可 */
		uint_to_big_endian(pgno_child, &wb[BTREE_HDR_MOST_RIGHT_CHILD_OFFSET], sizeof(unsigned int));
		return 0;
	}

	btree_mgr_find_cellptr(node, idx, cell_ptr);
	btree_mgr_parse_cellptr(node, cell_ptr, &cell_info);

	uint_to_big_endian(pgno_child, &cell_info.cell[BTREE_CELL_LCHILD_OFFSET], BTREE_CELL_LCHILD_SZ);

	return 0;
}

/**
 * @brief 判断node位于父节点的idx
 */
static __INLINE__ int btree_mgr_check_idx_in_parent(btree_node * node, unsigned int * idx_in_parent)
{
	unsigned int j = 0;
	btree_cellinfo temp_cell_info;
	unsigned int cell_ptr = 0;

	/* 如果父节点不存在,或者父节点没有发生改变,不需要调用此函数 */
	assert(node && node->parent);
	assert(node->parent->idx_changed);
	assert(idx_in_parent);
	assert(!node->parent->has_cell_ovfl && NULL == node->parent->cell_ovfl.cell_buf);

	/* 在parent节点里的数组里轮询,查找 */
	for(; j < node->parent->ncell; j ++)
	{
		btree_mgr_find_cellptr(node->parent, j, cell_ptr);
		assert(cell_ptr);

		btree_mgr_parse_cellptr(node->parent, cell_ptr, &temp_cell_info);
		if(temp_cell_info.lchild_pgno != node->pg_no)
			continue;

		*idx_in_parent = j;
		break;
	}

	/* 如果找不到,说明是最右边的那一个 */
	if(j == node->parent->ncell)
	{
		unsigned int right_most;
		btree_mgr_get_rmost(node->parent, right_most);

		assert(node->pg_no == right_most);

		*idx_in_parent = node->parent->ncell;
	}

	return 0;
}

/**
 * @brief 重设缓存中的子节点parent指针与idx_in_parent
 */
static __INLINE__ int btree_mgr_reparent_child(btree_node * node)
{
	unsigned int i;
	unsigned int sub_child;
	btree_node * sub_node = NULL;

	assert(node && node->btree_mgr);

	assert(node->idx_changed);

	if(node->has_cell_ovfl)
	{
		assert(node->cell_ovfl.cell_buf);
		return 0;
	}

	if(node->node_flag & e_btree_page_flag_leaf)
	{
		node->idx_changed = 0;
		return 0;
	}

	for(i = 0; i < node->ncell; i ++)
	{
		btree_mgr_get_idx_lchild(node, i, sub_child);
		sub_node = btree_mgr_get_cached_node(node->btree_mgr, sub_child, node, i);
		if(NULL == sub_node)
			continue;

		btree_mgr_release_node(sub_node);
	}

	btree_mgr_get_rmost(node, sub_child);
	sub_node = btree_mgr_get_cached_node(node->btree_mgr, sub_child, node, node->ncell);
	if(sub_node)
		btree_mgr_release_node(sub_node);

	node->idx_changed = 0;

	return 0;
}

/**
 * @brief 拆分一个溢出的节点
 */
static int btree_mgr_div_node(btree_node * node)
{
	/*
	* 如果存在溢出记录,则要分裂结点
	* pager分配新的一页,计算分界点的payload,中间的那条记录添加至上层节点(可能会导致上层节点溢出)
	*/

	/*
	* 将原始的cell ptr数组拷贝出来,并将cell ovfl也在合适的位置添加
	* 循环解析出每个cell的大小,计算是否达到临界填充值时的分界点
	* 将分界点添加至上层节点
	* 分配新页,将剩余的cell转拷到新页上,同时相应更新上层的子节点索引,
	* 将原始页上剩余的cell依次删除
	*/
	int ret = 0;
	unsigned int * cell_array = NULL;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int k = 0;

	/* 临时变量,保存填充值 */
	unsigned int fill_sz = 0;
	/* 保存分界点在cell ptr的index */
	unsigned int idx_div = 0;
	/* 临时保存当前cell info */
	btree_cellinfo cell_info;
	/* 临时保存循环过程中的cell"指针" */
	unsigned int cell_ptr = 0;
	/* 临时变量,保存idx_div的lchild */
	unsigned int div_lchild = 0;

	btree_mgr_t * btree_mgr = NULL;

#ifdef _DEBUG
unsigned int pgr_ref = PagerGetRefCount(node->btree_mgr->hpgr);
#endif

	assert(node && node->btree_mgr);
	assert(node->has_cell_ovfl && node->cell_ovfl.cell_buf);
	assert(node->cell_ovfl.idx <= node->ncell);
	assert(node->read_buf == PageHeadMakeReadable(node->pg));

	btree_mgr = node->btree_mgr;

	/* 分配临时存储空间,用于存放cell ptr数组 */
	/* todo: 这里有内存泄漏cell_array并没有被释放 */
	cell_array = MyMemPoolMalloc(btree_mgr->hm, node->ncell * sizeof(cell_array[0]));
	if(NULL == cell_array)
		return -1;

	for(; i < (node->ncell - 1) && k < node->ncell; i ++, k ++)
	{
		if(node->cell_ovfl.idx == i)
		{
			/* 0表示溢出的那条记录 */
			cell_array[k] = 0;
			k += 1;
		}

		btree_mgr_find_cellptr(node, i, cell_ptr);

		cell_array[k] = cell_ptr;
	}

	/* 循环解析出每个cell的大小,计算是否达到临界填充值时的分界点 */
	for(i = 0; i < node->ncell; i ++)
	{
		if(cell_array[i])
			btree_mgr_parse_cellptr(node, cell_array[i], &cell_info);
		else
		{
			/* 如果是溢出的那个cell,直接从cell_ovfl信息里取 */
			unsigned int temp_len = 0;
			const unsigned char * pctemp = MyBufferGet(node->cell_ovfl.cell_buf, (size_t *)&temp_len);
			btree_mgr_parse_cell(node, pctemp, temp_len, &cell_info);
		}

		fill_sz += btree_mgr_cal_cell_real_fill_sz(cell_info.cell_sz);
		if(fill_sz < btree_mgr->pg_min_fill_sz)
			continue;

		idx_div = i + 1;
		break;
	}

	assert((idx_div < node->ncell) && (idx_div >= 1));

	/* 取出分界点的lchild */
	if(cell_array[idx_div])
	{
		btree_mgr_parse_cellptr(node, cell_array[idx_div], &cell_info);
	}
	else
	{
		/* 如果是溢出的那个cell,直接从cell_ovfl信息里取 */
		unsigned int temp_len = 0;
		const unsigned char * pctemp = MyBufferGet(node->cell_ovfl.cell_buf, (size_t *)&temp_len);
		btree_mgr_parse_cell(node, pctemp, temp_len, &cell_info);
	}
	div_lchild = cell_info.lchild_pgno;

	/*
	* 将分界点添加至上层节点
	* 这里需要判断是否是根节点,如果是根节点,需要分配两个新页
	*/
	if(node->parent)
	{
		/* todo:可以考虑将idx_div前面的cell移至new_node,这样可以减少一些内存操作 */

		/* 非根节点,添加至上层节点即可 */
		btree_node * new_node = NULL;
		unsigned int idx_ovfl = 0;

		unsigned int idx_in_parent = node->idx_in_parent_cellarray;
		if(node->parent->idx_changed)
			btree_mgr_check_idx_in_parent(node, &idx_in_parent);

		/* 将idx_div之后的元素转拷至新的页,并更改parent的相应的"指针" */
		new_node = btree_mgr_new_node(btree_mgr, node->parent, idx_in_parent + 1, node->node_flag);
		if(NULL == new_node)
		{
			ret = -1;
			goto btree_mgr_div_node_end_;
		}

		/* 更新parent的idex_in_parent对应的lchild,要提前更新,因为添加有可能导致父节点溢出 */
		if(0 != btree_mgr_update_idx_lchild(node->parent, idx_in_parent, new_node->pg_no))
		{
			btree_mgr_delete_node(new_node);

			ret = -1;
			goto btree_mgr_div_node_end_;
		}

		/*
		* 往parent里添加cell,有可能导致parent溢出,所以先设好cell的lchild域
		* 同时要考虑,叶子节点的cell进入非叶子节点的情况
		*/
		{
			assert(!(node->parent->node_flag & e_btree_page_flag_leaf));
			if(!(node->node_flag & e_btree_page_flag_leaf))
			{
				unsigned char * cell = MyMemPoolMalloc(btree_mgr->hm, cell_info.cell_sz);
				if(NULL == cell)
				{
					btree_mgr_delete_node(new_node);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}

				memcpy(cell, cell_info.cell, cell_info.cell_sz);
				uint_to_big_endian(node->pg_no, cell, sizeof(unsigned int));
				if(0 != btree_mgr_insert_cell(node->parent, idx_in_parent,
					cell, cell_info.cell_sz))
				{
					btree_mgr_delete_node(new_node);
					MyMemPoolFree(btree_mgr->hm, cell);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}
				MyMemPoolFree(btree_mgr->hm, cell);
			}
			else
			{
				/* 叶子节点进入非叶子节点的情况,要多分配4个字节用于存储页号 */
				unsigned char * cell = MyMemPoolMalloc(btree_mgr->hm, cell_info.cell_sz + sizeof(unsigned int));
				if(NULL == cell)
				{
					btree_mgr_delete_node(new_node);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}

				memcpy(cell + sizeof(unsigned int), cell_info.cell, cell_info.cell_sz);
				uint_to_big_endian(node->pg_no, cell, sizeof(unsigned int));

				if(0 != btree_mgr_insert_cell(node->parent, idx_in_parent,
					cell, cell_info.cell_sz + sizeof(unsigned int)))
				{
					btree_mgr_delete_node(new_node);
					MyMemPoolFree(btree_mgr->hm, cell);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}

				MyMemPoolFree(btree_mgr->hm, cell);
			}
		}

		idx_ovfl = node->cell_ovfl.idx;

		j = 0;
		for(i = idx_div + 1; i < node->ncell; i ++)
		{
			/* 先将相应的cell转存至新页,然后删除 */

			if(idx_ovfl != i)
			{
				/* 如果不是cell ovfl,从原始页里取cell转存 */
				btree_mgr_parse_cellptr(node, cell_array[i], &cell_info);

				if(0 != btree_mgr_insert_cell(new_node, j, cell_info.cell, cell_info.cell_sz))
				{
					btree_mgr_delete_node(new_node);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}
			}
			else
			{
				/* 如果是cell ovfl,把ovfl的存入 */
				unsigned int sz_temp = 0;
				unsigned char * pctemp = NULL;

				assert(node->has_cell_ovfl && idx_div != idx_ovfl && node->cell_ovfl.cell_buf);

				pctemp = MyBufferGet(node->cell_ovfl.cell_buf, (size_t *)&sz_temp);

				if(0 != btree_mgr_insert_cell(new_node, j, pctemp, sz_temp))
				{
					btree_mgr_delete_node(new_node);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}
			}
			j ++;
		}

		assert(node->ncell > 1);
		for(i = (node->ncell - 1); i >= idx_div; i --)
		{
			/* 从原始页去除相应的cell,或者去除cell ovfl信息 */
			if(i < idx_ovfl)
			{
				if(0 != btree_mgr_drop_cell(node, i, NULL))
				{
					btree_mgr_delete_node(new_node);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}
			}
			else if(i > idx_ovfl)
			{
				if(0 != btree_mgr_drop_cell(node, i - 1, NULL))
				{
					btree_mgr_delete_node(new_node);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}
			}
			else
			{
				//assert(node->has_cell_ovfl && idx_div != idx_ovfl && node->cell_ovfl.cell_buf);
				assert(node->has_cell_ovfl && node->cell_ovfl.cell_buf);
				if(0 != btree_mgr_throwup_cell_ovfl(node))
				{
					btree_mgr_delete_node(new_node);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}
			}
		}

		/* 如果溢出cell在分割点之前 */
		if(node->cell_ovfl.idx < idx_div)
		{
			/* 如果是cell ovfl,把ovfl的存入 */
			int ret = 0;

			assert(node->has_cell_ovfl && node->cell_ovfl.cell_buf);

			/* 注意此处会引起之前解析过的cellinfo无效 */
			ret = btree_mgr_eat_cell_ovfl(node);

			if(0 != ret)
			{
				btree_mgr_delete_node(new_node);

				ret = -1;
				goto btree_mgr_div_node_end_;
			}
		}

		assert(!node->has_cell_ovfl && !new_node->has_cell_ovfl);

		/* 更新parent idx_in_parent + 1对应cell的lchild */
		assert(idx_in_parent + 1 <= node->parent->ncell);

		assert(new_node->node_flag == node->node_flag);

		if(!(node->node_flag & e_btree_page_flag_leaf))
		{
			/* 如果new_node不为叶子节点,更新new_node的 rightmost 为node的right most */
			unsigned int node_rmost = 0;
			btree_mgr_get_rmost(node, node_rmost);
			if(0 != btree_mgr_update_idx_lchild(new_node, new_node->ncell, node_rmost))
			{
				btree_mgr_delete_node(new_node);

				ret = -1;
				goto btree_mgr_div_node_end_;
			}
			/* 更新node rightmost 为idx_div的lchild*/
			if(0 != btree_mgr_update_idx_lchild(node, node->ncell, div_lchild))
			{
				btree_mgr_delete_node(new_node);

				ret = -1;
				goto btree_mgr_div_node_end_;
			}
		}
		else
		{
			assert(0 == div_lchild);
		}

		assert(!node->has_cell_ovfl && NULL == node->cell_ovfl.cell_buf);
		assert(!new_node->has_cell_ovfl && NULL == new_node->cell_ovfl.cell_buf);

		assert((BTREE_NODE_PAYLOAD(node->btree_mgr) - node->nfree) > node->btree_mgr->pg_min_fill_sz);
		assert((BTREE_NODE_PAYLOAD(new_node->btree_mgr) - new_node->nfree) > node->btree_mgr->pg_min_fill_sz);

		btree_mgr_reparent_child(node->parent);
		assert(0 == node->parent->idx_changed || (node->parent->has_cell_ovfl && node->parent->cell_ovfl.cell_buf));
		assert(PagerGetPageRefCount(node->parent->pg) >= 2);

		btree_mgr_reparent_child(node);
		assert(0 == node->idx_changed);

		btree_mgr_reparent_child(new_node);
		assert(0 == new_node->idx_changed);

		/* 取消对node的引用 */
		btree_mgr_release_node(new_node);

#ifdef _DEBUG
/* 在一次平衡过程中,因为增加了一个节点,引用计数增加应不超过1 */
assert(PagerGetRefCount(node->btree_mgr->hpgr) <= (pgr_ref + 1));
#endif

	}
	else
	{
		/*
		* 如果是根节点
		* 分配两节点出来,idx_div前后的节点分别加新的这两页节点
		*/

		unsigned int i = 0;
		unsigned int j = 0;
		unsigned int idx_ovfl = 0;

		btree_node * new_node1 = NULL;
		btree_node * new_node2 = NULL;

		btree_cellinfo temp_cell_info;

		new_node1 = btree_mgr_new_node(btree_mgr, node, 0, node->node_flag);
		new_node2 = btree_mgr_new_node(btree_mgr, node, 1, node->node_flag);

		if(NULL == new_node1 || NULL == new_node2)
		{
			if(new_node1)
				btree_mgr_delete_node(new_node1);

			if(new_node2)
				btree_mgr_delete_node(new_node2);

			ret = -1;
			goto btree_mgr_div_node_end_;
		}

		/* 将idx_div之前的转存入new_node1 */
		idx_ovfl = node->cell_ovfl.idx;
		for(j = 0, i = 0; i < idx_div; i ++)
		{
			if(i !=  idx_ovfl)
			{
				btree_mgr_parse_cellptr(node, cell_array[i], &temp_cell_info);
				if(0 != btree_mgr_insert_cell(new_node1, j, temp_cell_info.cell, temp_cell_info.cell_sz))
				{
					btree_mgr_delete_node(new_node1);
					btree_mgr_delete_node(new_node2);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}
			}
			else
			{
				/* 如果是cell ovfl,把ovfl的存入 */
				unsigned int sz_temp = 0;
				unsigned char * pctemp = NULL;

				assert(node->has_cell_ovfl && idx_div != idx_ovfl && node->cell_ovfl.cell_buf);

				pctemp = MyBufferGet(node->cell_ovfl.cell_buf, (size_t *)&sz_temp);

				if(0 != btree_mgr_insert_cell(new_node1, j, pctemp, sz_temp))
				{
					btree_mgr_delete_node(new_node1);
					btree_mgr_delete_node(new_node2);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}
			}
			j ++;
		}

		/* 将idx_div之后的转存入new_node2 */
		for(j = 0, i = idx_div + 1; i < node->ncell; i ++)
		{
			if(i !=  idx_ovfl)
			{
				btree_mgr_parse_cellptr(node, cell_array[i], &temp_cell_info);
				if(0 != btree_mgr_insert_cell(new_node2, j, temp_cell_info.cell, temp_cell_info.cell_sz))
				{
					btree_mgr_delete_node(new_node1);
					btree_mgr_delete_node(new_node2);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}
			}
			else
			{
				/* 如果是cell ovfl,把ovfl的存入 */
				unsigned int sz_temp = 0;
				unsigned char * pctemp = NULL;

				assert(node->has_cell_ovfl && idx_div != idx_ovfl && node->cell_ovfl.cell_buf);

				pctemp = MyBufferGet(node->cell_ovfl.cell_buf, (size_t *)&sz_temp);

				if(0 != btree_mgr_insert_cell(new_node2, j, pctemp, sz_temp))
				{
					btree_mgr_delete_node(new_node1);
					btree_mgr_delete_node(new_node2);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}
			}
			j ++;
		}

		assert(!new_node1->has_cell_ovfl && !new_node2->has_cell_ovfl);

		/* 更新new_node2的right most */
		if(!(node->node_flag & e_btree_page_flag_leaf))
		{
			unsigned int node_rmost = 0;
			btree_mgr_get_rmost(node, node_rmost);
			if(0 != btree_mgr_update_idx_lchild(new_node2, new_node2->ncell, node_rmost))
			{
				btree_mgr_delete_node(new_node1);
				btree_mgr_delete_node(new_node2);

				ret = -1;
				goto btree_mgr_div_node_end_;
			}

			/* 更新new_node1的right most */
			if(0 != btree_mgr_update_idx_lchild(new_node1, new_node1->ncell, div_lchild))
			{
				btree_mgr_delete_node(new_node1);
				btree_mgr_delete_node(new_node2);

				ret = -1;
				goto btree_mgr_div_node_end_;
			}
		}
		else
		{
			/* 如果node是叶节点,new_node2一定为叶节点,不用更新right most */
			assert(new_node1->node_flag & e_btree_page_flag_leaf);
			assert(new_node2->node_flag & e_btree_page_flag_leaf);
			assert(0 == div_lchild);
		}


		/* 将idx_div cell拷贝出来,然后将node清空,再将cell拷回node */
		/* 如果idx_div正巧是cell ovfl,则还要将它存入node */
		{
			unsigned char * cell = NULL;
			unsigned int cell_sz = 0;
			if(node->node_flag & e_btree_page_flag_leaf)
			{
				assert((new_node1->node_flag & e_btree_page_flag_leaf) &&
					(new_node2->node_flag & e_btree_page_flag_leaf));

				cell_sz = cell_info.cell_sz + sizeof(unsigned int);
				cell = MyMemPoolMalloc(btree_mgr->hm, cell_sz);
				if(NULL == cell)
				{
					btree_mgr_delete_node(new_node1);
					btree_mgr_delete_node(new_node2);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}

				uint_to_big_endian(new_node1->pg_no, cell, sizeof(unsigned int));
				memcpy(cell + sizeof(unsigned int), cell_info.cell, cell_info.cell_sz);

				/* 如果node依然为叶子节点,需要将它改成非叶子节点 */
				if(0 != btree_mgr_set_nodeflag(node, (unsigned char)(node->node_flag & (0xff & ~e_btree_page_flag_leaf))))
				{
					btree_mgr_delete_node(new_node1);
					btree_mgr_delete_node(new_node2);

					MyMemPoolFree(btree_mgr->hm, cell);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}
			}
			else
			{
				assert(!(new_node1->node_flag & e_btree_page_flag_leaf) &&
					!(new_node2->node_flag & e_btree_page_flag_leaf));

				cell_sz = cell_info.cell_sz;
				cell = MyMemPoolMalloc(btree_mgr->hm, cell_sz);
				if(NULL == cell)
				{
					btree_mgr_delete_node(new_node1);
					btree_mgr_delete_node(new_node2);

					ret = -1;
					goto btree_mgr_div_node_end_;
				}

				memcpy(cell, cell_info.cell, cell_info.cell_sz);
				uint_to_big_endian(new_node1->pg_no, cell, sizeof(unsigned int));
			}

			assert(cell);

			if(btree_mgr_clear_node(node))
			{
				btree_mgr_delete_node(new_node1);
				btree_mgr_delete_node(new_node2);

				MyMemPoolFree(btree_mgr->hm, cell);

				ret = -1;
				goto btree_mgr_div_node_end_;
			}

			assert(0 == node->ncell);

			/* node的0的lchild指向new_node1,并将向拷回node */
			if(0 != btree_mgr_insert_cell(node, 0, cell, cell_sz))
			{
				btree_mgr_delete_node(new_node1);
				btree_mgr_delete_node(new_node2);

				MyMemPoolFree(btree_mgr->hm, cell);

				ret = -1;
				goto btree_mgr_div_node_end_;
			}

			MyMemPoolFree(btree_mgr->hm, cell);
		}

		assert(1 == node->ncell);

		/* node的right most 指向new_node2 */
		if(0 != btree_mgr_update_idx_lchild(node, 1, new_node2->pg_no))
		{
			btree_mgr_delete_node(new_node1);
			btree_mgr_delete_node(new_node2);

			ret = -1;
			goto btree_mgr_div_node_end_;
		}

		btree_mgr_reparent_child(node);
		assert(PagerGetPageRefCount(node->pg));

		btree_mgr_reparent_child(new_node1);
		btree_mgr_reparent_child(new_node2);

		assert(PagerGetPageRefCount(node->pg) >= 2);

		assert(0 == node->idx_changed);
		assert(0 == new_node1->idx_changed);
		assert(0 == new_node2->idx_changed);

		assert(!node->has_cell_ovfl && NULL == node->cell_ovfl.cell_buf);
		assert(!new_node1->has_cell_ovfl && NULL == new_node1->cell_ovfl.cell_buf);
		assert(!new_node2->has_cell_ovfl && NULL == new_node2->cell_ovfl.cell_buf);

		assert((BTREE_NODE_PAYLOAD(new_node1->btree_mgr) - new_node1->nfree) > new_node1->btree_mgr->pg_min_fill_sz);
		assert((BTREE_NODE_PAYLOAD(new_node2->btree_mgr) - new_node2->nfree) > new_node2->btree_mgr->pg_min_fill_sz);

		/* 取消对new_node1与new_node2的引用 */
		btree_mgr_release_node(new_node1);
		btree_mgr_release_node(new_node2);

#ifdef _DEBUG
/* 增加了两个节点,引用计数增加应不超过二 */
assert(PagerGetRefCount(node->btree_mgr->hpgr) <= (pgr_ref + 2));
#endif
	}

btree_mgr_div_node_end_:

	assert(cell_array);
	MyMemPoolFree(btree_mgr->hm, cell_array);
	return 0;
}

/**
 * @brief 递归获取一棵b树的记录总数
 */
static unsigned int btree_mgr_get_count(btree_cursor * cursor, btree_node * node)
{
	unsigned int i;
	unsigned int count = 0;
	btree_node * sub_node = NULL;
	unsigned int sub_pgno = 0;
	btree_cellinfo cell_info;

	assert(cursor && node);

	count = node->ncell;

	if(node->node_flag & e_btree_page_flag_leaf)
		return count;

	for(i = 0; i <= node->ncell; i ++)
	{
		if(i == node->ncell)
		{
			btree_mgr_get_rmost(node, sub_pgno);
		}
		else
		{
			btree_mgr_parse_idx(node, i, &cell_info);
			sub_pgno = cell_info.lchild_pgno;
		}

		sub_node = btree_mgr_get_node(cursor->btree_mgr, sub_pgno, node, i);

		count += btree_mgr_get_count(cursor, sub_node);

		btree_mgr_release_node(sub_node);
	}

	return count;
}

/**
 * @brief 根据需要合并或者均分node1, node2, node1与node2是相邻的节点且node1在node2"前面"
 */
static __INLINE__ int btree_mgr_together_or_average_tow_nodes(btree_mgr_t * btree_mgr,
															  btree_node * node1, 
															  btree_node * node2,
															  unsigned int idx_node1_in_parent,
															  btree_node * parent)
{
	/* 左邻居存在,合在一起,如果不超过一个节点的容量,则就成了一个节点,如果超过了,均成两块 */

	/*
	* 先将两节点合并,连同idx_in_parent,形成一个大的节点,然后均成两块
	* 如果合并后的节点超过了一个节点的大小,此时一定是可以均成填充率超过40%的两块,因为max local不超过10%,去10-%给parent,90+%可以均成两块
	* 如果合并后节点不超过一个节点的大小,结束返回
	* 注意,如果此前位于叶节点那一层,父节点下来的cell在做相应的修正
	*/

	unsigned int together_fill_sz = 0;
	unsigned char * cell_parent = NULL;
	unsigned int cell_parent_sz = 0;

	btree_cellinfo cell_info;

	assert(node1 && node2 && btree_mgr && parent);
	assert(btree_mgr == node1->btree_mgr && btree_mgr == node2->btree_mgr);
	assert(parent == node1->parent && parent == node2->parent);
	assert(idx_node1_in_parent < parent->ncell);

	btree_mgr_parse_idx(parent, idx_node1_in_parent, &cell_info);
	cell_parent = cell_info.cell;
	cell_parent_sz = cell_info.cell_sz;

	assert(node1->node_flag == node2->node_flag);

	if(node2->node_flag & e_btree_page_flag_leaf)
	{
		cell_parent += BTREE_CELL_LCHILD_SZ;
		cell_parent_sz -= BTREE_CELL_LCHILD_SZ;
	}

	together_fill_sz = 2 * (btree_mgr->pg_sz - BTREE_HDR_SZ) - (node1->nfree + node2->nfree) +
		btree_mgr_cal_cell_real_fill_sz(cell_parent_sz);

	if(together_fill_sz < (btree_mgr->pg_sz - BTREE_HDR_SZ))
	{
		if(parent->ncell <= 1 && NULL == parent->parent)
		{
			/*
			* 这里讨论parent为根节点的情况
			* 如果根节点只剩一个cell了
			* 那将node1 node2合并存入根节点
			*/

			unsigned int i;

			assert(0 == idx_node1_in_parent);

			assert(!(parent->node_flag & e_btree_page_flag_leaf));
			if(!(node1->node_flag & e_btree_page_flag_leaf))
			{
				unsigned int node1_rmost = 0;
				unsigned int node2_rmost = 0;
				btree_mgr_get_rmost(node1, node1_rmost);
				btree_mgr_get_rmost(node2, node2_rmost);
				if(0 != btree_mgr_update_idx_lchild(parent, 0, node1_rmost))
					return -1;
				if(0 != btree_mgr_update_idx_lchild(parent, 1, node2_rmost))
					return -1;
			}
			else
			{
				/* 此时parent将转变成叶子节点了 */
				unsigned char * cell = MyMemPoolMalloc(btree_mgr->hm, cell_parent_sz);

				assert((cell_info.cell_sz - BTREE_CELL_LCHILD_SZ) == cell_parent_sz);
				assert((cell_info.cell + BTREE_CELL_LCHILD_SZ) == cell_parent);

				memcpy(cell, cell_parent, cell_parent_sz);

				/* 将唯一的cell去除 */
				if(0 != btree_mgr_drop_cell(parent, 0, &cell_info))
				{
					MyMemPoolFree(btree_mgr->hm, cell);
					return -1;
				}

				/* 将根节点的node_flag补充叶子节点的标识 */
				if(0 != btree_mgr_set_nodeflag(parent, (unsigned char)(parent->node_flag | e_btree_page_flag_leaf)))
				{
					MyMemPoolFree(btree_mgr->hm, cell);
					return -1;
				}

				/* 将改造后的cell加入 */
				if(0 != btree_mgr_insert_cell(parent, 0, cell, cell_parent_sz))
				{
					MyMemPoolFree(btree_mgr->hm, cell);
					return -1;
				}

				MyMemPoolFree(btree_mgr->hm, cell);

				/* right most必为0 */
				if(0 != btree_mgr_update_idx_lchild(parent, 1, 0))
					return -1;
			}

			/* 将node1里的cell转存至根节点 */
			assert(parent->node_flag == node1->node_flag);
			for(i = node1->ncell; i; i --)
			{
				btree_mgr_parse_idx(node1, i - 1, &cell_info);
				if(0 != btree_mgr_insert_cell(parent, 0, cell_info.cell, cell_info.cell_sz))
					return -1;
			}

			/* 将node2里的cell转存至根节点 */
			assert(parent->node_flag == node2->node_flag);
			for(i = 0; i < node2->ncell; i ++)
			{
				btree_mgr_parse_idx(node2, i, &cell_info);
				if(0 != btree_mgr_insert_cell(parent, parent->ncell, cell_info.cell, cell_info.cell_sz))
					return -1;
			}

			btree_mgr_ref_node(node1);
			btree_mgr_ref_node(node2);

			/* 重定向根节点的所有子节点的parent指针 */
			btree_mgr_reparent_child(parent);

			/* 删除node1 node2 */
			if(0 != btree_mgr_delete_node(node1))
			{
				btree_mgr_release_node(node1);
				btree_mgr_release_node(node2);
				return -1;
			}

			if(0 != btree_mgr_delete_node(node2))
			{
				btree_mgr_release_node(node2);
				return -1;
			}

			return 0;
		}
		else
		{
			/* 可以合并成一个节点 */
			unsigned int i;

			assert(parent->ncell > 1);

			/* 将父节点的idx_node1_in_parent(以下简称pull down node)加入和node2 */
			if(0 != btree_mgr_insert_cell(node2, 0, cell_parent, cell_parent_sz))
				return -1;
			assert(!node2->has_cell_ovfl && NULL == node2->cell_ovfl.cell_buf);

			if(!(node1->node_flag & e_btree_page_flag_leaf))
			{
				unsigned int node1_rmost = 0;

				/* pull down node的lchild更新为node1的right most */
				btree_mgr_get_rmost(node1, node1_rmost);

				assert(!(node2->node_flag & e_btree_page_flag_leaf));
				if(0 != btree_mgr_update_idx_lchild(node2, 0, node1_rmost))
					return -1;
			}

			if(0 != btree_mgr_drop_cell(parent, idx_node1_in_parent, &cell_info))
				return -1;

			/* 将node1的每个cell依次拷贝至node2 */
			for(i = node1->ncell; i; i --)
			{
				btree_mgr_parse_idx(node1, i - 1, &cell_info);
				if(0 != btree_mgr_insert_cell(node2, 0, cell_info.cell, cell_info.cell_sz))
					return -1;
			}

			assert(!node2->has_cell_ovfl && NULL == node2->cell_ovfl.cell_buf);

			btree_mgr_reparent_child(node2);
			btree_mgr_reparent_child(parent);

			assert(0 == node2->idx_changed);
			assert(0 == parent->idx_changed);

			assert(!node2->has_cell_ovfl && NULL == node2->cell_ovfl.cell_buf);
			assert(!parent->has_cell_ovfl && NULL == parent->cell_ovfl.cell_buf);

			assert((BTREE_NODE_PAYLOAD(node2->btree_mgr) - node2->nfree) > node2->btree_mgr->pg_min_fill_sz);

			/* 因为这里多做一次release,此处对node1引用计数增1,防止node1被提前释放了 */
			/* 删除lnode节点 */
			btree_mgr_ref_node(node1);
			if(0 != btree_mgr_delete_node(node1))
				return -1;

			return 0;
		}
	}
	else if((BTREE_NODE_PAYLOAD(btree_mgr) - node1->nfree) > btree_mgr->pg_min_fill_sz)
	{
		/* 需要均成两个节点 */
		unsigned int div_fill_sz = BTREE_NODE_PAYLOAD(btree_mgr) - node2->nfree + btree_mgr_cal_cell_real_fill_sz(cell_parent_sz);
		/* node1中当填充满足aver_fill_sz的临界cell的索引 */
		unsigned int div_idx = 0;
		unsigned int j;

        assert((BTREE_NODE_PAYLOAD(btree_mgr) - node2->nfree) < btree_mgr->pg_min_fill_sz);

		/* 将父节点的idx_node1_in_parent(以下简称pull down node)加入node2 */
		if(0 != btree_mgr_insert_cell(node2, 0, cell_parent, cell_parent_sz))
			return -1;
		assert(!node2->has_cell_ovfl && NULL == node2->cell_ovfl.cell_buf);

		if(!(node1->node_flag & e_btree_page_flag_leaf))
		{
			unsigned int node1_rmost = 0;

			assert(!(node2->node_flag & e_btree_page_flag_leaf));

			/* pull down node的lchild更新为node1的right most */
			btree_mgr_get_rmost(node1, node1_rmost);
			if(0 != btree_mgr_update_idx_lchild(node2, 0, node1_rmost))
				return -1;
		}

		if(0 != btree_mgr_drop_cell(parent, idx_node1_in_parent, &cell_info))
			return -1;

		for(j = node1->ncell; j; j --)
		{
			if(div_fill_sz >= btree_mgr->pg_min_fill_sz)
			{
				div_idx = j - 1;
				break;
			}

			btree_mgr_parse_idx(node1, j - 1, &cell_info);
			div_fill_sz += btree_mgr_cal_cell_real_fill_sz(cell_info.cell_sz);
		}

		/* 分界点必然在node1里的某个cell */
		assert(div_idx < node1->ncell);

		for(j = node1->ncell - 1; j >= div_idx + 1; j --)
		{
			btree_mgr_parse_idx(node1, j, &cell_info);
			if(0 != btree_mgr_insert_cell(node2, 0, cell_info.cell, cell_info.cell_sz))
				return -1;

			/* todo:we have bug here? */
			if(0 != btree_mgr_drop_cell(node1, j, &cell_info))
				return -1;
		}

		btree_mgr_parse_idx(node1, div_idx, &cell_info);

		/* 更新node1的right most为div_idx的lchild */
		if(!(node1->node_flag & e_btree_page_flag_leaf))
		{
			if(0 != btree_mgr_update_idx_lchild(node1, node1->ncell, cell_info.lchild_pgno))
				return -1;
		}
		else
			assert(0 == cell_info.lchild_pgno);

		{			
			unsigned char * cell = NULL;
			unsigned int cell_sz = 0;

			assert(!(parent->node_flag & e_btree_page_flag_leaf));

			if(node1->node_flag & e_btree_page_flag_leaf)				
				cell_sz = cell_info.cell_sz + BTREE_CELL_LCHILD_SZ;/* 如果node1是叶节点,需要对解析出来的cell补入4个字节的lchild填充位 */
			else
				cell_sz = cell_info.cell_sz;

			assert(cell_sz >= BTREE_CELL_MIN_SZ);

			cell = MyMemPoolMalloc(btree_mgr->hm, cell_sz);
			if(NULL == cell)
				return -1;

			if(node1->node_flag & e_btree_page_flag_leaf)
				memcpy(cell + BTREE_CELL_LCHILD_SZ, cell_info.cell, cell_info.cell_sz);
			else
				memcpy(cell, cell_info.cell, cell_info.cell_sz);

			/* 更新parent里idx_in_parent - 1这个位置的lchild */
			uint_to_big_endian(node1->pg_no, cell, BTREE_CELL_LCHILD_SZ);

			/* 将div_idx对应的cell存入parent的idx_node1_in_parent这个位置 */
			if(0 != btree_mgr_insert_cell(parent, idx_node1_in_parent, cell, cell_sz))
			{
				MyMemPoolFree(btree_mgr->hm, cell);
				return -1;
			}

			MyMemPoolFree(btree_mgr->hm, cell);
		}

		/* 从node1里去除div_dix这个cell */
		if(0 != btree_mgr_drop_cell(node1, div_idx, &cell_info))
			return -1;

		assert(!node1->has_cell_ovfl && NULL == node1->cell_ovfl.cell_buf);
		assert(!node2->has_cell_ovfl && NULL == node2->cell_ovfl.cell_buf);

		assert((BTREE_NODE_PAYLOAD(node1->btree_mgr) - node1->nfree) > node1->btree_mgr->pg_min_fill_sz);
		assert((BTREE_NODE_PAYLOAD(node2->btree_mgr) - node2->nfree) > node2->btree_mgr->pg_min_fill_sz);

		btree_mgr_reparent_child(node1);
		btree_mgr_reparent_child(node2);
		btree_mgr_reparent_child(parent);

		assert(0 == node1->idx_changed);
		assert(0 == node2->idx_changed);
		assert(0 == parent->idx_changed || (parent->has_cell_ovfl && parent->cell_ovfl.cell_buf));

		return 0;
	}
	else if((BTREE_NODE_PAYLOAD(btree_mgr) - node1->nfree) < btree_mgr->pg_min_fill_sz)
	{
		/* 需要均成两个节点 */
		unsigned int div_fill_sz = BTREE_NODE_PAYLOAD(btree_mgr) - node1->nfree + btree_mgr_cal_cell_real_fill_sz(cell_parent_sz);
		/* lnode中当填充满足aver_fill_sz的临界cell的索引 */
		unsigned int div_idx = 0;
		unsigned int j;

        assert((BTREE_NODE_PAYLOAD(btree_mgr) - node2->nfree) > btree_mgr->pg_min_fill_sz);

		/* 将父节点的idx_node1_in_parent(以下简称pull down node)加入和node */
		if(0 != btree_mgr_insert_cell(node1, node1->ncell, cell_parent, cell_parent_sz))
			return -1;
		assert(!node1->has_cell_ovfl && NULL == node1->cell_ovfl.cell_buf);

		if(!(node1->node_flag & e_btree_page_flag_leaf))
		{
			unsigned int node1_rmost = 0;

			/* pull down node的lchild更新为node1的right most */
			btree_mgr_get_rmost(node1, node1_rmost);
			if(0 != btree_mgr_update_idx_lchild(node1, node1->ncell - 1, node1_rmost))
				return -1;
		}

		if(0 != btree_mgr_drop_cell(parent, idx_node1_in_parent, &cell_info))
			return -1;

		/* 从node2的第一个cell开始查去 */
		for(j = 0; j < node2->ncell; j ++)
		{
			if(div_fill_sz >= btree_mgr->pg_min_fill_sz)
			{
				div_idx = j;
				break;
			}

			btree_mgr_parse_idx(node2, j, &cell_info);
			div_fill_sz += btree_mgr_cal_cell_real_fill_sz(cell_info.cell_sz);
		}

		assert(div_idx < node2->ncell - 1);

		if(div_idx)
		{
			/* 将node2 div_idx之前的cell拷贝至node1未尾 */
			for(j = div_idx; j; j --)
			{
				btree_mgr_parse_idx(node2, 0, &cell_info);
				if(0 != btree_mgr_insert_cell(node1, node1->ncell, cell_info.cell, cell_info.cell_sz))
					return -1;

				/* todo:we have bug here? */
				if(0 != btree_mgr_drop_cell(node2, 0, &cell_info))
					return -1;
			}
		}

		btree_mgr_parse_idx(node2, 0, &cell_info);
		
		if(!(node1->node_flag & e_btree_page_flag_leaf))
		{
			/* 更新node1的right most为div_idx的lchild */
			if(0 != btree_mgr_update_idx_lchild(node1, node1->ncell, cell_info.lchild_pgno))
				return -1;
		}
		else
			assert(0 == cell_info.lchild_pgno);


		{
			unsigned char * cell = NULL;
			unsigned int cell_sz = 0;

			assert(!(parent->node_flag & e_btree_page_flag_leaf));

			if(node2->node_flag & e_btree_page_flag_leaf)				
				cell_sz = cell_info.cell_sz + BTREE_CELL_LCHILD_SZ;/* 如果node1是叶节点,需要对解析出来的cell补入4个字节的lchild填充位 */
			else
				cell_sz = cell_info.cell_sz;

			assert(cell_sz >= BTREE_CELL_MIN_SZ);

			cell = MyMemPoolMalloc(btree_mgr->hm, cell_sz);
			if(NULL == cell)
				return -1;

			if(node2->node_flag & e_btree_page_flag_leaf)
				memcpy(cell + BTREE_CELL_LCHILD_SZ, cell_info.cell, cell_info.cell_sz);
			else
				memcpy(cell, cell_info.cell, cell_info.cell_sz);

			/* 更新parent里idx_in_parent - 1这个位置的lchild */
			uint_to_big_endian(node1->pg_no, cell, BTREE_CELL_LCHILD_SZ);

			/* 将div_idx转存至parent */
			if(0 != btree_mgr_insert_cell(parent, idx_node1_in_parent, cell, cell_sz))
			{
				MyMemPoolFree(btree_mgr->hm, cell);
				return -1;
			}

			MyMemPoolFree(btree_mgr->hm, cell);
		}

		if(0 != btree_mgr_drop_cell(node2, 0, &cell_info))
			return -1;

		assert(!node1->has_cell_ovfl && NULL == node1->cell_ovfl.cell_buf);
		assert(!node2->has_cell_ovfl && NULL == node2->cell_ovfl.cell_buf);

		assert((BTREE_NODE_PAYLOAD(node1->btree_mgr) - node1->nfree) > node1->btree_mgr->pg_min_fill_sz);
		assert((BTREE_NODE_PAYLOAD(node2->btree_mgr) - node2->nfree) > node2->btree_mgr->pg_min_fill_sz);

		btree_mgr_reparent_child(node1);
		btree_mgr_reparent_child(node2);
		btree_mgr_reparent_child(parent);

		assert(0 == node1->idx_changed);
		assert(0 == node2->idx_changed);
		assert(0 == parent->idx_changed || (parent->has_cell_ovfl && parent->cell_ovfl.cell_buf));

		return 0;
	}
	else
	{
		assert(0);
		return -1;
	}
}

/**
 * @brief 在填充率不足的情况下,通过向邻节点借cell,或者合并邻节点,补足填充率
 */
static int btree_mgr_fill_node(btree_node * node)
{
	/*
	* 如果当前的节点的填充率低于40%
	* 从左右邻居借节点,从上层结点拉一块下来,从邻居节点借一条记录给上层(注意,此时有可能导致上层产生溢出).
	* 如果没法借,则合并结点,将上层的分割结点拉至合并后的节点的适合位置
	*/
	
	int ret = 0;

	btree_mgr_t * btree_mgr = NULL;

	/* 记录左邻居的页号,如果存在的话 */
	unsigned int lsibling = 0;
	/* 记录右邻居的页号,如果存在的话 */
	unsigned int rsibling = 0;
	/* 记录当前节点在父节点中的索引 */
	unsigned int idx_in_parent = 0;

	/* 左邻居 */
	btree_node * rnode = NULL;
	/* 右邻居 */
	btree_node * lnode = NULL;

	btree_mgr = node->btree_mgr;

	/* 此节点填充率必须小于40%,并且不是根节点,即parent不为空,根节点允许出现填充率低于40%的情况 */
	assert(node && node->btree_mgr);
	assert(((BTREE_NODE_PAYLOAD(btree_mgr) - node->nfree) < btree_mgr->pg_min_fill_sz) && node->parent);
	assert(node->read_buf == PageHeadMakeWritable(node->pg));
	assert(node->parent->read_buf == PageHeadMakeWritable(node->parent->pg));
	assert(!(node->parent->node_flag & e_btree_page_flag_leaf));

	/* 查找出左右邻居,至少有一个存在 */
	idx_in_parent = node->idx_in_parent_cellarray;
	if(node->parent->idx_changed)
		btree_mgr_check_idx_in_parent(node, &idx_in_parent);

	/* 根据b树的定义,子分支的数量比cell的个数要大1 */
	assert(idx_in_parent <= node->parent->ncell);

	if(idx_in_parent)
		btree_mgr_get_idx_lchild(node->parent, (idx_in_parent - 1), lsibling);

	if(idx_in_parent < node->parent->ncell - 1)
		btree_mgr_get_idx_lchild(node->parent, (idx_in_parent + 1), rsibling);
	else if(idx_in_parent == node->parent->ncell - 1)
		btree_mgr_get_rmost(node->parent, rsibling);

	assert((idx_in_parent == 0 && 0 == lsibling) ||
		(idx_in_parent == node->parent->ncell && 0 == rsibling) ||
		(rsibling && lsibling));

	if(lsibling)
	{
		/* 左邻居存在,合在一起,如果不超过一个节点的容量,则就成了一个节点,如果超过了,均成两块 */

		/*
		* 先将两节点合并,连同idx_in_parent,形成一个大的节点,然后均成两块
		* 如果合并后的节点超过了一个节点的大小,此时一定是可以均成填充率超过40%的两块,因为max local不超过10%,去10-%给parent,90+%可以均成两块
		* 如果合并后节点不超过一个节点的大小,结束返回
		* 注意,如果此前位于叶节点那一层,父节点下来的cell在做相应的修正
		*/

		lnode = btree_mgr_get_node(btree_mgr, lsibling, node->parent, idx_in_parent - 1);
		if(lnode)
		{
			if(0 != btree_mgr_together_or_average_tow_nodes(btree_mgr, lnode, node, idx_in_parent - 1, node->parent))
				ret = -1;
			else
				ret = 0;

			goto btree_mgr_fill_node_end_;
		}
	}
	
	if(rsibling)
	{
		/* 右邻居存在,合在一起,如果不超过一个节点的容量,则就成了一个节点,如果超过了,均成两块 */
		/* 这部分代码逻辑上lnode不为空时相同 */

		/*
		* 先将两节点合 ?连同idx_in_parent,形成一个大的节点,然后均成两块
		* 如果合并后的节点超过了一个节点的大.此时一定是可以均成填充率超过40%的两块,因为max local不超过10%, ?0-%给parent,90+%可以均成两块
		* 如果合并后节点不超过一个节点的大小,结束返回
		* 注意,如果此前位于叶节点那一层,父节点下来的cell在做相应的修正
		*/

		rnode = btree_mgr_get_node(btree_mgr, rsibling, node->parent, idx_in_parent + 1);
		if(NULL == rnode)
		{
			ret = -1;
			goto btree_mgr_fill_node_end_;
		}

		if(0 != btree_mgr_together_or_average_tow_nodes(btree_mgr, node, rnode, idx_in_parent, node->parent))
			ret = -1;
		else
			ret = 0;
	}

btree_mgr_fill_node_end_:

	if(lnode)
		btree_mgr_release_node(lnode);

	if(rnode)
		btree_mgr_release_node(rnode);
    
	return ret;
}

/**
 * @brief 平衡b树
 */
static __INLINE__ int btree_mgr_balance(btree_node * node)
{
	int need_continue = 0;
	btree_mgr_t * btree_mgr = NULL;
	btree_node * parent = NULL;

	assert(node && node->btree_mgr);

	btree_mgr = node->btree_mgr;

	while(node)
	{
		assert(PagerGetPageRefCount(node->pg));

		parent = node->parent;

		if(node->has_cell_ovfl)
		{
			assert(node->cell_ovfl.cell_buf);

			if(0 != btree_mgr_div_node(node))
				return -1;

			assert(!node->has_cell_ovfl && NULL == node->cell_ovfl.cell_buf);

			need_continue = 1;
		}
		else if(((BTREE_NODE_PAYLOAD(btree_mgr) - node->nfree) < btree_mgr->pg_min_fill_sz) && node->parent)
		{
			/* 如果根节点,允许出现填充率低到40%的情况 */
			if(0 != btree_mgr_fill_node(node))
				return -1;

			need_continue = 1;
		}

		assert((parent && PagerGetPageRefCount(parent->pg)) || NULL == parent);

		/* 如果当前节点既没有溢出,也没有出现填充率不足的情况,则平衡工作已经完成了 */
		if(!need_continue)
			break;

		/* 递归向上,直至当前节点没有溢出或者填充率低的情况出现 */
		node = parent;
		need_continue = 0;
	}

	return 0;
}

/**
 * @brief 添加键值
 */
static __INLINE__ int btree_mgr_add(btree_cursor * cursor,
									const void * key, const unsigned int key_sz,
									const void * data, const unsigned int data_sz)
{
	btree_cellinfo cell_info;
	unsigned char * cell = NULL;
	int ret = 0;
	int res = 0;

	assert(cursor && cursor->btree_mgr && cursor->root_pgno);

	assert((NULL == key && 0 == key_sz) || (key));
	assert((NULL == data && 0 == data_sz) || (data));
	assert(cursor->cur_node);

	/* 如果找到了重复键值,添加失败 */
	if(0 != btree_mgr_search(cursor, key, key_sz, &res))
		return -1;

	if(0 == res)
		return -1;

	/* 添加与删除总是发生的叶子节点处 */
	assert(cursor->cur_node->node_flag & e_btree_page_flag_leaf);

	/* 添加新的记录至游标所处的位置 */
	btree_mgr_fill_cellinfo(cursor->cur_node, key_sz, data_sz, &cell_info);
	assert(key_sz == cell_info.key_sz && data_sz == cell_info.data_sz);

	cell = MyMemPoolMalloc(cursor->btree_mgr->hm, cell_info.cell_sz);
	if(NULL == cell)
		return -1;

	/* 打包cell */
	if(0 != btree_mgr_pack_cell(cursor->cur_node, &cell_info,
		cell, cell_info.cell_sz,
		key, key_sz,
		data, data_sz))
	{
		ret = -1;
		goto btree_mgr_add_end_;
	}

	/* 将打包后的cell添加到指定的位置 */
	if(0 != btree_mgr_insert_cell(cursor->cur_node, cursor->idx, cell, cell_info.cell_sz))
	{
		ret = -1;
		goto btree_mgr_add_end_;
	}

	/* 调用balance平衡b树 */
	if(0 != btree_mgr_balance(cursor->cur_node))
	{
		ret = -1;
		goto btree_mgr_add_end_;
	}

btree_mgr_add_end_:

	assert(cell);
	MyMemPoolFree(cursor->btree_mgr->hm, cell);

	/* 将游标移动至根节点 */
	btree_mgr_move_to_root(cursor);

	return ret;
}

/**
 * @brief 添加键值
 */
int btreeMgrAdd(HBTREE_CURSOR hcur,
				const void * key, const unsigned int key_sz,
				const void * data, const unsigned int data_sz)
{
	if(NULL == hcur || NULL == hcur->btree_mgr || 0 == hcur->root_pgno)
		return -1;

	return btree_mgr_add(hcur, key, key_sz, data, data_sz);
}

/**
 * @brief 复制游标
 */
static __INLINE__ btree_cursor * btree_mgr_duplicate_cursor(btree_cursor * src_cur)
{
	btree_cursor * cursor = NULL;

	assert(src_cur && src_cur->btree_mgr);
	assert(src_cur->btree_mgr->hpgr);
	assert(src_cur->root_pgno);

	cursor = btree_mgr_alloc_cursor(src_cur->btree_mgr, src_cur->key_cmp, src_cur->context, src_cur->context_sz);
	if(NULL == cursor)
		return NULL;

	cursor->root_pgno = src_cur->root_pgno;
	cursor->cur_node = btree_mgr_get_node(cursor->btree_mgr,
		src_cur->cur_node->pg_no,
		src_cur->cur_node->parent,
		src_cur->cur_node->idx_in_parent_cellarray);

	if(NULL == cursor->cur_node)
		goto btree_mgr_copy_cursor_end_;

	cursor->idx = src_cur->idx;

	assert(cursor->context == src_cur->context);
	assert(cursor->context_sz == src_cur->context_sz);
	assert(cursor->key_cmp == src_cur->key_cmp);
	assert(cursor->root_pgno == src_cur->root_pgno);
	assert(cursor->cur_node == src_cur->cur_node);
	assert(cursor->cur_node->pg_no == src_cur->cur_node->pg_no);
	assert(cursor->idx == src_cur->idx);

	return cursor;

btree_mgr_copy_cursor_end_:

	btree_mgr_release_cursor(cursor);
	return NULL;
}

/**
 * @brief 寻找右孩子
 */
#define btree_mgr_get_idx_rchild(_n, _i, _rchild) do{\
		assert(_n && _i < _n->ncell);\
		if(_i + 1 < _n->ncell) \
			btree_mgr_get_idx_lchild(_n, _i + 1, _rchild);\
		else \
			btree_mgr_get_rmost(_n, _rchild);\
	}while(0)

/**
 * @brief 则寻当前游标的下一个cell
 */
static __INLINE__ int btree_mgr_next(btree_cursor * cursor)
{
	unsigned int sub_pgno = 0;

	assert(cursor && cursor->btree_mgr);
	assert(cursor->btree_mgr->hpgr);
	assert(cursor->cur_node && cursor->idx < cursor->cur_node->ncell);

	cursor->idx += 1;
	if(cursor->cur_node->node_flag & e_btree_page_flag_leaf)
	{
		if(cursor->idx < cursor->cur_node->ncell)
			return 0;

		while(cursor->cur_node->parent)
		{
			btree_node * temp = cursor->cur_node;
			cursor->idx = cursor->cur_node->idx_in_parent_cellarray;
			cursor->cur_node = cursor->cur_node->parent;
			btree_mgr_release_node(temp);

			if(cursor->idx < cursor->cur_node->ncell)
				break;
		}

		if(NULL == cursor->cur_node->parent)
		{
			assert(cursor->cur_node->pg_no == cursor->root_pgno);
			if(cursor->idx >= cursor->cur_node->ncell)
				return -1;
		}

		return 0;
	}

	while(!(cursor->cur_node->node_flag & e_btree_page_flag_leaf))
	{
		btree_node * sub_node = NULL;

		if(cursor->idx < cursor->cur_node->ncell)
			btree_mgr_get_idx_lchild(cursor->cur_node, cursor->idx, sub_pgno);
		else
			btree_mgr_get_rmost(cursor->cur_node, sub_pgno);

		sub_node = btree_mgr_get_node(cursor->btree_mgr,
			sub_pgno,
			cursor->cur_node,
			cursor->idx);

		if(NULL == sub_node)
			return -1;

		btree_mgr_release_node(cursor->cur_node);
		cursor->cur_node = sub_node;
		cursor->idx = 0;
	}

	return 0;
}

/**
 * @brief 删除键值
 */
static __INLINE__ int btree_mgr_del(btree_cursor * cursor,
									const void * key,
									const unsigned int key_sz)
{
	/*
	* 先把指定位置的东西删除了,再把替补cell添加进去
	* 就当前这个节点做一次平衡
	* 把替补cell,一定是位于叶子节点,删除,再做一次平衡操作
	*/
	int ret = -1;
	btree_cursor * temp_cursor = NULL;
	unsigned char * cell = NULL;
	btree_cellinfo cell_info;
	unsigned int lchild = 0;
	int res = 0;

	assert(cursor && cursor->btree_mgr && cursor->root_pgno);

	assert(cursor->btree_mgr == cursor->cur_node->btree_mgr);

	/* 查找键值是否存在,不存在删除失败 */
	if(0 != btree_mgr_search(cursor, key, key_sz, &res))
		return -1;

	if(res)
	{
		ret = -1;
		goto btree_mgr_del_end_;
	}

	/* 如果当前节点是叶节点,直进行删除工作 */
	if(cursor->cur_node->node_flag & e_btree_page_flag_leaf)
	{
		if(0 != btree_mgr_delete_record(cursor->cur_node, cursor->idx, NULL))
		{
			ret = -1;
			goto btree_mgr_del_end_;
		}

		if(0 != btree_mgr_balance(cursor->cur_node))
		{
			ret = -1;
			goto btree_mgr_del_end_;
		}

		ret = 0;
		goto btree_mgr_del_end_;
	}

	/*
	* 如果不是叶节点,则需要进行转化
	* 游标走到"下一个"cell的位置
	*/
	temp_cursor = btree_mgr_duplicate_cursor(cursor);
	if(NULL == temp_cursor)
		return -1;

	if(0 != btree_mgr_next(temp_cursor))
	{
		ret = -1;
		goto btree_mgr_del_end_;
	}

	/* 内部节点的下一个(以下简称next cell)必然位于叶节点处,且其位置索引必为0 */
	assert(temp_cursor->cur_node->node_flag & e_btree_page_flag_leaf);
	assert(0 == temp_cursor->idx);

	/* 将当前位置的值去除,并用next cell里的内容替代 */
	btree_mgr_parse_idx(temp_cursor->cur_node, temp_cursor->idx, &cell_info);
	cell = MyMemPoolMalloc(temp_cursor->btree_mgr->hm, cell_info.cell_sz + sizeof(unsigned int));
	if(NULL == cell)
	{
		ret = -1;
		goto btree_mgr_del_end_;
	}

	assert(!(cursor->cur_node->node_flag & e_btree_page_flag_leaf));
	btree_mgr_get_idx_lchild(cursor->cur_node, cursor->idx, lchild);
	uint_to_big_endian(lchild, cell, sizeof(unsigned int));
	memcpy(cell + sizeof(unsigned int), cell_info.cell, cell_info.cell_sz);

	if(0 != btree_mgr_delete_record(cursor->cur_node, cursor->idx, NULL))
	{
		ret = -1;
		goto btree_mgr_del_end_;
	}

	if(0 != btree_mgr_insert_cell(cursor->cur_node, cursor->idx, cell, cell_info.cell_sz + sizeof(unsigned int)))
	{
		ret = -1;
		goto btree_mgr_del_end_;
	}

	if(0 != btree_mgr_balance(cursor->cur_node))
	{
		ret = -1;
		goto btree_mgr_del_end_;
	}

	/* 将next cell删除,并做平衡操作 */
	if(0 != btree_mgr_drop_cell(temp_cursor->cur_node, temp_cursor->idx, &cell_info))
	{
		ret = -1;
		goto btree_mgr_del_end_;
	}

	if(0 != btree_mgr_balance(temp_cursor->cur_node))
	{
		ret = -1;
		goto btree_mgr_del_end_;
	}

	ret = 0;

btree_mgr_del_end_:

	if(cell)
		MyMemPoolFree(cursor->btree_mgr->hm, cell);

	if(temp_cursor)
		btree_mgr_release_cursor(temp_cursor);

	btree_mgr_move_to_root(cursor);

	return ret;
}

/**
 * @brief 删除键值
 */
int btreeMgrDel(HBTREE_CURSOR hcur,
				const void * key,
				const unsigned int key_sz)
{	
	if(NULL == hcur || NULL == hcur->btree_mgr || 0 == hcur->root_pgno)
		return -1;

	return btree_mgr_del(hcur, key, key_sz);
}

/**
 * @brief 查找键值
 */
int btreeMgrSearch(HBTREE_CURSOR hcur,
				   const void * key,
				   const unsigned int key_sz)
{
	int res = 0;
	if(NULL == hcur)
		return -1;

	if(0 != btree_mgr_search(hcur, key, key_sz, &res))
		return -1;

	if(res)
		return -1;

	return 0;
}

/**
 * @brief 获取游标当前所在记录的key
 */
int btreeMgrGetKey(HBTREE_CURSOR hcur, void ** pkey, unsigned int * pkey_sz, HMYMEMPOOL hm)
{
	btree_cellinfo cell_info;

	if(NULL == hcur || NULL == hcur->btree_mgr || NULL == hcur->cur_node || NULL == pkey || NULL == pkey_sz)
		return -1;

	btree_mgr_parse_idx(hcur->cur_node, hcur->idx, &cell_info);

	if(hcur->cur_node->node_flag & e_btree_page_flag_intkey)
	{
		*((unsigned *)pkey) = cell_info.key_sz;
		*pkey_sz = 0;

		return 0;
	}
	else
	{
		*pkey = MyMemPoolMalloc(hm, cell_info.key_sz);
		*pkey_sz = cell_info.key_sz;
		return btree_mgr_cell_get_key(hcur->cur_node, &cell_info, *pkey, cell_info.key_sz);
	}
}

/**
 * @brief 获取游标当前所在记录的data
 */
int btreeMgrGetData(HBTREE_CURSOR hcur, void ** pdata, unsigned int * pdata_sz, HMYMEMPOOL hm)
{
	btree_cellinfo cell_info;

	if(NULL == hcur || NULL == hcur->btree_mgr || NULL == hcur->cur_node || NULL == pdata || NULL == pdata_sz)
		return -1;

	if(!(hcur->cur_node->node_flag & e_btree_page_flag_hasdata))
		return -1;

	btree_mgr_parse_idx(hcur->cur_node, hcur->idx, &cell_info);

	*pdata = MyMemPoolMalloc(hm, cell_info.data_sz);
	*pdata_sz = cell_info.data_sz;
	return btree_mgr_cell_get_data(hcur->cur_node, &cell_info, *pdata, cell_info.data_sz);
}

/**
 * @brief 递归删除node以及它的所有子节点
 * @param bdel_self:是否把自己也删除
 */
static int btree_mgr_clear_node_and_sub(btree_node * node, int bdel_self)
{
	unsigned int i;
	unsigned int right_most = 0;
	btree_node * sub_node = NULL;
	btree_cellinfo cell_info;

	assert(node);

	if(!(node->node_flag & e_btree_page_flag_leaf))
	{
		/* 如果不是叶节点,则对每个子分支进行递归 */
		for(i = 0; i < node->ncell; i ++)
		{
			btree_mgr_parse_idx(node, i, &cell_info);

			sub_node = btree_mgr_get_node(node->btree_mgr, cell_info.lchild_pgno, node, i);
			if(NULL == sub_node)
				return -1;

			if(0 != btree_mgr_clear_node_and_sub(sub_node, 1))
			{
				btree_mgr_release_node(sub_node);
				return -1;
			}
			else
				btree_mgr_release_node(sub_node);
		}

		btree_mgr_get_rmost(node, right_most);
		assert(right_most);

		sub_node = btree_mgr_get_node(node->btree_mgr, right_most, node, node->ncell);
		if(NULL == sub_node)
			return -1;

		if(0 != btree_mgr_clear_node_and_sub(sub_node, 1))
		{
			btree_mgr_release_node(sub_node);
			return -1;
		}
		else
			btree_mgr_release_node(sub_node);
	}

	/* 删除所有的溢出页,再删除本节点即可 */
	for(i = 0; i < node->ncell; i ++)
	{
		if(0 != btree_mgr_delete_record(node, i, NULL))
			return -1;
	}

	if(bdel_self)
	{
		btree_mgr_ref_node(node);
		if(0 != btree_mgr_delete_node(node))
			return -1;
	}

	return 0;
}

/**
 * @brief 将游标所指的表里的所有记录全部删除
 */
static __INLINE__ int btree_mgr_clear(btree_cursor * cursor)
{
	if(0 != btree_mgr_move_to_root(cursor))
		return -1;

	return btree_mgr_clear_node_and_sub(cursor->cur_node, 0);
}

/**
 * @brief 将游标所指的表里的所有记录全部删除
 */
int btreeMgrClear(HBTREE_CURSOR hcur)
{
	if(NULL == hcur || NULL == hcur->btree_mgr)
		return -1;

	return btree_mgr_clear(hcur);
}

/**
 * @brief 获取主树的游标,主树记录所有b树管理器的其它b树的根节点
 */
int btreeMgrOpenMaster(HBTREE_MGR hbtreeMgr,
					   HBTREE_CURSOR * phcur,
					   XCOMPARE cmp,
					   const void * context,unsigned int context_sz)
{
	if(NULL == hbtreeMgr || NULL == phcur)
		return -1;

	if(NULL != *phcur)
	{
		LOG_WARN(("*phcur must be null"));
		return -1;
	}

	*phcur = btree_mgr_alloc_cursor(hbtreeMgr, cmp, context, context_sz);
	if(NULL == *phcur)
		return -1;

	(*phcur)->root_pgno = BTREE_MASTER_ROOT;
	if(PagerGetTotalPagesCount(hbtreeMgr->hpgr) >= BTREE_MASTER_ROOT)
		(*phcur)->cur_node = btree_mgr_get_node(hbtreeMgr, (*phcur)->root_pgno, NULL, 0);
	else
	{
		(*phcur)->cur_node = btree_mgr_new_node(hbtreeMgr, NULL, 0, e_btree_page_flag_leaf | e_btree_page_flag_hasdata);
		assert((*phcur)->cur_node->pg_no == (*phcur)->root_pgno);
	}

	if(NULL == (*phcur)->cur_node)
		return -1;

	return 0;
}

/**
 * @brief 创建一个表
 * @param hcur:不可为空,创建成功将给hcur的root_pg赋值
 */
int btreeMgrCreateTable(HBTREE_CURSOR hcur_master,
						HBTREE_CURSOR * phcur,
						XCOMPARE cmp, const void * context, unsigned int context_sz,
						const void * table_id, const unsigned int table_id_sz,
						const void * table_info, const unsigned int table_info_sz,
						unsigned char flag)
{
	int res = 0;
	unsigned char * tbl_real_info = NULL;
	unsigned int pgno_tlb_root = 0;

	if(NULL == phcur || NULL == hcur_master || NULL == hcur_master->btree_mgr || NULL == table_id || 0 == table_id_sz)
		return -1;

	/* 首先在master(即主索引树)表里查找 */
	if(0 != btree_mgr_move_to_root(hcur_master))
		return -1;

	/* 如果找到,则创建失败 */
	if(0 != btree_mgr_search(hcur_master, table_id, table_id_sz, &res))
		return -1;

	if(0 == res)
		return -1;

	/* 找不到,将用户的建表信息存入主索引树,并分配root_pgno */
	pgno_tlb_root = PagerGetPageNo(hcur_master->btree_mgr->hpgr);
	if(0 == pgno_tlb_root)
		return -1;

	*phcur = btree_mgr_alloc_cursor(hcur_master->btree_mgr, cmp, context, context_sz);
	if(NULL == *phcur)
		goto btreeMgrCreateTable_err_;

	/* 将root_pgno赋给hcur,并加载根节点页缓存 */
	(*phcur)->root_pgno = pgno_tlb_root;
	(*phcur)->cur_node = btree_mgr_get_node(hcur_master->btree_mgr, pgno_tlb_root, NULL, 0);
	assert(pgno_tlb_root == (*phcur)->cur_node->pg_no);
	if(NULL == (*phcur)->cur_node)
		goto btreeMgrCreateTable_err_;

	if(0 != btree_mgr_set_nodeflag((*phcur)->cur_node, (unsigned char)(flag | e_btree_page_flag_leaf)))
		goto btreeMgrCreateTable_err_;

	/* 将当前表信息,表的根节点页号添加到主索引树当中 */
	tbl_real_info = MyMemPoolMalloc(hcur_master->btree_mgr->hm, table_info_sz + sizeof(unsigned int));
	if(NULL == tbl_real_info)
		goto btreeMgrCreateTable_err_;
	uint_to_big_endian(pgno_tlb_root, tbl_real_info, sizeof(unsigned int));
	memcpy(tbl_real_info + sizeof(unsigned int), table_info, table_info_sz);
	if(0 != btree_mgr_add(hcur_master, table_id, table_id_sz, tbl_real_info, table_info_sz + sizeof(unsigned int)))
		goto btreeMgrCreateTable_err_;

	assert(tbl_real_info);
	MyMemPoolFree(hcur_master->btree_mgr->hm, tbl_real_info);

	return 0;

btreeMgrCreateTable_err_:

	if(*phcur)
		btree_mgr_release_cursor(*phcur);

	if(pgno_tlb_root)
		PagerReleasePageNo(hcur_master->btree_mgr->hpgr, pgno_tlb_root);

	if(tbl_real_info)
		MyMemPoolFree(hcur_master->btree_mgr->hm, tbl_real_info);

	return -1;
}

/**
 * @brief 获取指定表的根节点
 */
static __INLINE__ btree_node * btree_mgr_get_tbl_root_node(btree_cursor * cur_master,
														   const void * table_id, const unsigned int table_id_sz,
														   HMYBUFFER hb_tbl_info)
{
	int res = 0;
	unsigned int pgno_tbl_root = 0;
	btree_node * node = NULL;
	unsigned char * data = NULL;
	btree_cellinfo cell_info;

	assert(cur_master && cur_master->btree_mgr && table_id && table_id_sz);

	if(0 != btree_mgr_search(cur_master, table_id, table_id_sz, &res))
		return NULL;

	if(0 != res)
		return NULL;

	/* 取出hcur_master里,当前表的相关信息 */
	btree_mgr_parse_idx(cur_master->cur_node, cur_master->idx, &cell_info);
	assert(cell_info.data_sz >= sizeof(unsigned int));

	data = MyMemPoolMalloc(cur_master->btree_mgr->hm, cell_info.data_sz);
	if(0 != btree_mgr_cell_get_data(cur_master->cur_node, &cell_info, data, cell_info.data_sz))
		goto btree_mgr_get_tbl_root_node_err_;

	array_to_uint_as_big_endian(data, sizeof(unsigned int), pgno_tbl_root);
	assert(pgno_tbl_root);

	node = btree_mgr_get_node(cur_master->btree_mgr, pgno_tbl_root, NULL, 0);
	if(NULL == node)
		goto btree_mgr_get_tbl_root_node_err_;

	if(hb_tbl_info)
		MyBufferSet(hb_tbl_info, data + sizeof(unsigned int), cell_info.data_sz + sizeof(unsigned int));

	assert(data);
	MyMemPoolFree(cur_master->btree_mgr->hm, data);

	return node;

btree_mgr_get_tbl_root_node_err_:

	if(data)
		MyMemPoolFree(cur_master->btree_mgr->hm, data);

	if(node)
		btree_mgr_release_node(node);

	return NULL;
}

/**
 * @brief 打开一个表
 */
static __INLINE__ int btree_mgr_open_tbl(btree_cursor * cur_master,
										 btree_cursor ** pcur,
										 XCOMPARE cmp, const void * context, unsigned int context_sz,
										 const void * table_id, const unsigned int table_id_sz,
										 HMYBUFFER hb_tbl_info)
{
	assert(pcur && cur_master && cur_master->btree_mgr && table_id && table_id_sz && cmp);

	*pcur = btree_mgr_alloc_cursor(cur_master->btree_mgr, cmp, context, context_sz);
	if(NULL == *pcur)
		return -1;

	/* 取出hcur_master里,当前表的相关信息 */
	(*pcur)->cur_node = btree_mgr_get_tbl_root_node(cur_master, table_id, table_id_sz, hb_tbl_info);
	if(NULL == (*pcur)->cur_node)
		goto btree_mgr_open_tbl_err_;

	(*pcur)->root_pgno = (*pcur)->cur_node->pg_no;

	return 0;

btree_mgr_open_tbl_err_:

	if(*pcur)
		btree_mgr_release_cursor(*pcur);

	*pcur = NULL;

	return -1;
}

/**
 * @brief 打开一个表
 * @param hcur:不可为空,打开成功将给hcur的root_pg赋值
 */
int btreeMgrOpenTable(HBTREE_CURSOR hcur_master,
					  HBTREE_CURSOR * phcur,
					  XCOMPARE cmp, const void * context, unsigned int context_sz,
					  const void * table_id, const unsigned int table_id_sz,
					  HMYBUFFER hb_tbl_info)
{
	if(NULL == phcur || NULL == hcur_master || NULL == hcur_master->btree_mgr || NULL == table_id || 0 == table_id_sz || NULL == cmp)
		return -1;

	return btree_mgr_open_tbl(hcur_master, phcur, cmp, context, context_sz, table_id, table_id_sz, hb_tbl_info);
}

/**
 * @brief 删除一个表/索引
 */
int btreeMgrDropTable(HBTREE_CURSOR hcur_master,
					  const void * table_id, const unsigned int table_id_sz)
{
	int ret = 0;
	btree_node * node = NULL;

	if(NULL == hcur_master || NULL == hcur_master->btree_mgr)
		return -1;

	node = btree_mgr_get_tbl_root_node(hcur_master, table_id, table_id_sz, NULL);
	if(NULL == node)
		return -1;

	if(0 != btree_mgr_clear_node_and_sub(node, 1))
	{
		ret = -1;
		goto btreeMgrDropTable_end_;
	}

	if(0 != btree_mgr_del(hcur_master, table_id, table_id_sz))
	{
		ret = -1;
		goto btreeMgrDropTable_end_;
	}

btreeMgrDropTable_end_:

	if(node)
		btree_mgr_release_node(node);

	return ret;
}

/**
 * @brief 将游标移动到最右边的那个记录
 */
static __INLINE__ int btree_mgr_move_to_right_most(btree_cursor * cursor)
{
	btree_node * sub = NULL;
	unsigned int pgno_sub = 0;

	assert(cursor && cursor->btree_mgr);

	/* 将cur移动到最右边 */
	if(0 != btree_mgr_move_to_root(cursor))
		return -1;

	while(!(cursor->cur_node->node_flag & e_btree_page_flag_leaf))
	{
		btree_mgr_get_rmost(cursor->cur_node, pgno_sub);

		assert(pgno_sub);

		sub = btree_mgr_get_node(cursor->btree_mgr, pgno_sub, cursor->cur_node, cursor->cur_node->ncell);
		if(NULL == sub)
			return -1;

		btree_mgr_release_node(cursor->cur_node);
		cursor->cur_node = sub;
	}

	cursor->idx = cursor->cur_node->ncell - 1;

	return 0;
}

/**
 * @brief 获取一个表的新rowid
 */
extern int btreeMgrTableGetRowid(HBTREE_CURSOR hcur,
								 unsigned int * prowid)
{
	int res = 0;
	int loop = 1000;
	btree_cellinfo cell_info;

	if(NULL == hcur || NULL == hcur->btree_mgr || NULL == prowid)
		return -1;

	btree_mgr_move_to_right_most(hcur);

	assert(hcur->cur_node->node_flag & e_btree_page_flag_intkey);

	btree_mgr_parse_idx(hcur->cur_node, hcur->idx, &cell_info);

	/* 如果当前最大的rowid没有达到 0xffffffff */
	if(cell_info.key_sz < ((unsigned int)-1))
		*prowid = cell_info.key_sz + 1;

	/* 如果达到了 */

	while(loop)
	{
		/* 暂时先用这个随机数算法 */
		*prowid = rand();

		if(0 != btree_mgr_search(hcur, NULL, *prowid, &res))
			return -1;

		/* 如果当前生成的随机rowid没有重复,返回 */
		if(0 != res)
			return 0;

		loop--;
	}

	/* 重试1000次后,依然是重复的,则认为失败 */
	return -1;
}

/**
 * @brief 提交更改
 */
int btreeMgrCommit(HBTREE_MGR hbtreeMgr)
{
	if(NULL == hbtreeMgr || NULL == hbtreeMgr->hpgr)
		return -1;

	return PagerSyn(hbtreeMgr->hpgr);
}

/**
 * @brief 回滚
 */
int btreeMgrRollBack(HBTREE_MGR hbtreeMgr)
{
	if(NULL == hbtreeMgr || NULL == hbtreeMgr->hpgr)
		return -1;

	return PagerRollBack(hbtreeMgr->hpgr);
}

/**
 * @brief 获取一棵b树的记录总数
 */
unsigned int btreeMgrGetCount(HBTREE_CURSOR hcur)
{
	if(NULL == hcur || NULL == hcur->btree_mgr || NULL == hcur->btree_mgr->hpgr)
		return 0;

	if(0 != btree_mgr_move_to_root(hcur))
		return 0;

	return btree_mgr_get_count(hcur, hcur->cur_node);
}

/**
 * @brief 获取外存页文件有多少页,以及有多少空闲页
 */
int btreeMgrGetPagerInfo(HBTREE_MGR hbtreeMgr, pager_info_t * pager_info)
{
	if(NULL == hbtreeMgr || NULL == pager_info || NULL == hbtreeMgr->hpgr)
		return -1;

	pager_info->total_page_count = PagerGetTotalPagesCount(hbtreeMgr->hpgr);
	pager_info->free_page_count = PagerGetFreePages(hbtreeMgr->hpgr);

	return 0;
}

/**
 * @brief 获取页缓存引用计数
 */
int btreeMgrGetPagerRefCount(HBTREE_MGR hbtreeMgr)
{
	if(NULL == hbtreeMgr || NULL == hbtreeMgr->hpgr)
		return 0;

	return PagerGetRefCount(hbtreeMgr->hpgr);
}


/**
 * @brief 检查一个node是否已排好序
 */
static __INLINE__ int examin_sort_ok(btree_cursor * cursor, btree_node * node)
{
	unsigned int _j;
	assert(node->ncell || NULL == node->parent);
	if(1 >= node->ncell)
		return 0;
	for(_j = 0; _j < node->ncell - 1; _j ++) 
	{ 
		unsigned char * key1 = NULL; 
		unsigned char * key2 = NULL; 

		btree_cellinfo cell_info1; 
		btree_cellinfo cell_info2; 

		assert((_j + 1) < node->ncell); 

		btree_mgr_parse_idx(node, _j,		&cell_info1); 
		btree_mgr_parse_idx(node, _j + 1,	&cell_info2); 

		key1 = MyMemPoolMalloc(NULL, cell_info1.key_sz); 
		key2 = MyMemPoolMalloc(NULL, cell_info2.key_sz);

		btree_mgr_cell_get_key(node, &cell_info1, key1, cell_info1.key_sz); 
		btree_mgr_cell_get_key(node, &cell_info2, key2, cell_info2.key_sz); 

		{
			int ret = cursor->key_cmp(key1, cell_info1.key_sz, 
			key2, cell_info2.key_sz, 
			cursor->context, cursor->context_sz);
			assert(ret < 0); 
		}

		MyMemPoolFree(NULL, key1);
		MyMemPoolFree(NULL, key2);
	} 

	return 0;
}

/**
 * @brief 检查一个cell的ovfl链(如果有的话)是否合法
 */
static int examin_cell(btree_mgr_t * btree_mgr, btree_cellinfo * cellinfo, unsigned int * node_count)
{
	unsigned int pgno_ovfl = 0;

	assert(cellinfo);

	pgno_ovfl = cellinfo->overflow_pgno;

	while(pgno_ovfl)
	{
		const unsigned char * rb = NULL;
		HPAGE_HEAD pg = NULL;

		if(node_count)
			*node_count += 1;

		pg = PagerGetPage(btree_mgr->hpgr, pgno_ovfl);
		assert(pg);

		rb = PageHeadMakeReadable(pg);
		assert(rb);

		array_to_uint_as_big_endian(rb, sizeof(unsigned int), pgno_ovfl);

		PagerReleasePage(pg);
	}

	return 0;
}

/**
 * @brief 检查节点里的内容是否合法
 */
static int examin_node(btree_cursor * cursor, btree_node * node, unsigned int * node_count)
{
	int flag_content = 0;
	unsigned int i;
	unsigned int total_cell_sz = 0;
	unsigned int cell_content;
	unsigned int min_cell = -1;

	assert(node);
	get_2byte(&(node->read_buf[BTREE_HDR_FIRST_CONTENT]), BTREE_CELL_PTR_SIZE, cell_content);
	assert(cell_content <= node->btree_mgr->pg_sz);

	{
		unsigned int cell_ptr_end = BTREE_CELL_PTR_OFFSET + BTREE_CELL_PTR_SIZE * node->ncell;
		assert(cell_content >= cell_ptr_end);
	}

	assert(!node->has_cell_ovfl && NULL == node->cell_ovfl.cell_buf);

	if(0 == node->ncell)
	{
		assert(NULL == node->parent);

		if(cursor)
			assert(cursor->root_pgno == node->pg_no);
	}

	assert(node->node_flag & e_btree_page_flag_hasdata);

	assert(node->read_buf[BTREE_HDR_NODEFLAG_OFFSET] == node->node_flag);

	/* 查看每个cell */
	for(i = 0; i < node->ncell; i ++)
	{
		unsigned int cellptr;
		btree_cellinfo cellinfo;
		btree_mgr_parse_idx(node, i, &cellinfo);
		btree_mgr_find_cellptr(node, i, cellptr);

		assert(cellptr >= cell_content);

		examin_cell(node->btree_mgr, &cellinfo, node_count);

		if(min_cell > cellptr)
			min_cell = cellptr;

		if(cellptr == cell_content)
			flag_content = 1;

		assert((unsigned int)((4 * ((node->node_flag & e_btree_page_flag_leaf) ? 0 : 1) + 
			4 * ((node->node_flag & e_btree_page_flag_hasdata) ? 1 : 0) + 4 * (cellinfo.overflow_pgno?1:0) + 4)) == cellinfo.head_sz);

		{
			unsigned int local_sz = 0;
			unsigned int payload_sz = 
				cellinfo.key_sz * ((node->node_flag & e_btree_page_flag_intkey) ? 0 : 1) + cellinfo.data_sz;

			if(payload_sz > node->btree_mgr->max_local)
			{
				if(payload_sz % BTREE_OVERFLOW_PAYLOAD(node->btree_mgr) <= node->btree_mgr->max_local)
					local_sz = payload_sz % BTREE_OVERFLOW_PAYLOAD(node->btree_mgr);
				else
					local_sz = node->btree_mgr->min_local;
			}
			else
				local_sz = payload_sz;

			assert(local_sz == cellinfo.local_sz);
			assert(local_sz + cellinfo.head_sz == cellinfo.cell_sz);
		}

		//if(node->node_flag & e_btree_page_flag_leaf)
		//	assert(cellinfo.cell_sz >= 72);
		//else
		//	assert(cellinfo.cell_sz == 76);

		assert(PagePtrIsInpage(node->pg, cellinfo.cell));
		assert(PagePtrIsInpage(node->pg, cellinfo.cell + cellinfo.cell_sz));

		{
			unsigned int key_sz = 0;

			unsigned char * key = MyMemPoolMalloc(node->btree_mgr->hm, cellinfo.key_sz);
			btree_mgr_cell_get_key(node, &cellinfo, key, cellinfo.key_sz);

			memcpy(&key_sz, key, 4);

			assert(cellinfo.key_sz == key_sz);
			assert(cellinfo.data_sz == key_sz);

			if(cellinfo.overflow_pgno)
			{
				assert(cellinfo.local_sz <= node->btree_mgr->max_local);

				cellinfo.payload_sz = cellinfo.key_sz + cellinfo.data_sz;
			}
			else
			{
				assert(cellinfo.local_sz == cellinfo.key_sz + cellinfo.data_sz);
			}

			/* 请在测试程序的前四个字节填入abcd,呵呵 */
			assert(strncmp(key + 4, "abcd", 4) == 0);

			if(cellinfo.local_sz > 4)
			{
				assert(strncmp(cellinfo.cell + cellinfo.head_sz + 4, "abcd", (cellinfo.local_sz - 4) > 4 ? 4 : (cellinfo.local_sz - 4)) == 0);
			}

			MyMemPoolFree(node->btree_mgr->hm, key);
		}

		total_cell_sz += cellinfo.cell_sz;
	}

	if(0 == flag_content)
	{
		if(node->ncell)
		{
			/* 第一个空闲块与cell_content起始,应不超过frag的最大值 */
			unsigned int free_buf = 0;
			get_2byte(&(node->read_buf[BTREE_HDR_FIRST_FREE_OFFSET]), BTREE_CELL_PTR_SIZE, free_buf);
			if((min_cell > free_buf) && free_buf)
			{
				assert(free_buf - cell_content <= BTREE_MAX_FRAG);
				assert(free_buf - cell_content <= 4);
			}
			else
			{
				assert(min_cell - cell_content <= BTREE_MAX_FRAG);
				assert(min_cell - cell_content <= 4);
			}
			assert(node->read_buf[BTREE_HDR_NFRAG_OFFSET]);
		}
		else
		{
			assert(NULL == node->parent);
		}
	}
	else
		assert(min_cell == cell_content);

	/* 查看空闲块 */
	if(0 == cell_content)
	{
		assert(0 == node->ncell);
	}
	else
	{
		unsigned int free_buf = 0;
		unsigned int nfree = 0;

		nfree = cell_content - (2 * node->ncell + BTREE_CELL_PTR_OFFSET) +
			node->read_buf[BTREE_HDR_NFRAG_OFFSET];

		get_2byte(&(node->read_buf[BTREE_HDR_FIRST_FREE_OFFSET]), BTREE_CELL_PTR_SIZE, free_buf);

		/* 顺着空闲链表做统计 */
		while(free_buf)
		{
			unsigned int sz = 0;
			unsigned int next_free = 0;

			get_2byte(&(node->read_buf[free_buf + BTREE_FREE_BLOCK_SZ_OFFSET]), sizeof(unsigned short), sz);
			nfree += sz;

			get_2byte(&(node->read_buf[free_buf + BTREE_FREE_BLOCK_NEXT_OFFSET]), BTREE_CELL_PTR_SIZE, next_free);

			/* 空闲块按偏移的升序排列,并且不可能连续(释放时要处理合并连续空闲块的情况) */
			assert(next_free > free_buf + BTREE_FREE_BLOCK_MIN_SZ || 0 == next_free);

			free_buf = next_free;
		}

		/* 总大小应该合法 */
		assert((nfree + total_cell_sz + BTREE_CELL_PTR_SIZE * node->ncell + BTREE_CELL_PTR_OFFSET) == node->btree_mgr->pg_sz);

		/* 内存缓存的空闲空间应与计算出来的相等 */
		assert(node->nfree == nfree);
	}

	{
		unsigned int ncell = 0;

		assert(node->read_buf == PageHeadMakeReadable(node->pg));

		get_2byte(&(node->read_buf[BTREE_HDR_NCELL_OFFSET]), BTREE_HDR_NCELL_SZ, ncell);

		assert(ncell == node->ncell);
	}

	return 0;
}

/**
 * @brief 检查一棵b树是否合法
 */
static int examin_btree(btree_cursor * cursor, unsigned int * node_count,
						btree_node * node, btree_node * next_thread_node,
						unsigned int max_idx, unsigned int min_idx,
						int layer)
{
	int leaf = 0;
	unsigned int i;
	int cur_layer = layer + 1;
	int sub_layer = 0;

	assert(node);
	assert(cursor->key_cmp);

	/* 当前节点的一些属性校验 */
	examin_node(cursor, node, node_count);

	/* cell数组有序 */
	examin_sort_ok(cursor, node);

	/* 填充率必须高于40%,不能有cell_ovfl */
	assert(!node->has_cell_ovfl && NULL == node->cell_ovfl.cell_buf);

	if(node->parent)
		assert((BTREE_NODE_PAYLOAD(node->btree_mgr) - node->nfree) > node->btree_mgr->pg_min_fill_sz);
	else
		assert(node->pg_no == cursor->root_pgno);

	if(node_count)
		*node_count += 1;

	for(i = 0; i <= node->ncell; i ++)
	{
		int cur_sub_layer = 0;
		unsigned int sub_pgno = 0;
		btree_node * sub_node = NULL;

		if(i < node->ncell)
		{
			btree_cellinfo cell_info;
			btree_mgr_parse_idx(node, i, &cell_info);
			sub_pgno = cell_info.lchild_pgno;
		}
		else
		{
			if(!(node->node_flag & e_btree_page_flag_leaf))
				btree_mgr_get_rmost(node, sub_pgno);
			else
                array_to_uint_as_big_endian(&node->read_buf[BTREE_HDR_MOST_RIGHT_CHILD_OFFSET], sizeof(unsigned int), sub_pgno);
		}

		if(leaf)
		{
			assert(0 == sub_pgno);
			continue;
		}

		if(sub_pgno == 0)
		{
			assert(node->node_flag & e_btree_page_flag_leaf);
			if(i == 0)
				leaf = 1;

			continue;
		}

		sub_node = btree_mgr_get_node(node->btree_mgr, sub_pgno, node, i);
		assert(sub_node);

		examin_sort_ok(cursor, sub_node);
		/* 每个当前分支的最大值与最小值必须满足b树的定义 */
		if(i < node->ncell)
		{
			unsigned char * key1 = NULL; 
			unsigned char * key2 = NULL; 

			btree_cellinfo cell_info1; 
			btree_cellinfo cell_info2; 

			btree_mgr_parse_idx(node,		i,						&cell_info1); 
			btree_mgr_parse_idx(sub_node,	sub_node->ncell - 1,	&cell_info2); 

			key1 = MyMemPoolMalloc(NULL, cell_info1.key_sz); 
			key2 = MyMemPoolMalloc(NULL, cell_info2.key_sz); 

			btree_mgr_cell_get_key(node, &cell_info1, key1, cell_info1.key_sz); 
			btree_mgr_cell_get_key(sub_node, &cell_info2, key2, cell_info2.key_sz); 

			assert(cursor->key_cmp(key1, cell_info1.key_sz, 
				key2, cell_info2.key_sz, 
				cursor->context, cursor->context_sz) > 0); 

			MyMemPoolFree(NULL, key1);
			MyMemPoolFree(NULL, key2);
		}
		else if(next_thread_node)
		{
			unsigned char * key1 = NULL; 
			unsigned char * key2 = NULL; 

			btree_cellinfo cell_info1; 
			btree_cellinfo cell_info2; 

			btree_mgr_parse_idx(next_thread_node,	max_idx,				&cell_info1); 
			btree_mgr_parse_idx(sub_node,			sub_node->ncell - 1,	&cell_info2); 

			key1 = MyMemPoolMalloc(NULL, cell_info1.key_sz); 
			key2 = MyMemPoolMalloc(NULL, cell_info2.key_sz); 

			btree_mgr_cell_get_key(next_thread_node, &cell_info1, key1, cell_info1.key_sz); 
			btree_mgr_cell_get_key(sub_node, &cell_info2, key2, cell_info2.key_sz); 

			assert(cursor->key_cmp(key1, cell_info1.key_sz, 
				key2, cell_info2.key_sz, 
				cursor->context, cursor->context_sz) > 0); 

			MyMemPoolFree(NULL, key1);
			MyMemPoolFree(NULL, key2);
		}

		if(i)
		{
			unsigned char * key1 = NULL; 
			unsigned char * key2 = NULL; 

			btree_cellinfo cell_info1; 
			btree_cellinfo cell_info2; 

			btree_mgr_parse_idx(node,		i - 1,	&cell_info1); 
			btree_mgr_parse_idx(sub_node,	0,		&cell_info2); 

			key1 = MyMemPoolMalloc(NULL, cell_info1.key_sz); 
			key2 = MyMemPoolMalloc(NULL, cell_info2.key_sz); 

			btree_mgr_cell_get_key(node, &cell_info1, key1, cell_info1.key_sz); 
			btree_mgr_cell_get_key(sub_node, &cell_info2, key2, cell_info2.key_sz); 

			assert(cursor->key_cmp(key1, cell_info1.key_sz, 
				key2, cell_info2.key_sz, 
				cursor->context, cursor->context_sz) < 0); 

			MyMemPoolFree(NULL, key1);
			MyMemPoolFree(NULL, key2);
		}
		else if(next_thread_node)
		{
			unsigned char * key1 = NULL; 
			unsigned char * key2 = NULL; 

			btree_cellinfo cell_info1; 
			btree_cellinfo cell_info2; 

			btree_mgr_parse_idx(next_thread_node,	min_idx,	&cell_info1); 
			btree_mgr_parse_idx(sub_node,			0,			&cell_info2); 

			key1 = MyMemPoolMalloc(NULL, cell_info1.key_sz); 
			key2 = MyMemPoolMalloc(NULL, cell_info2.key_sz); 

			btree_mgr_cell_get_key(next_thread_node, &cell_info1, key1, cell_info1.key_sz); 
			btree_mgr_cell_get_key(sub_node, &cell_info2, key2, cell_info2.key_sz); 

			assert(cursor->key_cmp(key1, cell_info1.key_sz, 
				key2, cell_info2.key_sz, 
				cursor->context, cursor->context_sz) < 0); 

			MyMemPoolFree(NULL, key1);
			MyMemPoolFree(NULL, key2);
		}

		if(i == 0 || i == node->ncell)
			cur_sub_layer = examin_btree(cursor, node_count, sub_node, next_thread_node, max_idx, min_idx, cur_layer);
		else
			cur_sub_layer = examin_btree(cursor, node_count, sub_node, node, i, i - 1, cur_layer);

		btree_mgr_release_node(sub_node);

		/* 每个子分支的层数必须相同 */
		if(0 == i)
			sub_layer = cur_sub_layer;
		else
			assert(cur_sub_layer == sub_layer);
	}

	return sub_layer + 1;
}

/**
 * @brief 检查一棵b树是否合法
 */
int ExaminBtree(HBTREE_CURSOR hcur, unsigned int * node_count, int need_exam_pager)
{
	int ret;
	assert(hcur && hcur->btree_mgr);
	assert(hcur->btree_mgr->hpgr);

	if(0 != btree_mgr_move_to_root(hcur))
		assert(!1);

	assert(NULL == hcur->cur_node->parent);

	ret = examin_btree(hcur, node_count, hcur->cur_node, NULL, 0, 0, 0);

	if(need_exam_pager)
		PagerExamin(hcur->btree_mgr->hpgr, 1, 1, NULL, 0);

	return ret;
}

/**
 * @brief 检查一棵b树是否合法
 */
void examin_printf_node_cell_array(btree_node * node)
{
	unsigned int i;

#ifdef _MBCSV6		
	printf("node:%x ", node);
#elif defined WIN32
	printf("node:%x ", (long long)node);
#else
	printf("node:%x ", node);
#endif

	for(i = 0; i < node->ncell; i ++)
	{
		unsigned int cell_ptr;
		
		btree_mgr_find_cellptr(node, i, cell_ptr);
		printf("%4x ", cell_ptr);
	}

	printf("\r\n");
}















/**
 * @file btree.c ����b���㷨 2008-03-03 23:26
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.
 *        ����b���㷨,����ҳ�������pager�����Ϲ���
 * 
 * btree page head define
 *   OFFSET   SIZE     DESCRIPTION
 *      0       1      Flags. 1: intkey, 2: hasdata, 4: leaf
 *      1       1      ��¼��Ƭ�Ĵ�С
 *      2       2      byte offset to the first freeblock
 *      4       2      number of cells on this page
 *      6       2      first byte of the cell content area
 *      8       4      Right child (the Ptr(N+1) value).  reserve on leaves.
 *
 * һ����¼�ķֲ�����
 *    SIZE    DESCRIPTION
 *      4     Page number of the left child. Omitted if leaf flag is set.
 *      4     Number of bytes of data. Omitted if the zerodata flag is set.
 *      4     Number of bytes of key. Or the key itself if intkey flag is set.
 *      4     First page of the overflow chain.  Omitted if no overflow
 *      *     Payload
 *
 * ���ҳ�ĸ�ʽ����
 *    SIZE    DESCRIPTION
 *      4     Page number of next overflow page
 *      *     Data
 *
 * ҳ�ڿ��п鶨��
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


/* һҳ�ֳ�10�� */
#define BTREE_PERCENT 10

/* ���ʱ,���ҳ�洢Ϊ1/10 */
#define BTREE_MAX_LOCAL 1

/* ÿҳ����С�����4/10 */
#define BTREE_FILL_RATE 4

/* ÿ��cell"ָ��"�Ĵ�С */
#define BTREE_CELL_PTR_SIZE 2

/* master��ĸ��ڵ� */
#define BTREE_MASTER_ROOT 2


/**
 * һ����¼�ķֲ�����
 *    SIZE    DESCRIPTION
 *      4     Page number of the left child. Omitted if leaf flag is set.
 *      4     Number of bytes of data. Omitted if the zerodata flag is set.
 *      4     Number of bytes of key. Or the key itself if intkey flag is set.
 *      4     First page of the overflow chain.  Omitted if no overflow
 *      *     Payload
 */
typedef struct __btree_cell_info_t_
{
	/* cell���ݵ���ʼ */
	unsigned char * cell;
	/* ����cell���ܴ�С */
	unsigned int cell_sz;

	/* payload֮ǰ���ֽ��� */
	unsigned int head_sz;

	/* key�Ĵ�С,���߾���key����(���key�Ǹ��������Ļ�) */
	unsigned int key_sz;

	/* data�Ĵ�С */
	unsigned int data_sz;

	/* ���ش��˶���payload,���������Ļ� */
	unsigned int local_sz;

	/* payload���ܴ�С */
	unsigned int payload_sz;

	/* ��¼���ӵ�ҳ��(����Ƿ�Ҷ�ڵ�) */
	unsigned int lchild_pgno;
	/* �׸�overflow��ҳ�� */
	unsigned int overflow_pgno;
}btree_cellinfo;

/* �˽ṹ��Ĵ洢�ռ丽�ӵ�ҳ�������,����Ȩ��ҳ����������� */
typedef struct __btree_node_
{
	/* ��¼�Ƿ��Ѿ���ʼ������ */
	int init_flag;

	/* ���ڵ� */
	struct __btree_node_ * parent;

	/* ���������Ƿ��иı� */
	unsigned int idx_changed;

	/* ��Ӧ���ڵ��е�cell������ */
	unsigned int idx_in_parent_cellarray;

	/* ҳ��ʶ */
	unsigned char node_flag;

	/* cell���� */
	unsigned int ncell;

	/* ���д洢�ռ��С */
	unsigned int nfree;

	/* ҳ����ָ�� */
	HPAGE_HEAD pg;
	/* ��¼���ڶ�ȡ��ҳ����ָ�� */
	unsigned char * read_buf;
	/* ��ǰ�ڵ��Ӧ��ҳ�� */
	unsigned int pg_no;

	/* �˽ڵ�������ĸ�btree������ */
	struct __btree_mgr_t_ * btree_mgr;

	/*
	* �����cell,������������ָ��¼������ҳ���洢����������,
	* ����ָ����Ӽ�¼ʱ��ҳ�Ѿ�û�ж���ռ���,�������¼�¼��ʱ���������
	* ֮����balance�������ٷ�����ʵ�λ��(�ܹ����ѽڵ����������Ĵ洢�ռ�)
	*/
	struct cell_overflow
	{
		HMYBUFFER cell_buf;
		unsigned int idx;
	}cell_ovfl;
	/* ��ʶ,1:��ʾcell_ovfl��������������Ч�� */
	int has_cell_ovfl;
}btree_node;

typedef struct __btree_cursor_
{
	/* ���α��Ӧbtree��root page num */
	unsigned int root_pgno;

	/* ��ǰ�α�������ҳ�ڵ� */
	btree_node * cur_node;
	/* ��ǰ�α��ڽڵ�cell�����λ�� */
	unsigned int idx;

	/* �Ƚϻص�����,�û�����������Ϣ */
	XCOMPARE key_cmp;
	void * context;
	unsigned int context_sz;

	/* ���α�Ĺ������ĸ�btree���� */
	struct __btree_mgr_t_ * btree_mgr;

	/* �α����� */
	struct __btree_cursor_ * cursor_prev, * cursor_next;

	///* cell��Ϣ����ʱ�洢�� */
	//btree_cellinfo cell_info;

	/* ������,�ڴ���������Ƿ������� */
	int err;
}btree_cursor;

typedef struct __btree_mgr_t_
{
	/* �ڴ�ؾ�� */
	HMYMEMPOOL hm;

	/* ҳ������ */
	HPAGER hpgr;
	/* ҳ�Ĵ�С */
	unsigned int pg_sz;

	/* ��¼���ʱ,�ڱ�ҳ�洢�Ĵ�С���������� */
	unsigned int max_local;
	unsigned int min_local;

	/* �������е��α� */
	btree_cursor * cursor_lst;

	/* �����ٽ�����ʶ�Ӧ�����ֵ,�����,�۳���ÿ���ڵ�ͷ�ռ�Ĵ洢��Ϣ�Ĵ�С */
	unsigned int pg_min_fill_sz;
}btree_mgr_t;


/**
 * btree page head define
 *   OFFSET   SIZE     DESCRIPTION
 *      0       1      Flags. 1: intkey, 2: hasdata, 4: leaf
 *      1       1      ��¼��Ƭ�Ĵ�С
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

/* ҳ��ʶ��ƫ�� */
#define BTREE_HDR_PAGEFLAG_OFFSET (((unused_btree_head *)NULL)->pageflag - (char *)NULL)

/* ������Ƭ������ */
#define BTREE_HDR_NFRAG_OFFSET (((unused_btree_head *)NULL)->nfrag - (char *)NULL)

/* �ڵ��ʶ��ƫ�� */
#define BTREE_HDR_NODEFLAG_OFFSET (((unused_btree_head *)NULL)->nodeflag - (char *)NULL)

/* �ڵ���cellֵ��ƫ�� */
#define BTREE_HDR_NCELL_OFFSET (((unused_btree_head *)NULL)->ncell - (char *)NULL)
/* �ڵ����cellֵ�ռ�Ĵ�С */
#define BTREE_HDR_NCELL_SZ (sizeof(((unused_btree_head *)NULL)->ncell))

/* �ڵ����һ�����д洢��ƫ�Ƶ�ֵ */
#define BTREE_HDR_FIRST_FREE_OFFSET (((unused_btree_head *)NULL)->first_free - (char *)NULL)

/* �ڵ�content����ʼ */
#define BTREE_HDR_FIRST_CONTENT (((unused_btree_head *)NULL)->first_content - (char *)NULL)

/* ��¼�ڵ����ұ��ӽڵ��ƫ�� */
#define BTREE_HDR_MOST_RIGHT_CHILD_OFFSET (((unused_btree_head *)NULL)->most_right_child - (char *)NULL)

/* �ڵ��ͷ��Ϣ�����ֵ */
#define BTREE_HDR_SZ (sizeof(unused_btree_head))

/* cellƫ������ĳ�ʼƫ�� */
#define BTREE_CELL_PTR_OFFSET (sizeof(unused_btree_head))

/* ������Ƭ������ */
#define BTREE_MAX_FRAG 128

/* һ���ڵ�ĳ����� */
#define BTREE_NODE_PAYLOAD(_btree_mgr) (_btree_mgr->pg_sz - BTREE_HDR_SZ)


/**
 * ���ҳ�ĸ�ʽ����
 *    SIZE    DESCRIPTION
 *      4     Page number of next overflow page
 *      *     Data
 */
typedef struct __unused_overflow_head_
{
	char next[4];
	char data[1];
}unused_overflow_head;

/* ���ҳͷ��Ϣ�Ĵ�С */
#define BTREE_OVERFLOW_HDR_SZ (((unused_overflow_head *)NULL)->data - (char *)NULL)

/* ���ҳ�ĳ�����Ϣ�� */
#define BTREE_OVERFLOW_PAYLOAD(__btree_mgr) ((__btree_mgr)->pg_sz - BTREE_OVERFLOW_HDR_SZ)


/*
 * ҳ�ڿ��п鶨��
 *    SIZE    DESCRIPTION
 *      2     Byte offset of the next freeblock
 *      2     Bytes in this freeblock
 */
typedef struct __unused_free_block_
{
	char next[2];
	char sz[2];
}unused_free_block;

/* ��һ���ռ���ƫ�� */
#define BTREE_FREE_BLOCK_NEXT_OFFSET (((unused_free_block*)NULL)->next - (char *)NULL)

/* �����п�Ĵ�С */
#define BTREE_FREE_BLOCK_SZ_OFFSET (((unused_free_block*)NULL)->sz - (char *)NULL)

/* ���п����Сֵ */
#define BTREE_FREE_BLOCK_MIN_SZ (sizeof(unused_free_block))


/*
 * һ����¼�ķֲ�����
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

/* cellͷ��Ϣ��󳤶� */
#define BTREE_CELL_HDR_SZ (sizeof(unused_cell))

/* cell����Сֵ */
#define BTREE_CELL_MIN_SZ BTREE_CELL_HDR_SZ

/* lchild��ƫ�� */
#define BTREE_CELL_LCHILD_OFFSET (((unused_cell *)NULL)->lchild - (unsigned char *)NULL)
/* lchild�Ĵ�С */
#define BTREE_CELL_LCHILD_SZ (sizeof(((unused_cell *)NULL)->lchild))


/**
 * @brief ����һ��cell�ܻ�ռȥ���ٿռ�
 */
#define btree_mgr_cal_cell_real_fill_sz(__c_sz) (__c_sz + BTREE_CELL_PTR_SIZE)

/**
 * @brief ����ĳ���ڵ������
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
 * @brief ����ĳ���ڵ����ҳ�����ü���
 */
#define btree_mgr_ref_node(_n) do{\
		assert(_n && _n->pg); \
		PageHeadRef(_n->pg);\
	}while(0)

/**
 * @brief ��ʼ��һ���ڵ�
 * @param bnew:�Ƿ���һ���·���ڵ�
 */
static __INLINE__ int btree_mgr_init_node(btree_node * node, int bnew)
{
	/*
	* btree page head define
	*   OFFSET   SIZE     DESCRIPTION
	*      0       1      Flags. 1: intkey, 2: hasdata, 4: leaf
	*      1       1      ��¼��Ƭ�Ĵ�С
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
			/* ˵�����Ǹ��ڵ��ʱû���κδ洢 */
			node->nfree = node->btree_mgr->pg_sz - BTREE_CELL_PTR_OFFSET;
		}
		else
		{
			/* ���д洢�ռ��С */
			node->nfree = cell_content - (2 * node->ncell + BTREE_CELL_PTR_OFFSET) +
				data[BTREE_HDR_NFRAG_OFFSET];

			get_2byte(&(data[BTREE_HDR_FIRST_FREE_OFFSET]), BTREE_CELL_PTR_SIZE, free_buf);

			/* ˳�ſ���������ͳ�� */
			while(free_buf)
			{
				unsigned int sz = 0;
				unsigned int next_free = 0;

				get_2byte(&(data[free_buf + BTREE_FREE_BLOCK_SZ_OFFSET]), sizeof(unsigned short), sz);
				node->nfree += sz;

				get_2byte(&(data[free_buf + BTREE_FREE_BLOCK_NEXT_OFFSET]), BTREE_CELL_PTR_SIZE, next_free);

				/* ���п鰴ƫ�Ƶ���������,���Ҳ���������(�ͷ�ʱҪ����ϲ��������п�����) */
				assert(next_free > free_buf + BTREE_FREE_BLOCK_MIN_SZ || 0 == next_free);

				free_buf = next_free;
			}
		}

		node->idx_changed = 0;

		return 0;
	}
}

/**
 * @brief ȡ����ĳһҳ������,������������ڵ�.
 */
static __INLINE__ int btree_mgr_release_node(btree_node * node)
{
	assert(node);

	return  PagerReleasePage(node->pg);
}

/**
 * @brief ��ȡһ���ڵ�
 * @param b_in_cache:�Ƿ�ֻ�ӻ������ȡ
 * @param b_new:�Ƿ����µĽڵ�
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

	/* ����Ѿ���ʼ������,����������Щ���� */
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
 * @brief ��ȡһ���ڵ�
 */
#define btree_mgr_get_node_new(__btr_mgr, __pgno, __parent, __idx_in_parent) \
	btree_mgr_get_node_aux(__btr_mgr, __pgno, __parent, __idx_in_parent, 0, 1)

/**
 * @brief ��ȡһ���ڵ�
 */
#define btree_mgr_get_node(__btr_mgr, __pgno, __parent, __idx_in_parent) \
	btree_mgr_get_node_aux(__btr_mgr, __pgno, __parent, __idx_in_parent, 0, 0)

/**
 * @brief ��ȡһ�������нڵ�,���ڻ�����,��ʧ�ܷ���
 */
#define btree_mgr_get_cached_node(__btr_mgr, __pgno, __parent, __idx_in_parent) \
	btree_mgr_get_node_aux(__btr_mgr, __pgno, __parent, __idx_in_parent, 1, 0)

/**
 * @brief ����һ���ڵ�,������ڵ�ĳ���ҳ���ü�������1
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
 * @brief ��pager����һҳ,����һ���µĽڵ�
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
 * @brief ���α��������
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
 * @brief ���α������������
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
 * @brief ������һ���α�
 */
static __INLINE__ int btree_mgr_release_cursor(btree_cursor * cursor)
{
	assert(cursor && cursor->btree_mgr);

	if(cursor->cur_node)
		btree_mgr_release_node(cursor->cur_node);

	/* ������������ */
	assert(cursor->cursor_next || cursor->cursor_prev || cursor == cursor->btree_mgr->cursor_lst);
	btree_mgr_out_of_cursor_list(cursor->btree_mgr, cursor);

	MyMemPoolFree(cursor->btree_mgr->hm, cursor);

	return 0;
}

/**
 * @brief ������һ���α�
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

	/* �����α����� */
	btree_mgr_add_to_cursor_list(btree_mgr, cursor);

	return cursor;
}

/**
 * @brief ���α��������ڵ�
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
 * @brief ����btree������
 */
static int btree_mgr_destroy(btree_mgr_t * btree_mgr)
{
	btree_cursor * cursor;

	assert(btree_mgr);

	/* �ͷ������α� */
	cursor = btree_mgr->cursor_lst;
	while(cursor)
	{
		btree_mgr_release_cursor(cursor);
		cursor = btree_mgr->cursor_lst;
	}

	/* ����pager */
	if(btree_mgr->hpgr)
		PagerDestruct(btree_mgr->hpgr);

	/* �ͷ��Լ� */
	MyMemPoolFree(btree_mgr->hm, btree_mgr);

	return 0;
}

/**
 * @brief ȥ��ovfl��Ϣ
 */
#define btree_mgr_clear_ovfl(__n) do{\
		(__n)->has_cell_ovfl = 0;\
		MyBufferDestruct((__n)->cell_ovfl.cell_buf);\
		(__n)->cell_ovfl.cell_buf = NULL;\
	}while(0)

/**
 * @brief ҳ�����ص����� 
 */
static int btree_page_release(HPAGE_HEAD pg)
{
	btree_node * node = NULL;

	/* LOG_DEBUG(("btree_page_release")); */

	/* ���ü���Ϊһ��Ϊ�� */
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

	/* ȡ���Ը��ڵ������ */
	if(node->parent)
	{
		btree_node * parent = node->parent;
		node->parent = NULL;
		btree_mgr_release_node(parent);
	}

	/* cell_ovfl���bufferҪ�ͷ�,��Ϊû��������,���������ڴ�й© */
	btree_mgr_clear_ovfl(node);

	/* ���ڵ�ĳ�ʼ����־�ó�0 */
	node->init_flag = 0;

	return 0;
}

/**
 * @brief ҳ��������ص�����
 */
static int btree_page_reload(HPAGE_HEAD pg)
{
	btree_node * node = NULL;

	LOG_DEBUG(("btree_page_reload"));

	assert(sizeof(*node) == PageHeadGetUserDataSize(pg));

	node = PageHeadGetUserData(pg);

	assert(node);

	/* ���ڵ�ҳ�����Ϣ���¶�һ�� */
	if(node->init_flag)
		btree_mgr_init_node(node, 0);

	return 0;
}


/**
 * @brief ����btree
 * @param pg_sz:btree �� page size,��Ϊ0
 * @param cache_pg_count:btreeҳ�������ֵ,��Ϊ0;
 * @param sector_sz:OS�����Ĵ�С,��Ϊ0
 * @param rand_seed:�������ʼ������
 * @param rand_seed_sz:rand_seed��ָ�������Ĵ�С
 * @param user_rate:ҳ�ļ�ʹ����,��Ϊ0
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
	* ��¼���ʱ,�ڱ�ҳ�洢�Ĵ�С����������
	* max_localӦΪpage_sz��1/10,
	*/
	assert(btree_mgr->pg_sz / BTREE_PERCENT > (BTREE_CELL_PTR_SIZE + BTREE_CELL_HDR_SZ));

	btree_mgr->max_local = (((btree_mgr->pg_sz - BTREE_HDR_SZ) / BTREE_PERCENT 
		- (BTREE_CELL_PTR_SIZE + BTREE_CELL_HDR_SZ)) / SYS_ALIGNMENT) * SYS_ALIGNMENT;
	btree_mgr->min_local = ((btree_mgr->max_local / 2) / SYS_ALIGNMENT) * SYS_ALIGNMENT;

	btree_mgr->pg_min_fill_sz = ((btree_mgr->pg_sz - BTREE_HDR_SZ) * BTREE_FILL_RATE) / BTREE_PERCENT;
	assert(btree_mgr->pg_min_fill_sz > BTREE_HDR_SZ + btree_mgr->max_local);

	assert(btree_mgr->max_local >= 2 * BTREE_CELL_MIN_SZ);
	assert(btree_mgr->min_local >= 2 * BTREE_CELL_MIN_SZ);

	/* �α�����ͷ�ÿ� */
	btree_mgr->cursor_lst = NULL;
	return btree_mgr;

btreeMgrConstruct_end_:

	btree_mgr_destroy(btree_mgr);
	return NULL;
}

/**
 * @brief ����btree
 */
int btreeMgrDestruct(HBTREE_MGR hbtreeMgr)
{
	if(hbtreeMgr)
		return btree_mgr_destroy(hbtreeMgr);

	return -1;
}

/**
 * @brief �ͷ��α� ��btreeMgrGetCursor�Ƕ�ż����
 */
int btreeMgrReleaseCursor(HBTREE_CURSOR hcur)
{
	if(NULL == hcur || NULL == hcur->btree_mgr)
		return -1;

	btree_mgr_release_cursor(hcur);
	return 0;
}

/**
 * @brief ��cell��ȡ��data
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

	/* ���û�з������,�������� */
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

		/* ����key��ռ�е����� */
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

		/* ˵��data�ڱ�ҳҲ�в��ִ洢 */

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
 * @brief ��cell��ȡ��key
 */
static __INLINE__ int btree_mgr_cell_get_key(btree_node * node,
											 btree_cellinfo * cell_info,
											 unsigned char * key, unsigned int key_sz)
{
	assert(cell_info);

	/* ���������key,��ֵ���� */
	if(node->node_flag & e_btree_page_flag_intkey)
		return 0;

	/* ���û�з������,�������� */
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

		/* ���key���ڱ��ڵ�ҳ�洢,�������� */
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
			/* �����key�����ҳ��Ҳ�д洢,��key_sz������ȫ���������� */
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
 * @brief ����һ������õ�cell
 */
static __INLINE__ int btree_mgr_parse_cell(btree_node * node,
										   const unsigned char * cell, const unsigned int cell_buf_sz,
										   btree_cellinfo * cell_info)
{
	/**
	 * һ����¼�ķֲ�����
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
		/* �������Ҷ�ڵ� */
		array_to_uint_as_big_endian(cell, sizeof(unsigned int), cell_info->lchild_pgno);
		cell_sz += sizeof(unsigned int);
		cell += sizeof(unsigned int);
	}

	if(node->node_flag & e_btree_page_flag_hasdata)
	{
		/* ����ڵ㺬�������� */
		array_to_uint_as_big_endian(cell, sizeof(unsigned int), cell_info->data_sz);
		cell_sz += sizeof(unsigned int);
		cell += sizeof(unsigned int);
		payload_sz += cell_info->data_sz;
	}

	/* ����key�� */
	array_to_uint_as_big_endian(cell, sizeof(unsigned int), cell_info->key_sz);
	cell_sz += sizeof(unsigned int);
	cell += sizeof(unsigned int);

	/* ����������ε�key */
	if(!(node->node_flag & e_btree_page_flag_intkey))
		payload_sz += cell_info->key_sz;

	/* ����payload_sz��ֵ,�жϵ�ǰcell�Ƿ�������� */
	if(payload_sz > node->btree_mgr->max_local)
	{
		unsigned int surplus = payload_sz % BTREE_OVERFLOW_PAYLOAD(node->btree_mgr);
		if(surplus <= node->btree_mgr->max_local)
			cell_info->local_sz = surplus;
		else
			cell_info->local_sz = node->btree_mgr->min_local;

		/* ����overflowҳ�� */
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
 * @brief ��cell"ָ��"ָ������ݽ�������
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
 * @brief ��ȡָ��idx��cell"ָ��"
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
 * @brief ������Ӧidx��Ӧcell����Ϣ
 */
#define btree_mgr_parse_idx(_n, _i, _ci) do{\
		unsigned int __cell_ptr = 0;\
		assert((_n) && (_ci));\
		assert((_i) < _n->ncell);\
		btree_mgr_find_cellptr((_n), (_i), __cell_ptr);\
		btree_mgr_parse_cellptr((_n), __cell_ptr, (_ci));\
	}while(0)

/**
 * @brief ȡ��ĳ���ڵ�ָ��idx��lchild
 */
#define btree_mgr_get_idx_lchild(_n, _i, _lchild) do{\
		btree_cellinfo __ci;\
		assert((_n));\
		assert((_i) < (_n)->ncell);\
		btree_mgr_parse_idx(_n, _i, &__ci);\
		_lchild = __ci.lchild_pgno;\
	}while(0)

/**
 * @brief ���cell info��Ϣ,����cell_sz�Ĵ�С(����cellͷ����,�Լ�local payload,�����������payload)
 */
static __INLINE__ unsigned int btree_mgr_fill_cellinfo(btree_node * node,
													   unsigned int key_sz, unsigned data_sz,
													   btree_cellinfo * cell_info)
{
	/* key��һ�����ڵ�,��������ҳ�������Լ�payload��ֵ�п��ܱ����� */
	unsigned int cell_sz = sizeof(unsigned int);
	unsigned int payload_sz = 0;

	assert(node && node->btree_mgr);
	assert(cell_info);

	memset(cell_info, 0, sizeof(*cell_info));
	cell_info->key_sz = key_sz;

	/* �Ƿ���Ҷ�ӽڵ� */
	if(!(node->node_flag & e_btree_page_flag_leaf))
		cell_sz += sizeof(unsigned int);

	/* �ؼ����Ƿ�Ϊ���� */
	if(!(node->node_flag & e_btree_page_flag_intkey))
		payload_sz += key_sz;

	/* �Ƿ���������� */
	if(node->node_flag & e_btree_page_flag_hasdata)
	{
		cell_info->data_sz = data_sz;
 
		cell_sz += sizeof(unsigned int);
		payload_sz += data_sz;
	}

	/* payload�Ƿ���� */
	if(payload_sz <= node->btree_mgr->max_local)
	{
		cell_info->head_sz = cell_sz;
		cell_sz += payload_sz;
		cell_info->local_sz = payload_sz;
	}
	else
	{
		/* todo:�˶��߼���btree_mgr_parse_cellptr�еĶ�Ӧ�Ĳ��ֿ�������һ������ */
		unsigned surplus = payload_sz % BTREE_OVERFLOW_PAYLOAD(node->btree_mgr);

		/* �������ҳҳ�Ŵ洢�ռ� */
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
 * @brief ����û�������,�γ�һ��cell
 */
static __INLINE__ int btree_mgr_pack_cell(btree_node * node,
										  btree_cellinfo * cell_info,
										  unsigned char * cell, const unsigned int cell_sz,
										  const void * key, const unsigned int key_sz,
										  const void * data, const unsigned int data_sz)
{
	/**
	 * һ����¼�ķֲ�����
	 *    SIZE    DESCRIPTION
	 *      4     Page number of the left child. Omitted if leaf flag is set.
	 *      4     Number of bytes of data. Omitted if the zerodata flag is set.
	 *      4     Number of bytes of key. Or the key itself if intkey flag is set.
	 *      4     First page of the overflow chain.  Omitted if no overflow
	 *      *     Payload
	 */

	int ret = 0;

	btree_mgr_t * btree_mgr = NULL;

	/* ��¼����ڵ����ʱָ�� */
	HPAGE_HEAD pg_ovfl = NULL;

	/* ��¼payload�Ĵ�С */
	unsigned int payload_sz = 0;

	/* ��ʱָ��,ָ�����payload�Ĵ洢�ռ� */
	unsigned char * pPayload = NULL;

	/* ��ʱ�洢��ǰ���ڴ洢payload�Ŀռ�ʣ���С */
	unsigned int space_left = 0;

	/* ��ʱָ��,ָ��Ϊ���ҳ��Ԥ���Ĵ洢�ռ� */
	unsigned char * pPrior = NULL;

	/* ��ʱָ��,��ʼֵָ��key,���keyΪ����,��ָ��data(�����data�Ļ�) */
	unsigned char * pSrc = NULL;
	unsigned int Src_sz = 0;

	assert(node && node->btree_mgr);
	assert(cell && cell_sz && cell_info);
	assert(cell_sz == cell_info->cell_sz);
	assert(data_sz == cell_info->data_sz);
	assert(key_sz == cell_info->key_sz);

	cell_info->cell = cell;

	/* �������Ҷ�ڵ�,Ҫ������ӵ�ҳ���� */
	if(!(node->node_flag & e_btree_page_flag_leaf))
	{
		assert(cell_info->lchild_pgno);

		uint_to_big_endian(cell_info->lchild_pgno, cell, sizeof(unsigned int));
		cell += sizeof(sizeof(unsigned int));

		assert((unsigned int)(cell - cell_info->cell) <= cell_sz);
	}

	/* �������������,Ҫ���������ĳ��� */
	if(node->node_flag & e_btree_page_flag_hasdata)
	{
		assert(data && data_sz);

		uint_to_big_endian(data_sz, cell, sizeof(unsigned int));
		cell += sizeof(sizeof(unsigned int));

		assert((unsigned int)(cell - cell_info->cell) < cell_sz);
	}

	/* ���key_sz */
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

		/* ������� */
		return 0;
	}

	payload_sz = cell_info->payload_sz;
	if(payload_sz > node->btree_mgr->max_local)
	{
		assert(cell_info->local_sz < payload_sz);

		/* ��������,Ϊ�׸����ҳ��ҳ��Ԥ���洢�ռ� */
		pPrior = cell;
		pPayload = cell + sizeof(unsigned int);
	}
	else
		pPayload = cell;

	/* ��ʼ����payload�ռ�ʣ��Ϊlocal_sz */
	space_left = cell_info->local_sz;
	btree_mgr = node->btree_mgr;
	assert(btree_mgr);

	/* ����key(�����Ҫ)��data(�����Ҫ)��pPayloadָ��Ĵ洢�ռ� */
	while(payload_sz)
	{
		unsigned int n = 0;
		if(0 == space_left)
		{
			/* ����ռ䲻��,����һҳover flow */
			unsigned int pgno = PagerGetPageNo(btree_mgr->hpgr);
			if(0 == pgno)
			{
				ret = -1;
				goto btree_mgr_pack_cell_end_;/* ȡ����ĳЩ�ڵ������ */
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
			/* �����ҳ��д��Ԥ�����ڴ�ռ� */
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
 * @brief �α굱ǰ��ָ��λ�õ�cell��Ϣ
 */
static __INLINE__ int btree_mgr_get_cell(btree_cursor * cursor, btree_cellinfo * cellinfo)
{
	assert(cursor && cursor->cur_node && cursor->root_pgno && cellinfo);
	assert(cursor->cur_node->pg && cursor->cur_node->pg_no);

	/* ȡ��cell"ָ��" */
	/* ����cell"ָ��"ȡ��cell������ */
	btree_mgr_parse_idx(cursor->cur_node, cursor->idx, cellinfo);

	return 0;
}

/**
 * @brief ����cell�Ļص�����
 *  > 0  ��ʾ key ���� data
 *  == 0 ��ʾ key ���� data
 *  < 0  ��ʾ key С�� data
 *
 * @param key:�ؼ���
 * @param key_sz:�ؼ��ֵĻ�������С
 * @param context:�û��Զ��������������
 * @param context_sz:�û��Զ�������������ݵĳ���
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
 * @brief ��ȡright most
 */
#define btree_mgr_get_rmost(_n, _rm) do{\
		assert(_n);\
		assert(!(_n->node_flag & e_btree_page_flag_leaf));\
		assert(_n->read_buf == PageHeadMakeReadable(_n->pg));\
		array_to_uint_as_big_endian(&_n->read_buf[BTREE_HDR_MOST_RIGHT_CHILD_OFFSET], sizeof(unsigned int), _rm);\
		assert(_rm);\
	}while(0)

/**
 * @brief ���ҹؼ�ֵ
 * @param pres:0��ʾ�ҵ��˹ؼ���
 *         ������:��ʾû�ҵ��ؼ���
 *         С����:��ʾû�ҵ��ؼ���
 *         ��� ����Ϊ 1 3 4 ���� 2 
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

	/* ���α��������ڵ� */
	if(0 != btree_mgr_move_to_root(cursor))
		return -1;

	assert(cursor->cur_node->init_flag);
	assert(cursor->cur_node->pg_no == cursor->root_pgno);

	while(1)
	{
		/* �ڵ�ǰ�ڵ��������ֲ��� */
		const unsigned char * rbuf = cursor->cur_node->read_buf;

		assert(PagePtrIsInpage(cursor->cur_node->pg, &rbuf[BTREE_CELL_PTR_OFFSET]));
		assert(PagePtrIsInpage(cursor->cur_node->pg, 
			rbuf + BTREE_CELL_PTR_OFFSET + cursor->cur_node->ncell * BTREE_CELL_PTR_SIZE - 1));

		if(0 == cursor->cur_node->ncell)
		{
			/* ���û��cell,ֱ�ӷ��ؼ��� */
			assert(NULL == cursor->cur_node->parent);
			assert(cursor->cur_node->node_flag & e_btree_page_flag_leaf);

			*pres = 1;
			return 0;
		}

		/* ����ҵ�,���� */
		ret = MyBinarySearch(&rbuf[BTREE_CELL_PTR_OFFSET], cursor->cur_node->ncell, BTREE_CELL_PTR_SIZE,
			key, key_sz, binarysearch_compare,
			&cursor->idx, cursor, sizeof(*cursor));

		if(0 != cursor->err)
		{
			/* ������� */
			cursor->err = 0;
			return -1;
		}

		if(0 == ret)
		{
			/* ����ҵ� */
			*pres = 0;
			return 0;
		}

		if(cursor->cur_node->node_flag & e_btree_page_flag_leaf)
		{
			/* �����ǰ�Ѿ���Ҷ�ڵ�,�򷵻� */
			*pres = ret;
			return 0;
		}

		/* ����"ָ��"ȥȡcell��Ϣ */
		if(cursor->idx < cursor->cur_node->ncell)
		{
			btree_mgr_get_cell(cursor, &cell_info);
			pgno_child = cell_info.lchild_pgno;
		}
		else
			btree_mgr_get_rmost(cursor->cur_node, pgno_child);

		assert(pgno_child);

		/* �Ҳ���,������Ӧ���ӷ�֧�������� */
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
 * @brief ����ĳһ���ڵ�Ŀ�����Ƭ
 */
static __INLINE__ int btree_mgr_defrag_node(btree_node * node)
{
	/* ��¼���ݵ�content�洢�����ָ�� */
	unsigned char * content_new = NULL;
	unsigned int new_cell_ptr = 0;
	/* ��¼������Ƭǰ��content��ʼ */
	unsigned int content_begin = 0;
	/* �ڻؿ������е���ʱ����,��¼cell��"ָ��" */
	unsigned int cell_ptr = 0;
	/* ��ʱ�洢ĳ��cell����Ϣ */
	btree_cellinfo cell_info;

	unsigned char * write_buf = NULL;
	unsigned int i = 0;

	assert(node);

	/*
	* ���ڵ��ڵ�cell content��������
	* ��ѭcell ptr����,����ͷ������˳�ν�������.
	* ������Ӧ��ֵfirst_free,cell_content,nfrag
	*/

	write_buf = PageHeadMakeWritable(node->pg);
	if(NULL == write_buf)
		return -1;

	get_2byte(&(write_buf[BTREE_HDR_FIRST_CONTENT]), BTREE_CELL_PTR_SIZE, content_begin);

	content_new = MyMemPoolMalloc(node->btree_mgr->hm, node->btree_mgr->pg_sz);
	if(NULL == content_new)
		return -1;

	/* ��ʼѭ�� */
	content_new += node->btree_mgr->pg_sz;
	new_cell_ptr = node->btree_mgr->pg_sz;
	for(i = 0; i < node->ncell; i ++)
	{
		btree_mgr_find_cellptr(node, i, cell_ptr);
		assert(cell_ptr);

		/* ������Ӧ��cell��Ϣ */
		btree_mgr_parse_cellptr(node,cell_ptr, &cell_info);

		assert(new_cell_ptr > (BTREE_CELL_PTR_OFFSET + node->ncell * BTREE_CELL_PTR_SIZE + node->nfree));

		/* �����ؽڵ�ҳ���� */
		content_new -= cell_info.cell_sz;
		new_cell_ptr -= cell_info.cell_sz;

		assert(new_cell_ptr);
		assert(cell_ptr >= content_begin);

		memcpy(content_new, &write_buf[cell_ptr], cell_info.cell_sz);

		/* ������Ӧ��cell ptr */
		put_2byte(new_cell_ptr, &write_buf[BTREE_CELL_PTR_OFFSET + i * BTREE_CELL_PTR_SIZE], BTREE_CELL_PTR_SIZE);
	}

	content_begin = new_cell_ptr;

	/* ���е�free�ռ�Ӧ��������gap�� */
	assert((content_begin - node->nfree) == BTREE_CELL_PTR_OFFSET + node->ncell * BTREE_CELL_PTR_SIZE);
	assert(content_begin <= node->btree_mgr->pg_sz);

	memcpy(&write_buf[content_begin], content_new, node->btree_mgr->pg_sz - content_begin);

	/*
	* ������Ӧ��ֵfirst_free,cell_content,nfrag
	* btree page head define
	*   OFFSET   SIZE     DESCRIPTION
	*      0       1      Flags. 1: intkey, 2: hasdata, 4: leaf
	*      1       1      ��¼��Ƭ�Ĵ�С
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
 * @brief ��ָ��ƫ�Ƶ�ҳ�ڴ洢�ռ�黹,��btree_mgr_alloc_space�Ƕ�ż����
 */
static __INLINE__ int btree_mgr_free_space(btree_node * node, unsigned int ptr, const unsigned int sz)
{
	/*
	* ���ptrǡ�ý�����gap֮��,����Ҫ����free"����",
	* ˳������ǰ��,ֱ������ʱ��free_ptr����ptrΪֹ,��ptr��������,��ʱҪע���ж��Ƿ���Ժϲ����ڵĺ��п�
	* ����nfreeֵ,
	* ������Ҫ����first content��first free
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

	/* ���ptr������content,��������content */
	if(ptr == content)
	{
		content = ptr + sz;

		/* �����Ƿ񻹿��Ժϲ�����ĺ��п� */
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

	/* Ѱ��ptrӦ�����λ�� */
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

	/* ��ptr����"����" */
	if(pre_free_store)
	{
		unsigned int pre_sz = 0;
		unsigned int total_sz = 0;
		get_2byte(&wb[pre_free_store + BTREE_FREE_BLOCK_SZ_OFFSET], sizeof(unsigned short), pre_sz);

		if(pre_free_store + pre_sz == ptr)
		{
			/* ������ǰһ��ϲ� */
			total_sz = pre_sz + sz;

			/* �Ƿ�������鶼�ϲ� */
			if(ptr + sz == cur_free && cur_free)
			{
				/* ��¼�ϲ������һ�����п������ */
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
			/* ǰһ������ڴ�nextָ��ptr */
			put_2byte(ptr, &wb[pre_free_store + BTREE_FREE_BLOCK_NEXT_OFFSET], BTREE_CELL_PTR_SIZE);
		}
	}
	else
	{
		put_2byte(ptr, &wb[BTREE_HDR_FIRST_FREE_OFFSET], BTREE_CELL_PTR_SIZE);
	}

	if(ptr + sz == cur_free)
	{
		/* �����������һ�����п�ϲ� */
		unsigned int cur_next = 0;
		get_2byte(&wb[cur_free + BTREE_FREE_BLOCK_SZ_OFFSET], sizeof(unsigned short), next_sz);
		get_2byte(&wb[cur_free + BTREE_FREE_BLOCK_NEXT_OFFSET], BTREE_CELL_PTR_SIZE, cur_next);
		
		/* �����¿���п�,�����п�Ĵ�С */
		put_2byte(cur_next, &wb[ptr + BTREE_FREE_BLOCK_NEXT_OFFSET], BTREE_CELL_PTR_SIZE);
		put_2byte(sz + next_sz, &wb[ptr + BTREE_FREE_BLOCK_SZ_OFFSET], sizeof(unsigned short));
	}
	else
	{
		/* ���ܺϲ�,��ptr��next��ָ��cur_free, ���뱾���п�Ĵ�С */
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
 * @brief ��һ��node���洢��cell�óɿ�
 */
static __INLINE__ int btree_mgr_clear_node(btree_node * node)
{
	/* ��frag�ó�0,��ncell�ó�0,��content�õ��ײ�,��first free�ó��� */
	/* right most�ó�0 */

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
 * @brief ��ҳ�����ڷ���ָ����С�Ĵ洢�ռ�,���ص�ʱ�ռ�����û�ҳ������õ�ַ��ʼ��ƫ��
 */
static __INLINE__ unsigned int btree_mgr_alloc_space(btree_node * node, unsigned int sz)
{
	/*
	* btree page head define
	*   OFFSET   SIZE     DESCRIPTION
	*      0       1      Flags. 1: intkey, 2: hasdata, 4: leaf
	*      1       1      ��¼��Ƭ�Ĵ�С
	*      2       2      byte offset to the first freeblock
	*      4       2      number of cells on this page
	*      6       2      first byte of the cell content area
	*      8       4      Right child (the Ptr(N+1) value).  reserve on leaves.
	*/

	/*
	* ҳ�ڿ��п鶨��
	*    SIZE    DESCRIPTION
	*      2     Byte offset of the next freeblock
	*      2     Bytes in this freeblock
	*/

	unsigned int free_idx = 0;
	unsigned int pc_prev = 0;
	unsigned int block_sz = 0;
	unsigned int ret_idx = 0;

	/* cell"ָ��"�Ľ��� */
	unsigned int cell_ptr_end = 0;
	/* cell content����ʼ */
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
	* ������ڿռ��,��ӿռ�����������
	* ��������ڿ��п�,���cellָ�������δβ�� cell content֮����Ƕ������ȡ(���¼��gap)
	* �����Ȼ��ȡ����,��Ӧ�ϲ�������Ƭ,
	* ��gap�������Ҫ�Ĵ洢�ռ�
	*/

	/* ��gap�����,�����Ȼ���䲻��,��ʱ����Ҫ���п�����Ƭ���� */
	cell_ptr_end = BTREE_CELL_PTR_OFFSET + BTREE_CELL_PTR_SIZE * node->ncell;
	get_2byte(&(write_buf[BTREE_HDR_FIRST_CONTENT]), BTREE_CELL_PTR_SIZE, cell_content);

	/* cell_contentΪ0,˵����һ���µĽڵ� */
	if(0 == cell_content)
		cell_content = node->btree_mgr->pg_sz;

	assert(cell_content >= cell_ptr_end);

	/* Ϊcell ptr���������Ԥ���ֽ� */
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

		/* ���������ƬС��BTREE_MAX_FRAG */
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
						* ���ʣ����ڴ治��4�ֽ�,�޷��γ�һ�����п�"����"�ڵ�
						* ���������п鶼�����ȥ,�����пռ�ȴֻ�ۼ���Ӧ��szֵ,����Ӧ����nfrag��ֵ
						* ��������Ƭ�ﵽ����,��ʱ������Ƭ����,�ͷų����еĿ�����Ƭ,������ʹ��
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
						/* �ۼ���Ӧ��sz,���� */
						put_2byte(block_sz - sz,
							&(write_buf[free_idx + BTREE_FREE_BLOCK_SZ_OFFSET]),
							sizeof(unsigned short));
					}

					/* �ӿ��п�ĵײ��ó�һ���� */
					ret_idx = free_idx + block_sz - sz;
					goto btree_mgr_alloc_space_end_;
				}

				/* ѭ������ */
				pc_prev = free_idx + BTREE_FREE_BLOCK_NEXT_OFFSET;
				get_2byte(&(write_buf[free_idx + BTREE_FREE_BLOCK_NEXT_OFFSET]),
					sizeof(unsigned short),
					free_idx);
			}
		}
		else
		{
			/* ��Ƭ����������,������Ƭ���� */
			if(0 != btree_mgr_defrag_node(node))
				return 0;

			get_2byte(&(write_buf[BTREE_HDR_FIRST_CONTENT]), BTREE_CELL_PTR_SIZE, cell_content);
		}

		/* �����Ȼ���䲻��,��Ҫ���п�����Ƭ���� */
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
 * @brief ��һ��cell�Խ���
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

	/* �ռ��㹻,����ռ�,�洢 */
	ptr = btree_mgr_alloc_space(node, cell_sz);
	if(0 == ptr)
		return -1;

	wb = PageHeadMakeWritable(node->pg);
	if(NULL == wb)
		return -1;

	/* �洢cell���� */
	memcpy(&wb[ptr], cell, cell_sz);

	/* ����cell ptr���� */
	end = BTREE_CELL_PTR_OFFSET + node->ncell * BTREE_CELL_PTR_SIZE;
	ins = BTREE_CELL_PTR_OFFSET + idx * BTREE_CELL_PTR_SIZE;
	for(i = end; i > ins; i -= BTREE_CELL_PTR_SIZE)
	{
		wb[i] = wb[i - 2];
		wb[i + 1] = wb[i - 1];
	}

	put_2byte(ptr, &wb[ins], BTREE_HDR_NCELL_SZ);

	/* ���ı�ʶλidx_change */
	node->idx_changed = 1;

	return 0;
}

/**
 * @brief ��һ������õ�cell��ӵ�ָ����λ��
 */
static __INLINE__ int btree_mgr_insert_cell(btree_node * node,
											unsigned int idx,
											unsigned char * cell, unsigned int cell_sz)
{
	/*
	* ����ýڵ�ʹ�õ�ҳ�ϵĴ洢�ռ��㹻,
	* ���µ�cell����ȥ
	* �������,�͹��ڸ��ӿռ���,��֮���ƽ��������ٿ�����ȥ
	*/

	unsigned char * wb = NULL;

	assert(node && cell && cell_sz);
	assert(0 == node->has_cell_ovfl);
	assert(idx <= node->ncell);

	wb = PageHeadMakeWritable(node->pg);
	if(NULL == wb)
		return -1;

	/* �ռ䲻��,����cell_ovfl��Ϣ��,��balance����֮����ת����������ҳ������ */
	if(node->nfree < (cell_sz + BTREE_CELL_PTR_SIZE))
	{
		if(NULL == node->cell_ovfl.cell_buf)
			node->cell_ovfl.cell_buf = MyBufferConstruct(node->btree_mgr->hm, cell_sz);

		MyBufferSet(node->cell_ovfl.cell_buf, cell, cell_sz);
		node->cell_ovfl.idx = idx;
		node->has_cell_ovfl = 1;

		/* ���ı�ʶλidx_change */
		node->idx_changed = 1;

		assert(node->cell_ovfl.idx <= node->ncell);

		goto btree_mgr_insert_cell_end_;
	}

	if(0 != btree_mgr_eat_cell(node, idx, cell, cell_sz))
		return -1;

btree_mgr_insert_cell_end_:

	/* ����ncell��ֵ+1 */
	node->ncell += 1;
	put_2byte(node->ncell, &wb[BTREE_HDR_NCELL_OFFSET], BTREE_HDR_NCELL_SZ);

	return 0;
}

/**
 * @brief �������cellȥ��,ncellҪ��1
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
 * @brief �������cell�Ի���
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
 * @brief �ӽڵ���ɾ��һ��cell
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

	/* ncell������1,������cell ptr����,idx change��ʶ��1 */
	if(0 != btree_mgr_free_space(node, cell_ptr, cell_sz))
		return -1;

	if(node->has_cell_ovfl)
		ncell_in = node->ncell - 1;
	else
		ncell_in = node->ncell;

	/* ����̵�ptr��ǰ�� */
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

	/* ncell������1 */
	node->ncell -= 1;
	put_2byte(node->ncell, &wb[BTREE_HDR_NCELL_OFFSET], BTREE_HDR_NCELL_SZ);

	/* idx change��ʶ��1 */
	node->idx_changed = 1;

	return 0;
}

/**
 * @brief �ӽڵ���ɾ��һ����¼,�������ڵ��cell,�Լ���̵�ovflҳ(�����)
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

	/* ��������ҳ */
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
 * @brief ����idx�±���ָ�ڵ��lchild,������ncell,���ʾ����right most child
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
		/* ����right most���� */
		uint_to_big_endian(pgno_child, &wb[BTREE_HDR_MOST_RIGHT_CHILD_OFFSET], sizeof(unsigned int));
		return 0;
	}

	btree_mgr_find_cellptr(node, idx, cell_ptr);
	btree_mgr_parse_cellptr(node, cell_ptr, &cell_info);

	uint_to_big_endian(pgno_child, &cell_info.cell[BTREE_CELL_LCHILD_OFFSET], BTREE_CELL_LCHILD_SZ);

	return 0;
}

/**
 * @brief �ж�nodeλ�ڸ��ڵ��idx
 */
static __INLINE__ int btree_mgr_check_idx_in_parent(btree_node * node, unsigned int * idx_in_parent)
{
	unsigned int j = 0;
	btree_cellinfo temp_cell_info;
	unsigned int cell_ptr = 0;

	/* ������ڵ㲻����,���߸��ڵ�û�з����ı�,����Ҫ���ô˺��� */
	assert(node && node->parent);
	assert(node->parent->idx_changed);
	assert(idx_in_parent);
	assert(!node->parent->has_cell_ovfl && NULL == node->parent->cell_ovfl.cell_buf);

	/* ��parent�ڵ������������ѯ,���� */
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

	/* ����Ҳ���,˵�������ұߵ���һ�� */
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
 * @brief ���軺���е��ӽڵ�parentָ����idx_in_parent
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
 * @brief ���һ������Ľڵ�
 */
static int btree_mgr_div_node(btree_node * node)
{
	/*
	* ������������¼,��Ҫ���ѽ��
	* pager�����µ�һҳ,����ֽ���payload,�м��������¼������ϲ�ڵ�(���ܻᵼ���ϲ�ڵ����)
	*/

	/*
	* ��ԭʼ��cell ptr���鿽������,����cell ovflҲ�ں��ʵ�λ�����
	* ѭ��������ÿ��cell�Ĵ�С,�����Ƿ�ﵽ�ٽ����ֵʱ�ķֽ��
	* ���ֽ��������ϲ�ڵ�
	* ������ҳ,��ʣ���cellת������ҳ��,ͬʱ��Ӧ�����ϲ���ӽڵ�����,
	* ��ԭʼҳ��ʣ���cell����ɾ��
	*/
	int ret = 0;
	unsigned int * cell_array = NULL;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int k = 0;

	/* ��ʱ����,�������ֵ */
	unsigned int fill_sz = 0;
	/* ����ֽ����cell ptr��index */
	unsigned int idx_div = 0;
	/* ��ʱ���浱ǰcell info */
	btree_cellinfo cell_info;
	/* ��ʱ����ѭ�������е�cell"ָ��" */
	unsigned int cell_ptr = 0;
	/* ��ʱ����,����idx_div��lchild */
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

	/* ������ʱ�洢�ռ�,���ڴ��cell ptr���� */
	/* todo: �������ڴ�й©cell_array��û�б��ͷ� */
	cell_array = MyMemPoolMalloc(btree_mgr->hm, node->ncell * sizeof(cell_array[0]));
	if(NULL == cell_array)
		return -1;

	for(; i < (node->ncell - 1) && k < node->ncell; i ++, k ++)
	{
		if(node->cell_ovfl.idx == i)
		{
			/* 0��ʾ�����������¼ */
			cell_array[k] = 0;
			k += 1;
		}

		btree_mgr_find_cellptr(node, i, cell_ptr);

		cell_array[k] = cell_ptr;
	}

	/* ѭ��������ÿ��cell�Ĵ�С,�����Ƿ�ﵽ�ٽ����ֵʱ�ķֽ�� */
	for(i = 0; i < node->ncell; i ++)
	{
		if(cell_array[i])
			btree_mgr_parse_cellptr(node, cell_array[i], &cell_info);
		else
		{
			/* �����������Ǹ�cell,ֱ�Ӵ�cell_ovfl��Ϣ��ȡ */
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

	/* ȡ���ֽ���lchild */
	if(cell_array[idx_div])
	{
		btree_mgr_parse_cellptr(node, cell_array[idx_div], &cell_info);
	}
	else
	{
		/* �����������Ǹ�cell,ֱ�Ӵ�cell_ovfl��Ϣ��ȡ */
		unsigned int temp_len = 0;
		const unsigned char * pctemp = MyBufferGet(node->cell_ovfl.cell_buf, (size_t *)&temp_len);
		btree_mgr_parse_cell(node, pctemp, temp_len, &cell_info);
	}
	div_lchild = cell_info.lchild_pgno;

	/*
	* ���ֽ��������ϲ�ڵ�
	* ������Ҫ�ж��Ƿ��Ǹ��ڵ�,����Ǹ��ڵ�,��Ҫ����������ҳ
	*/
	if(node->parent)
	{
		/* todo:���Կ��ǽ�idx_divǰ���cell����new_node,�������Լ���һЩ�ڴ���� */

		/* �Ǹ��ڵ�,������ϲ�ڵ㼴�� */
		btree_node * new_node = NULL;
		unsigned int idx_ovfl = 0;

		unsigned int idx_in_parent = node->idx_in_parent_cellarray;
		if(node->parent->idx_changed)
			btree_mgr_check_idx_in_parent(node, &idx_in_parent);

		/* ��idx_div֮���Ԫ��ת�����µ�ҳ,������parent����Ӧ��"ָ��" */
		new_node = btree_mgr_new_node(btree_mgr, node->parent, idx_in_parent + 1, node->node_flag);
		if(NULL == new_node)
		{
			ret = -1;
			goto btree_mgr_div_node_end_;
		}

		/* ����parent��idex_in_parent��Ӧ��lchild,Ҫ��ǰ����,��Ϊ����п��ܵ��¸��ڵ���� */
		if(0 != btree_mgr_update_idx_lchild(node->parent, idx_in_parent, new_node->pg_no))
		{
			btree_mgr_delete_node(new_node);

			ret = -1;
			goto btree_mgr_div_node_end_;
		}

		/*
		* ��parent�����cell,�п��ܵ���parent���,���������cell��lchild��
		* ͬʱҪ����,Ҷ�ӽڵ��cell�����Ҷ�ӽڵ�����
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
				/* Ҷ�ӽڵ�����Ҷ�ӽڵ�����,Ҫ�����4���ֽ����ڴ洢ҳ�� */
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
			/* �Ƚ���Ӧ��cellת������ҳ,Ȼ��ɾ�� */

			if(idx_ovfl != i)
			{
				/* �������cell ovfl,��ԭʼҳ��ȡcellת�� */
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
				/* �����cell ovfl,��ovfl�Ĵ��� */
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
			/* ��ԭʼҳȥ����Ӧ��cell,����ȥ��cell ovfl��Ϣ */
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

		/* ������cell�ڷָ��֮ǰ */
		if(node->cell_ovfl.idx < idx_div)
		{
			/* �����cell ovfl,��ovfl�Ĵ��� */
			int ret = 0;

			assert(node->has_cell_ovfl && node->cell_ovfl.cell_buf);

			/* ע��˴�������֮ǰ��������cellinfo��Ч */
			ret = btree_mgr_eat_cell_ovfl(node);

			if(0 != ret)
			{
				btree_mgr_delete_node(new_node);

				ret = -1;
				goto btree_mgr_div_node_end_;
			}
		}

		assert(!node->has_cell_ovfl && !new_node->has_cell_ovfl);

		/* ����parent idx_in_parent + 1��Ӧcell��lchild */
		assert(idx_in_parent + 1 <= node->parent->ncell);

		assert(new_node->node_flag == node->node_flag);

		if(!(node->node_flag & e_btree_page_flag_leaf))
		{
			/* ���new_node��ΪҶ�ӽڵ�,����new_node�� rightmost Ϊnode��right most */
			unsigned int node_rmost = 0;
			btree_mgr_get_rmost(node, node_rmost);
			if(0 != btree_mgr_update_idx_lchild(new_node, new_node->ncell, node_rmost))
			{
				btree_mgr_delete_node(new_node);

				ret = -1;
				goto btree_mgr_div_node_end_;
			}
			/* ����node rightmost Ϊidx_div��lchild*/
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

		/* ȡ����node������ */
		btree_mgr_release_node(new_node);

#ifdef _DEBUG
/* ��һ��ƽ�������,��Ϊ������һ���ڵ�,���ü�������Ӧ������1 */
assert(PagerGetRefCount(node->btree_mgr->hpgr) <= (pgr_ref + 1));
#endif

	}
	else
	{
		/*
		* ����Ǹ��ڵ�
		* �������ڵ����,idx_divǰ��Ľڵ�ֱ���µ�����ҳ�ڵ�
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

		/* ��idx_div֮ǰ��ת����new_node1 */
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
				/* �����cell ovfl,��ovfl�Ĵ��� */
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

		/* ��idx_div֮���ת����new_node2 */
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
				/* �����cell ovfl,��ovfl�Ĵ��� */
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

		/* ����new_node2��right most */
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

			/* ����new_node1��right most */
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
			/* ���node��Ҷ�ڵ�,new_node2һ��ΪҶ�ڵ�,���ø���right most */
			assert(new_node1->node_flag & e_btree_page_flag_leaf);
			assert(new_node2->node_flag & e_btree_page_flag_leaf);
			assert(0 == div_lchild);
		}


		/* ��idx_div cell��������,Ȼ��node���,�ٽ�cell����node */
		/* ���idx_div������cell ovfl,��Ҫ��������node */
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

				/* ���node��ȻΪҶ�ӽڵ�,��Ҫ�����ĳɷ�Ҷ�ӽڵ� */
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

			/* node��0��lchildָ��new_node1,�����򿽻�node */
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

		/* node��right most ָ��new_node2 */
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

		/* ȡ����new_node1��new_node2������ */
		btree_mgr_release_node(new_node1);
		btree_mgr_release_node(new_node2);

#ifdef _DEBUG
/* �����������ڵ�,���ü�������Ӧ�������� */
assert(PagerGetRefCount(node->btree_mgr->hpgr) <= (pgr_ref + 2));
#endif
	}

btree_mgr_div_node_end_:

	assert(cell_array);
	MyMemPoolFree(btree_mgr->hm, cell_array);
	return 0;
}

/**
 * @brief �ݹ��ȡһ��b���ļ�¼����
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
 * @brief ������Ҫ�ϲ����߾���node1, node2, node1��node2�����ڵĽڵ���node1��node2"ǰ��"
 */
static __INLINE__ int btree_mgr_together_or_average_tow_nodes(btree_mgr_t * btree_mgr,
															  btree_node * node1, 
															  btree_node * node2,
															  unsigned int idx_node1_in_parent,
															  btree_node * parent)
{
	/* ���ھӴ���,����һ��,���������һ���ڵ������,��ͳ���һ���ڵ�,���������,�������� */

	/*
	* �Ƚ����ڵ�ϲ�,��ͬidx_in_parent,�γ�һ����Ľڵ�,Ȼ���������
	* ����ϲ���Ľڵ㳬����һ���ڵ�Ĵ�С,��ʱһ���ǿ��Ծ�������ʳ���40%������,��Ϊmax local������10%,ȥ10-%��parent,90+%���Ծ�������
	* ����ϲ���ڵ㲻����һ���ڵ�Ĵ�С,��������
	* ע��,�����ǰλ��Ҷ�ڵ���һ��,���ڵ�������cell������Ӧ������
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
			* ��������parentΪ���ڵ�����
			* ������ڵ�ֻʣһ��cell��
			* �ǽ�node1 node2�ϲ�������ڵ�
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
				/* ��ʱparent��ת���Ҷ�ӽڵ��� */
				unsigned char * cell = MyMemPoolMalloc(btree_mgr->hm, cell_parent_sz);

				assert((cell_info.cell_sz - BTREE_CELL_LCHILD_SZ) == cell_parent_sz);
				assert((cell_info.cell + BTREE_CELL_LCHILD_SZ) == cell_parent);

				memcpy(cell, cell_parent, cell_parent_sz);

				/* ��Ψһ��cellȥ�� */
				if(0 != btree_mgr_drop_cell(parent, 0, &cell_info))
				{
					MyMemPoolFree(btree_mgr->hm, cell);
					return -1;
				}

				/* �����ڵ��node_flag����Ҷ�ӽڵ�ı�ʶ */
				if(0 != btree_mgr_set_nodeflag(parent, (unsigned char)(parent->node_flag | e_btree_page_flag_leaf)))
				{
					MyMemPoolFree(btree_mgr->hm, cell);
					return -1;
				}

				/* ��������cell���� */
				if(0 != btree_mgr_insert_cell(parent, 0, cell, cell_parent_sz))
				{
					MyMemPoolFree(btree_mgr->hm, cell);
					return -1;
				}

				MyMemPoolFree(btree_mgr->hm, cell);

				/* right most��Ϊ0 */
				if(0 != btree_mgr_update_idx_lchild(parent, 1, 0))
					return -1;
			}

			/* ��node1���cellת�������ڵ� */
			assert(parent->node_flag == node1->node_flag);
			for(i = node1->ncell; i; i --)
			{
				btree_mgr_parse_idx(node1, i - 1, &cell_info);
				if(0 != btree_mgr_insert_cell(parent, 0, cell_info.cell, cell_info.cell_sz))
					return -1;
			}

			/* ��node2���cellת�������ڵ� */
			assert(parent->node_flag == node2->node_flag);
			for(i = 0; i < node2->ncell; i ++)
			{
				btree_mgr_parse_idx(node2, i, &cell_info);
				if(0 != btree_mgr_insert_cell(parent, parent->ncell, cell_info.cell, cell_info.cell_sz))
					return -1;
			}

			btree_mgr_ref_node(node1);
			btree_mgr_ref_node(node2);

			/* �ض�����ڵ�������ӽڵ��parentָ�� */
			btree_mgr_reparent_child(parent);

			/* ɾ��node1 node2 */
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
			/* ���Ժϲ���һ���ڵ� */
			unsigned int i;

			assert(parent->ncell > 1);

			/* �����ڵ��idx_node1_in_parent(���¼��pull down node)�����node2 */
			if(0 != btree_mgr_insert_cell(node2, 0, cell_parent, cell_parent_sz))
				return -1;
			assert(!node2->has_cell_ovfl && NULL == node2->cell_ovfl.cell_buf);

			if(!(node1->node_flag & e_btree_page_flag_leaf))
			{
				unsigned int node1_rmost = 0;

				/* pull down node��lchild����Ϊnode1��right most */
				btree_mgr_get_rmost(node1, node1_rmost);

				assert(!(node2->node_flag & e_btree_page_flag_leaf));
				if(0 != btree_mgr_update_idx_lchild(node2, 0, node1_rmost))
					return -1;
			}

			if(0 != btree_mgr_drop_cell(parent, idx_node1_in_parent, &cell_info))
				return -1;

			/* ��node1��ÿ��cell���ο�����node2 */
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

			/* ��Ϊ�������һ��release,�˴���node1���ü�����1,��ֹnode1����ǰ�ͷ��� */
			/* ɾ��lnode�ڵ� */
			btree_mgr_ref_node(node1);
			if(0 != btree_mgr_delete_node(node1))
				return -1;

			return 0;
		}
	}
	else if((BTREE_NODE_PAYLOAD(btree_mgr) - node1->nfree) > btree_mgr->pg_min_fill_sz)
	{
		/* ��Ҫ���������ڵ� */
		unsigned int div_fill_sz = BTREE_NODE_PAYLOAD(btree_mgr) - node2->nfree + btree_mgr_cal_cell_real_fill_sz(cell_parent_sz);
		/* node1�е��������aver_fill_sz���ٽ�cell������ */
		unsigned int div_idx = 0;
		unsigned int j;

        assert((BTREE_NODE_PAYLOAD(btree_mgr) - node2->nfree) < btree_mgr->pg_min_fill_sz);

		/* �����ڵ��idx_node1_in_parent(���¼��pull down node)����node2 */
		if(0 != btree_mgr_insert_cell(node2, 0, cell_parent, cell_parent_sz))
			return -1;
		assert(!node2->has_cell_ovfl && NULL == node2->cell_ovfl.cell_buf);

		if(!(node1->node_flag & e_btree_page_flag_leaf))
		{
			unsigned int node1_rmost = 0;

			assert(!(node2->node_flag & e_btree_page_flag_leaf));

			/* pull down node��lchild����Ϊnode1��right most */
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

		/* �ֽ���Ȼ��node1���ĳ��cell */
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

		/* ����node1��right mostΪdiv_idx��lchild */
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
				cell_sz = cell_info.cell_sz + BTREE_CELL_LCHILD_SZ;/* ���node1��Ҷ�ڵ�,��Ҫ�Խ���������cell����4���ֽڵ�lchild���λ */
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

			/* ����parent��idx_in_parent - 1���λ�õ�lchild */
			uint_to_big_endian(node1->pg_no, cell, BTREE_CELL_LCHILD_SZ);

			/* ��div_idx��Ӧ��cell����parent��idx_node1_in_parent���λ�� */
			if(0 != btree_mgr_insert_cell(parent, idx_node1_in_parent, cell, cell_sz))
			{
				MyMemPoolFree(btree_mgr->hm, cell);
				return -1;
			}

			MyMemPoolFree(btree_mgr->hm, cell);
		}

		/* ��node1��ȥ��div_dix���cell */
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
		/* ��Ҫ���������ڵ� */
		unsigned int div_fill_sz = BTREE_NODE_PAYLOAD(btree_mgr) - node1->nfree + btree_mgr_cal_cell_real_fill_sz(cell_parent_sz);
		/* lnode�е��������aver_fill_sz���ٽ�cell������ */
		unsigned int div_idx = 0;
		unsigned int j;

        assert((BTREE_NODE_PAYLOAD(btree_mgr) - node2->nfree) > btree_mgr->pg_min_fill_sz);

		/* �����ڵ��idx_node1_in_parent(���¼��pull down node)�����node */
		if(0 != btree_mgr_insert_cell(node1, node1->ncell, cell_parent, cell_parent_sz))
			return -1;
		assert(!node1->has_cell_ovfl && NULL == node1->cell_ovfl.cell_buf);

		if(!(node1->node_flag & e_btree_page_flag_leaf))
		{
			unsigned int node1_rmost = 0;

			/* pull down node��lchild����Ϊnode1��right most */
			btree_mgr_get_rmost(node1, node1_rmost);
			if(0 != btree_mgr_update_idx_lchild(node1, node1->ncell - 1, node1_rmost))
				return -1;
		}

		if(0 != btree_mgr_drop_cell(parent, idx_node1_in_parent, &cell_info))
			return -1;

		/* ��node2�ĵ�һ��cell��ʼ��ȥ */
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
			/* ��node2 div_idx֮ǰ��cell������node1δβ */
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
			/* ����node1��right mostΪdiv_idx��lchild */
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
				cell_sz = cell_info.cell_sz + BTREE_CELL_LCHILD_SZ;/* ���node1��Ҷ�ڵ�,��Ҫ�Խ���������cell����4���ֽڵ�lchild���λ */
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

			/* ����parent��idx_in_parent - 1���λ�õ�lchild */
			uint_to_big_endian(node1->pg_no, cell, BTREE_CELL_LCHILD_SZ);

			/* ��div_idxת����parent */
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
 * @brief ������ʲ���������,ͨ�����ڽڵ��cell,���ߺϲ��ڽڵ�,���������
 */
static int btree_mgr_fill_node(btree_node * node)
{
	/*
	* �����ǰ�Ľڵ������ʵ���40%
	* �������ھӽ�ڵ�,���ϲ�����һ������,���ھӽڵ��һ����¼���ϲ�(ע��,��ʱ�п��ܵ����ϲ�������).
	* ���û����,��ϲ����,���ϲ�ķָ��������ϲ���Ľڵ���ʺ�λ��
	*/
	
	int ret = 0;

	btree_mgr_t * btree_mgr = NULL;

	/* ��¼���ھӵ�ҳ��,������ڵĻ� */
	unsigned int lsibling = 0;
	/* ��¼���ھӵ�ҳ��,������ڵĻ� */
	unsigned int rsibling = 0;
	/* ��¼��ǰ�ڵ��ڸ��ڵ��е����� */
	unsigned int idx_in_parent = 0;

	/* ���ھ� */
	btree_node * rnode = NULL;
	/* ���ھ� */
	btree_node * lnode = NULL;

	btree_mgr = node->btree_mgr;

	/* �˽ڵ�����ʱ���С��40%,���Ҳ��Ǹ��ڵ�,��parent��Ϊ��,���ڵ������������ʵ���40%����� */
	assert(node && node->btree_mgr);
	assert(((BTREE_NODE_PAYLOAD(btree_mgr) - node->nfree) < btree_mgr->pg_min_fill_sz) && node->parent);
	assert(node->read_buf == PageHeadMakeWritable(node->pg));
	assert(node->parent->read_buf == PageHeadMakeWritable(node->parent->pg));
	assert(!(node->parent->node_flag & e_btree_page_flag_leaf));

	/* ���ҳ������ھ�,������һ������ */
	idx_in_parent = node->idx_in_parent_cellarray;
	if(node->parent->idx_changed)
		btree_mgr_check_idx_in_parent(node, &idx_in_parent);

	/* ����b���Ķ���,�ӷ�֧��������cell�ĸ���Ҫ��1 */
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
		/* ���ھӴ���,����һ��,���������һ���ڵ������,��ͳ���һ���ڵ�,���������,�������� */

		/*
		* �Ƚ����ڵ�ϲ�,��ͬidx_in_parent,�γ�һ����Ľڵ�,Ȼ���������
		* ����ϲ���Ľڵ㳬����һ���ڵ�Ĵ�С,��ʱһ���ǿ��Ծ�������ʳ���40%������,��Ϊmax local������10%,ȥ10-%��parent,90+%���Ծ�������
		* ����ϲ���ڵ㲻����һ���ڵ�Ĵ�С,��������
		* ע��,�����ǰλ��Ҷ�ڵ���һ��,���ڵ�������cell������Ӧ������
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
		/* ���ھӴ���,����һ��,���������һ���ڵ������,��ͳ���һ���ڵ�,���������,�������� */
		/* �ⲿ�ִ����߼���lnode��Ϊ��ʱ��ͬ */

		/*
		* �Ƚ����ڵ�� ?��ͬidx_in_parent,�γ�һ����Ľڵ�,Ȼ���������
		* ����ϲ���Ľڵ㳬����һ���ڵ�Ĵ�.��ʱһ���ǿ��Ծ�������ʳ���40%������,��Ϊmax local������10%, ?0-%��parent,90+%���Ծ�������
		* ����ϲ���ڵ㲻����һ���ڵ�Ĵ�С,��������
		* ע��,�����ǰλ��Ҷ�ڵ���һ��,���ڵ�������cell������Ӧ������
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
 * @brief ƽ��b��
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
			/* ������ڵ�,�����������ʵ͵�40%����� */
			if(0 != btree_mgr_fill_node(node))
				return -1;

			need_continue = 1;
		}

		assert((parent && PagerGetPageRefCount(parent->pg)) || NULL == parent);

		/* �����ǰ�ڵ��û�����,Ҳû�г�������ʲ�������,��ƽ�⹤���Ѿ������ */
		if(!need_continue)
			break;

		/* �ݹ�����,ֱ����ǰ�ڵ�û�������������ʵ͵�������� */
		node = parent;
		need_continue = 0;
	}

	return 0;
}

/**
 * @brief ��Ӽ�ֵ
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

	/* ����ҵ����ظ���ֵ,���ʧ�� */
	if(0 != btree_mgr_search(cursor, key, key_sz, &res))
		return -1;

	if(0 == res)
		return -1;

	/* �����ɾ�����Ƿ�����Ҷ�ӽڵ㴦 */
	assert(cursor->cur_node->node_flag & e_btree_page_flag_leaf);

	/* ����µļ�¼���α�������λ�� */
	btree_mgr_fill_cellinfo(cursor->cur_node, key_sz, data_sz, &cell_info);
	assert(key_sz == cell_info.key_sz && data_sz == cell_info.data_sz);

	cell = MyMemPoolMalloc(cursor->btree_mgr->hm, cell_info.cell_sz);
	if(NULL == cell)
		return -1;

	/* ���cell */
	if(0 != btree_mgr_pack_cell(cursor->cur_node, &cell_info,
		cell, cell_info.cell_sz,
		key, key_sz,
		data, data_sz))
	{
		ret = -1;
		goto btree_mgr_add_end_;
	}

	/* ��������cell��ӵ�ָ����λ�� */
	if(0 != btree_mgr_insert_cell(cursor->cur_node, cursor->idx, cell, cell_info.cell_sz))
	{
		ret = -1;
		goto btree_mgr_add_end_;
	}

	/* ����balanceƽ��b�� */
	if(0 != btree_mgr_balance(cursor->cur_node))
	{
		ret = -1;
		goto btree_mgr_add_end_;
	}

btree_mgr_add_end_:

	assert(cell);
	MyMemPoolFree(cursor->btree_mgr->hm, cell);

	/* ���α��ƶ������ڵ� */
	btree_mgr_move_to_root(cursor);

	return ret;
}

/**
 * @brief ��Ӽ�ֵ
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
 * @brief �����α�
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
 * @brief Ѱ���Һ���
 */
#define btree_mgr_get_idx_rchild(_n, _i, _rchild) do{\
		assert(_n && _i < _n->ncell);\
		if(_i + 1 < _n->ncell) \
			btree_mgr_get_idx_lchild(_n, _i + 1, _rchild);\
		else \
			btree_mgr_get_rmost(_n, _rchild);\
	}while(0)

/**
 * @brief ��Ѱ��ǰ�α����һ��cell
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
 * @brief ɾ����ֵ
 */
static __INLINE__ int btree_mgr_del(btree_cursor * cursor,
									const void * key,
									const unsigned int key_sz)
{
	/*
	* �Ȱ�ָ��λ�õĶ���ɾ����,�ٰ��油cell��ӽ�ȥ
	* �͵�ǰ����ڵ���һ��ƽ��
	* ���油cell,һ����λ��Ҷ�ӽڵ�,ɾ��,����һ��ƽ�����
	*/
	int ret = -1;
	btree_cursor * temp_cursor = NULL;
	unsigned char * cell = NULL;
	btree_cellinfo cell_info;
	unsigned int lchild = 0;
	int res = 0;

	assert(cursor && cursor->btree_mgr && cursor->root_pgno);

	assert(cursor->btree_mgr == cursor->cur_node->btree_mgr);

	/* ���Ҽ�ֵ�Ƿ����,������ɾ��ʧ�� */
	if(0 != btree_mgr_search(cursor, key, key_sz, &res))
		return -1;

	if(res)
	{
		ret = -1;
		goto btree_mgr_del_end_;
	}

	/* �����ǰ�ڵ���Ҷ�ڵ�,ֱ����ɾ������ */
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
	* �������Ҷ�ڵ�,����Ҫ����ת��
	* �α��ߵ�"��һ��"cell��λ��
	*/
	temp_cursor = btree_mgr_duplicate_cursor(cursor);
	if(NULL == temp_cursor)
		return -1;

	if(0 != btree_mgr_next(temp_cursor))
	{
		ret = -1;
		goto btree_mgr_del_end_;
	}

	/* �ڲ��ڵ����һ��(���¼��next cell)��Ȼλ��Ҷ�ڵ㴦,����λ��������Ϊ0 */
	assert(temp_cursor->cur_node->node_flag & e_btree_page_flag_leaf);
	assert(0 == temp_cursor->idx);

	/* ����ǰλ�õ�ֵȥ��,����next cell���������� */
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

	/* ��next cellɾ��,����ƽ����� */
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
 * @brief ɾ����ֵ
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
 * @brief ���Ҽ�ֵ
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
 * @brief ��ȡ�α굱ǰ���ڼ�¼��key
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
 * @brief ��ȡ�α굱ǰ���ڼ�¼��data
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
 * @brief �ݹ�ɾ��node�Լ����������ӽڵ�
 * @param bdel_self:�Ƿ���Լ�Ҳɾ��
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
		/* �������Ҷ�ڵ�,���ÿ���ӷ�֧���еݹ� */
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

	/* ɾ�����е����ҳ,��ɾ�����ڵ㼴�� */
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
 * @brief ���α���ָ�ı�������м�¼ȫ��ɾ��
 */
static __INLINE__ int btree_mgr_clear(btree_cursor * cursor)
{
	if(0 != btree_mgr_move_to_root(cursor))
		return -1;

	return btree_mgr_clear_node_and_sub(cursor->cur_node, 0);
}

/**
 * @brief ���α���ָ�ı�������м�¼ȫ��ɾ��
 */
int btreeMgrClear(HBTREE_CURSOR hcur)
{
	if(NULL == hcur || NULL == hcur->btree_mgr)
		return -1;

	return btree_mgr_clear(hcur);
}

/**
 * @brief ��ȡ�������α�,������¼����b��������������b���ĸ��ڵ�
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
 * @brief ����һ����
 * @param hcur:����Ϊ��,�����ɹ�����hcur��root_pg��ֵ
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

	/* ������master(����������)������� */
	if(0 != btree_mgr_move_to_root(hcur_master))
		return -1;

	/* ����ҵ�,�򴴽�ʧ�� */
	if(0 != btree_mgr_search(hcur_master, table_id, table_id_sz, &res))
		return -1;

	if(0 == res)
		return -1;

	/* �Ҳ���,���û��Ľ�����Ϣ������������,������root_pgno */
	pgno_tlb_root = PagerGetPageNo(hcur_master->btree_mgr->hpgr);
	if(0 == pgno_tlb_root)
		return -1;

	*phcur = btree_mgr_alloc_cursor(hcur_master->btree_mgr, cmp, context, context_sz);
	if(NULL == *phcur)
		goto btreeMgrCreateTable_err_;

	/* ��root_pgno����hcur,�����ظ��ڵ�ҳ���� */
	(*phcur)->root_pgno = pgno_tlb_root;
	(*phcur)->cur_node = btree_mgr_get_node(hcur_master->btree_mgr, pgno_tlb_root, NULL, 0);
	assert(pgno_tlb_root == (*phcur)->cur_node->pg_no);
	if(NULL == (*phcur)->cur_node)
		goto btreeMgrCreateTable_err_;

	if(0 != btree_mgr_set_nodeflag((*phcur)->cur_node, (unsigned char)(flag | e_btree_page_flag_leaf)))
		goto btreeMgrCreateTable_err_;

	/* ����ǰ����Ϣ,��ĸ��ڵ�ҳ����ӵ������������� */
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
 * @brief ��ȡָ����ĸ��ڵ�
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

	/* ȡ��hcur_master��,��ǰ��������Ϣ */
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
 * @brief ��һ����
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

	/* ȡ��hcur_master��,��ǰ��������Ϣ */
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
 * @brief ��һ����
 * @param hcur:����Ϊ��,�򿪳ɹ�����hcur��root_pg��ֵ
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
 * @brief ɾ��һ����/����
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
 * @brief ���α��ƶ������ұߵ��Ǹ���¼
 */
static __INLINE__ int btree_mgr_move_to_right_most(btree_cursor * cursor)
{
	btree_node * sub = NULL;
	unsigned int pgno_sub = 0;

	assert(cursor && cursor->btree_mgr);

	/* ��cur�ƶ������ұ� */
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
 * @brief ��ȡһ�������rowid
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

	/* �����ǰ����rowidû�дﵽ 0xffffffff */
	if(cell_info.key_sz < ((unsigned int)-1))
		*prowid = cell_info.key_sz + 1;

	/* ����ﵽ�� */

	while(loop)
	{
		/* ��ʱ�������������㷨 */
		*prowid = rand();

		if(0 != btree_mgr_search(hcur, NULL, *prowid, &res))
			return -1;

		/* �����ǰ���ɵ����rowidû���ظ�,���� */
		if(0 != res)
			return 0;

		loop--;
	}

	/* ����1000�κ�,��Ȼ���ظ���,����Ϊʧ�� */
	return -1;
}

/**
 * @brief �ύ����
 */
int btreeMgrCommit(HBTREE_MGR hbtreeMgr)
{
	if(NULL == hbtreeMgr || NULL == hbtreeMgr->hpgr)
		return -1;

	return PagerSyn(hbtreeMgr->hpgr);
}

/**
 * @brief �ع�
 */
int btreeMgrRollBack(HBTREE_MGR hbtreeMgr)
{
	if(NULL == hbtreeMgr || NULL == hbtreeMgr->hpgr)
		return -1;

	return PagerRollBack(hbtreeMgr->hpgr);
}

/**
 * @brief ��ȡһ��b���ļ�¼����
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
 * @brief ��ȡ���ҳ�ļ��ж���ҳ,�Լ��ж��ٿ���ҳ
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
 * @brief ��ȡҳ�������ü���
 */
int btreeMgrGetPagerRefCount(HBTREE_MGR hbtreeMgr)
{
	if(NULL == hbtreeMgr || NULL == hbtreeMgr->hpgr)
		return 0;

	return PagerGetRefCount(hbtreeMgr->hpgr);
}


/**
 * @brief ���һ��node�Ƿ����ź���
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
 * @brief ���һ��cell��ovfl��(����еĻ�)�Ƿ�Ϸ�
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
 * @brief ���ڵ���������Ƿ�Ϸ�
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

	/* �鿴ÿ��cell */
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

			/* ���ڲ��Գ����ǰ�ĸ��ֽ�����abcd,�Ǻ� */
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
			/* ��һ�����п���cell_content��ʼ,Ӧ������frag�����ֵ */
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

	/* �鿴���п� */
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

		/* ˳�ſ���������ͳ�� */
		while(free_buf)
		{
			unsigned int sz = 0;
			unsigned int next_free = 0;

			get_2byte(&(node->read_buf[free_buf + BTREE_FREE_BLOCK_SZ_OFFSET]), sizeof(unsigned short), sz);
			nfree += sz;

			get_2byte(&(node->read_buf[free_buf + BTREE_FREE_BLOCK_NEXT_OFFSET]), BTREE_CELL_PTR_SIZE, next_free);

			/* ���п鰴ƫ�Ƶ���������,���Ҳ���������(�ͷ�ʱҪ����ϲ��������п�����) */
			assert(next_free > free_buf + BTREE_FREE_BLOCK_MIN_SZ || 0 == next_free);

			free_buf = next_free;
		}

		/* �ܴ�СӦ�úϷ� */
		assert((nfree + total_cell_sz + BTREE_CELL_PTR_SIZE * node->ncell + BTREE_CELL_PTR_OFFSET) == node->btree_mgr->pg_sz);

		/* �ڴ滺��Ŀ��пռ�Ӧ������������� */
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
 * @brief ���һ��b���Ƿ�Ϸ�
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

	/* ��ǰ�ڵ��һЩ����У�� */
	examin_node(cursor, node, node_count);

	/* cell�������� */
	examin_sort_ok(cursor, node);

	/* ����ʱ������40%,������cell_ovfl */
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
		/* ÿ����ǰ��֧�����ֵ����Сֵ��������b���Ķ��� */
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

		/* ÿ���ӷ�֧�Ĳ���������ͬ */
		if(0 == i)
			sub_layer = cur_sub_layer;
		else
			assert(cur_sub_layer == sub_layer);
	}

	return sub_layer + 1;
}

/**
 * @brief ���һ��b���Ƿ�Ϸ�
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
 * @brief ���һ��b���Ƿ�Ϸ�
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















/**
 * @file page.c page cache management 2008-1-29 23:59
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it. 
 *        ���𻺴�page����,journal���Ʊ�֤���ݵ�������, ������ҳ����
 *
 * ���ҳ�ļ���ʽ����
 * ��һҳ
 *  offset bytes        des
 *  0      16           ��ʽ�汾
 *  16     4            ҳ��С
 *  20     4            ��һ������ҳ
 *  24     40           ����
 *  64   pg_sz - 64     �û�����     
 *
 * ����ҳ:
 * ��һ������ҳ
 * offset bytes   des
 * 0      4       �ܵĿ���ҳ��
 * 4      4       ����ҳ"����"����һ������ҳ��
 * 8      4       ��ǰҳ������˶��ٵĿ���ҳ��
 * 12             �������鿪ʼ,ÿ4���ֽڱ�ʾһ������ҳ��
 * �ǵ�һ������ҳ
 * 0      4       ������
 * 4      4       ����ҳ"����"����һ������ҳ��
 * 8      4       ��ǰҳ������˶��ٵĿ���ҳ��
 * 12             �������鿪ʼ,ÿ4���ֽڱ�ʾһ������ҳ��
 *
 * ҳ:
 * offset bytes      des
 * 0      1          1:��ʾ��ҳ��ʹ��,0:��ʾ��ҳ����
 * 1      3          ����
 * 4      pg_sz - 4  ���û�ʹ�õĿռ�
 *
 * journal��־�ļ�ͷ����
 * offset bytes          des
 * 0      4              ͷ�Ĵ�Сhd_sz
 * 4      16             ��ʽ�汾
 * 20     4              ��־�ﱸ���˼�ҳ
 * 24     4              У��ͳ�ʼ���ֵ
 * 28     4              ������־ǰҳ�ļ��Ĵ�С
 * 32     hd_sz-24       ����
 * 
 * journal��¼:
 * <ҳ���� + ҳ�� + У���>
 *
 * ��֤:os�Ƿ��ǰ���д�ļ�,syncʱ�Ƿ����ͬ��,
 * seek:win��seekһ���ɹ�,��ʹ��seek��λ�ó������ļ��Ĵ�С,��д��ʱ,������̿ռ䲻��(seek��λ��̫����),�����ʾ���̿ռ䲻��(errcode = 112)
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


/* Ĭ�ϵ���־�ļ����� */
#define JOURNAL "-journal"

/* ҳ�ļ���ʽ�汾 */
#define VERSION "Rhapsody V0.1.7\000"

/* ��С�Լ�����ҳ�ļ���С */
#define	MIN_PAGE_SIZE 64
#define MAX_PAGE_SIZE 32768

/* Ĭ�ϵ�ҳ�������� */
#define DEFAULT_CACHE_PAGES_COUNT 8

/* ��־�汾���� */
#define JOURNAL_MAGIC_STRING "1234567890123456"

/* ҳ�ļ��ĵ�һҳ */
#define PAGER_FIRST_PAGE_NO 1


/**
 * ���ҳ�ļ���ʽ����
 * ��һҳ
 *  offset bytes        des
 *  0      16           ��ʽ�汾
 *  16     4            ҳ��С
 *  20     4            ��һ������ҳ
 *  24     40           ����
 *  64   pg_sz - 64     �û�����
 */
typedef struct __unused_pager_info_
{
	unsigned char ver[16];
	unsigned char page_size[4];
	unsigned char first_free[4];
	unsigned char reserve[40];
}unused_pager_info;

/* ��¼ver��ƫ�� */
#define PAGER_HDR_VER_OFFSET (((unused_pager_info *)0)->ver - (unsigned char *)0)
/* ��¼ver�Ĵ�С */
#define PAGER_HDR_VER_LEN (sizeof(((unused_pager_info *)0)->ver))
/* page_size��ƫ�� */
#define PAGER_HDR_PAGE_SIZE_OFFSET (((unused_pager_info *)0)->page_size - (unsigned char *)0)
/* ��¼��һ������ҳҳ�ŵ�ƫ���� */
#define PAGER_HDR_FIRST_FREE_OFFSET (((unused_pager_info *)0)->first_free - (unsigned char *)0)
/* ��¼��һ������ҳҳ�ŵ����ռ䳤�� */
#define PAGER_HDR_FIRST_FREE_LEN (sizeof(((unused_pager_info *)0)->first_free))

/**
 * �ж����������ҳ�Ƿ�������ӿ���ҳ
 *
 * ����ҳ:
 * ��һ������ҳ
 * offset bytes   des
 * 0      4       �ܵĿ���ҳ��
 * 4      4       ����ҳ"����"����һ������ҳ��
 * 8      4       ��ǰҳ������˶��ٵĿ���ҳ��
 * 12             �������鿪ʼ,ÿ4���ֽڱ�ʾһ������ҳ��
 * �ǵ�һ������ҳ
 * 0      4       ������
 * 4      4       ����ҳ"����"����һ������ҳ��
 * 8      4       ��ǰҳ������˶��ٵĿ���ҳ��
 * 12             �������鿪ʼ,ÿ4���ֽڱ�ʾһ������ҳ��
 */
typedef struct __unused_free_page_
{
	unsigned char total_free_count[4];
	unsigned char next_free_link[4];
	unsigned char free_count[4];
	unsigned char free_pgno_array[1];
}unused_free_page;

/* �洢��ҳ����ƫ�� */
#define PAGER_FREE_TOTAL_FREE_COUNT_OFFSET (((unused_free_page *)NULL)->total_free_count - (unsigned char *)NULL)
/* ��һ������ҳ�������ӵ�ƫ�� */
#define PAGER_FREE_NEXT_FREE_LINK_OFFSET (((unused_free_page *)NULL)->next_free_link - (unsigned char *)NULL)
/* �洢��ҳ����ҳ����ƫ�� */
#define PAGER_FREE_FREE_COUNT_OFFSET (((unused_free_page *)NULL)->free_count - (unsigned char *)NULL)
/* ����ҳ�����ƫ�� */
#define PAGER_FREE_FREE_PGNO_ARRAY_OFFSET (((unused_free_page *)NULL)->free_pgno_array - (unsigned char *)NULL)

/**
 * ÿҳ���ڹ���Ŀռ�ƫ�ƶ���
 * 0      1          1:��ʾ��ҳ��ʹ��,0:��ʾ��ҳ����
 * 1      3          ����
 * 4      pg_sz - 4  ���û�ʹ�õĿռ�
 */
typedef struct __unused_page_
{
	unsigned char in_user[1];
	unsigned char reserve[3];
}unused_page;

/* ÿһҳΪ�˹����Ԥ�����ֽ� */
#define BYTE_RESERVE_PER_PAGE sizeof(unused_page)
#define PAGER_PAGE_USER_SIZE(__pgr) (__pgr->page_size - BYTE_RESERVE_PER_PAGE)
/* ��ʶҳ����ʹ���еı���λ��ƫ�� */
#define PAGER_PAGE_IN_USE_OFFSET (((unused_page*)NULL)->in_user - (unsigned char *)NULL)

/**
 * journal��־�ļ�ͷ����
 * offset bytes          des
 * 0      4              ͷ�Ĵ�Сhd_sz
 * 4      16             ��ʽ�汾
 * 20     4              ��־�ﱸ���˼�ҳ
 * 24     4              У��ͳ�ʼ���ֵ
 * 28     4              ������־ǰҳ�ļ��Ĵ�С
 * 32     hd_sz-24       ����
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

/* Ĭ�ϵ�������С */
#define PAGER_MIN_SECTOR_SIZE (sizeof(unused_journal_hdr))
#define JOURNAL_MAGIC_LEN (sizeof(((unused_journal_hdr *)0)->magic))
#define PAGER_JOURNAL_REC_COUNT_OFFSET (((unused_journal_hdr *)0)->rec_count - (unsigned char *)0)
#define PAGER_JOURNAL_REC_COUNT_LEN (sizeof(((unused_journal_hdr *)0)->rec_count))


struct __pghd_t_;

typedef struct __pager_t_
{
	/* �ڴ�ؾ�� */
	HMYMEMPOOL hm;

	/* �����ļ��� */
	HMYBUFFER hb_file_name;
	/* �����ҳ�ļ���� */
	HOSFILE hf;
	/* ҳ�ļ��������Ƿ����� */
	int page_file_is_integrity;
	/* ���pager����������õĴ��� */
	unsigned int ref_count;

	/* journal�ļ��� */
	HMYBUFFER hb_jf_name;
	/* journal �ļ���� */
	HOSFILE hjf;
	/* ��ǰ��־�ļ���β�� */
	int64 journal_end;
	/* ��ǰ��־�ļ������һ��ͷƫ��(��δͬ��) */
	int64 jouranl_prev_hdr;
	/*
	* У��ͳ�ʼ���������
	* ÿһ��������־ͷ,����¼һ�������,���ڼ����Ӧ�����ͷ��ҳ�ļ����
	* ��ֹ��Ϊ�ϵ������������
	* (���������п�����һ����ɾ������ȷ��journal�ļ�,����,��ʹ�ļ��Ǵ����,��У���ȴ�п�������ȷ��)
	* ��������������,����������������ķ���
	* ÿ��jfͷ��У��ͳ�ʼ���ֵ���ǲ�һ����.
	*/
	unsigned int cur_cksum_init;
	/* ����������� */
	HMYRAND hrand;

	/* ��¼����ҳ����,���մ��������ҳʱ,����ͬ��jfd,�Ա�֤���ݵ������� */
	struct __pghd_t_ * pg_free_first, * pg_free_last;

	/*
	* ��¼�Ѿ�ͬ����jf�Ŀ���ҳ����,����ʱ,���Ȼ�������������ҳ 
	* ����ȡ��ͷҳ����ʱ,�����ҳ�������������ͷ,��Ӧ��������ͷ����ҳ����
	*/
	struct __pghd_t_ * pg_syncjf_first, * pg_syncjf_last;

	/* �Ƿ���Ҫ����jfdͬ���Ĳ���,����ʱ��Ҫ�˱�ʶ�ó�0 */
	int need_sync_journal;
	/*
	* �Ƿ��Ѿ�����jfͬ������
	* �����жϵ����ֳ���ԭʼ��Сҳд�����ʱ,�Ƿ���Ҫ��ͬ��jf�Ĳ���
	* ���ͬ��jf�Ĳ����Ѿ�������,���ʱ����������
	*/
	int bsync_journal_has_done;

	/* ��¼�ж���ҳ����jfd����,����δͬ��jf����ҳ */
	unsigned int in_journal_pages_count;

	/* ��¼ҳ�Ƿ�journal״̬��λ��ɫ�� */
	HMYHASHTABLE hash_pgno_journal;

	/* ��¼���е�ҳ */
	struct __pghd_t_ * pg_all;

	/* ��¼��ҳ,ͬ��ʱ,����Щҳ��ͬ����jfd,Ȼ��д��fd��ͬ�� */
	struct __pghd_t_ * pg_dirty;

	/* �ļ�ҳ����ҳ�滺��Ĺ�ϣӳ��� <pgno> - <pghd_t *> */
	HMYHASHTABLE hash_page;

	/* ��¼����ļ��ж���ҳ */
	unsigned int total_page_count;
	/* ��¼��ʼ����jfʱҳ�ļ��ж���ҳ */
	unsigned int orig_pages_count;

	/* �����е����ҳ�� */
	unsigned int max_cache_pages;

	/* ÿҳ��С */
	unsigned int page_size;

	/*
	* ����ϵͳ��������С,����jfͷ�Ĵ�С
	* ��������ع�����,���ֵ��������
	*/
	unsigned int sector_size;

	/* ÿҳ���ӵ��û���չ���ݴ�С */
	unsigned int extra_size;

	/* ҳ�ļ�����ȡ�������Լ��ƶ�ʱ�Ļص����� */
	PAGE_RELEASE page_release_cb;
	PAGE_RELOAD page_reload_cb;
}pager_t;

typedef struct __pghd_t_
{
	/* which pager this page belong to */
	pager_t * pager;

	/* ҳ�� */
	unsigned int page_num;

	/* ҳ�������ü��� */
	unsigned int ref_count;

	/* ��¼��ǰҳ�����״̬ */
	int pg_state;

	/* ǰһ�����һ������ҳ,��ҳ�����ü���Ϊ0ʱ,Ӧ����������� */
	struct __pghd_t_ * pg_free_prev, * pg_free_next;

	/*
	* ��¼�Ѿ�ͬ����jf�Ŀ���ҳ����,����ʱ,���Ȼ�������������ҳ 
	* ����ȡ��ͷҳ����ʱ,�����ҳ�������������ͷ,��Ӧ��������ͷ����ҳ����
	* ��ҳ�����ü���Ϊ��ʱ,�����Ѿ�ͬ����hjf,Ӧ���������
	*/
	struct __pghd_t_ * pg_syncjf_prev, * pg_syncjf_next;

	/* ��һ����ҳ,����һ����ҳ�������� */
	struct __pghd_t_ * pg_dirty_prev, * pg_dirty_next;

	/* ��һ��ҳ,����һ������ҳ������ */
	struct __pghd_t_ * pg_all_prev, * pg_all_next;

	/*
	* ����ҳ�����Ԥ����һЩ�ֽ�
	* ��һ���ֽڱ�ʾ��ҳ�Ƿ�Ϊ����ҳ
	*/
	unsigned char pg_buf_start[BYTE_RESERVE_PER_PAGE];
}pghd_t;

typedef enum __page_event_
{
	/* ������ҳ����,����jf�������� */
	PAGE_EVT_WRITE_BUF_BK,

	/* ҳ���汻д��,��ͬ����jf */
	PAGE_EVT_WRITE_BACK_SYNCJF,

	/* ҳ���汻ͬ���� */
	PAGE_EVT_SYNC,

	/* ҳ����ع��� */
	PAGE_EVT_ROLL_BACK,

	/* jf�ļ���ͬ���� */
	PAGE_EVT_SYNC_JOURNAL,

	/* jf�ļ���ɾ�� */
	PAGE_EVT_DEL_JOURNAL,

	EVENT_END,
}PAGE_EVENT;

typedef enum __page_state_
{
	/* ҳ���汻����,��û�ж�ҳ�������κ��޸� */
	PAGE_CLEAN,

	/* ��ҳ���������޸�,����jf���˱���,��û��д��ҳ�ļ� */
	PAGE_DIRTY_NO_SYNC_JF,

	/* ��ҳ����������Ѿ�д��ҳ�ļ�,��jf�ﱸ���Ѿ����� */
	PAGE_CLEAN_SYNC,

	/* ҳ����д��ҳ�ļ���,�ֶ�ҳ����������޸�,jf��ı����Ѿ����� */
	PAGE_DIRTY_SYNC_JF,

	/* ҳ����һ�������״̬ */
	PAGE_STATE_ERR,
}PAGE_STATE;

/**
 * @brief ��ĳһ��ҳ�����óɷ�dirty״̬,����dirty����������
 *
 * ҳ����״̬��Ǩ��
 *---------------------------------------------------------------------------------------------------------------------------------------------------------------------
 *| ״̬\�¼�            |PAGE_EVT_WRITE_BUF_BK  |PAGE_EVT_WRITE_BACK_SYNCJF | PAGE_EVT_SYNC       | PAGE_EVT_ROLL_BACK | PAGE_EVT_SYNC_JOURNAL| PAGE_EVT_DEL_JOURNAL |
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
 * ҳ����״̬����Ǩͼ              
 *                          syn/roll back                    syn jf
 *                       ___________________     ________________________________________________________________
 *              ��̬  <-/                   \   /                                                                \->
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
 * @brief �ж�һ��ҳ�Ƿ���journal����֮��
 */
static __INLINE__ int pager_page_in_journal(pager_t * pager, unsigned int pgno)
{
	assert(pager && pager->hash_pgno_journal);

	if(MyHashTableSearch(pager->hash_pgno_journal, (void *)pgno))
		return 1;

	return 0;
}

/**
 * @brief �ж�ĳһ���Ƿ�Ϊ��
 */
#define pager_page_is_dirty(__pg) (PAGE_DIRTY_SYNC_JF == __pg->pg_state || PAGE_DIRTY_NO_SYNC_JF == __pg->pg_state)

/**
 * @brief �Ƿ���journal�����˱���
 */
#define pager_page_is_in_journal(__pg) (assert(pager_page_in_journal(__pg->pager, __pg->page_num) || PAGE_CLEAN == __pg->pg_state), PAGE_CLEAN != __pg->pg_state)

/**
 * @brief �Ƿ�����journalͬ��
 */
#define pager_page_is_sync_journal(__pg) (PAGE_CLEAN_SYNC == __pg->pg_state || PAGE_DIRTY_SYNC_JF == __pg->pg_state)

/**
 * @brief �ж�ҳ�����Ƿ����ύǰд����ҳ�ļ�
 */
#define pager_page_is_written_before_page_sync(__pg) pager_page_is_sync_journal(__pg)

/**
 * @brief �ж�ҳ�����Ƿ����ύǰд����ҳ�ļ�
 */
#define pager_page_is_clean_not_in_jf(__pg) (PAGE_CLEAN == __pg->pg_state)

/**
 * @brief ��ȡ�ж���ҳ������
 */
#define pager_get_cached_pages_count(__pgr) MyHashTableGetElementCount(__pgr->hash_page)

/**
 * @brief ����jf��־��¼�ĳ���
 */
#define pager_get_journal_rec_len(__pgr) (__pgr->page_size + sizeof(unsigned int) + sizeof(unsigned int))

/**
 * @brief дһ���޷������������ļ�
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
 * @brief ���ļ��ж���һ��������
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
 * @brief ��ҳ�����ϣ���в���
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
 * @brief �ж�ĳһ���Ƿ���free_list
 */
#define pager_page_is_in_free_list(__pg) (__pg->pg_free_prev || __pg->pg_free_next || __pg == __pg->pager->pg_free_first)

/**
 * @brief �ж�ĳһ���Ƿ���sync_jf list
 */
#define pager_page_is_in_syncjf_list(__pg) (__pg->pg_syncjf_next || __pg->pg_syncjf_prev || __pg == __pg->pager->pg_syncjf_first)

/**
 * @brief �ж�ĳһ���Ƿ���dirty list��
 */
#define pager_page_is_in_dirty_list(__pg) (__pg->pg_dirty_next || __pg->pg_dirty_prev || __pg == __pg->pager->pg_dirty)

/**
 * @brief �ж�ĳһ���Ƿ������е�ҳ����������
 */
#define pager_page_is_in_all_list(__pg) (__pg->pg_all_next || __pg->pg_all_prev || __pg == __pg->pager->pg_all)

/**
 * @brief ��ĳһ��ҳ�����óɷ�dirty״̬,����dirty����������
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
 * @brief ��ĳ��ҳ�������dirty list
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
 * @brief ��ĳһ��ҳ�����ó�dirty,������dirty����
 */
static __INLINE__ int pager_make_page_dirty(pager_t * pager, pghd_t * pg, PAGE_EVENT evt)
{
	assert(!pager_page_is_dirty(pg));

	/* �޸�ҳ״̬ */
	pager_change_page_state(pg, evt);

	pager_add_page_to_dirty_list(pager, pg);

	return 0;
}

/**
 * @brief ��sync jf list������
 */
static __INLINE__ int pager_out_syncjf_list(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);
	assert(0 == pg->ref_count);

	if(pg == pager->pg_syncjf_first)
		pager->pg_syncjf_first = pg->pg_syncjf_next;

	if(pg == pager->pg_syncjf_last)
		pager->pg_syncjf_last = pg->pg_syncjf_prev;

	/* ����ͬ������־������������ */
	if(pg->pg_syncjf_prev)
		pg->pg_syncjf_prev->pg_syncjf_next = pg->pg_syncjf_next;

	if(pg->pg_syncjf_next)
		pg->pg_syncjf_next->pg_syncjf_prev = pg->pg_syncjf_prev;

	pg->pg_syncjf_prev = NULL;
	pg->pg_syncjf_next = NULL;

	return 0;
}

/**
 * @brief ��free list������,������ͬ��jfҳ��������������
 */
static __INLINE__ int pager_out_free_list(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);
	assert(0 == pg->ref_count);

	if(pg == pager->pg_free_first)
		pager->pg_free_first = pg->pg_free_next;

	if(pg == pager->pg_free_last)
		pager->pg_free_last = pg->pg_free_prev;

	/* �ӿ���ҳ�������������� */
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
 * @brief ���뵽����ҳ��������δβ
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
 * @brief ��ĳ��ҳ������뵽��ͬ����jf������δβ,�����뵽����ҳ��������δβ
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

	/* ���뵽����ҳ�������� */
	if(!pager_page_is_in_free_list(pg))
		pager_add_to_free_list(pager, pg);

	assert(pager_page_is_in_free_list(pg));

	return 0;
}

/**
 * @brief ��ĳ��ҳ�������ҳ�����ϣ����ܵ�ҳ��������
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
 * @brief ��ĳ��ҳ�����ҳ�����ϣ����ܵ�ҳ������������
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
 * @brief ����ָ��ҳ�����ü���
 */
static __INLINE__ int pager_ref_page(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg);
	assert(pg->pager == pager);

	/* ������ü�����,��ӿ��������г�ȥ,���ü�����1 */
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
 * @brief ��ʼ��ҳ����
 */
static __INLINE__ int pager_init_page(pager_t * pager, pghd_t * pg, unsigned int pgno)
{
	assert(pager && pg && pgno != 0);

	memset(pg, 0, sizeof(*pg));

	/* ����injournal��ɫͼ��ʼ����ر�ʶ */
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
 * @brief ����һ��ҳ����
 */
static __INLINE__ pghd_t * pager_allocate_page(pager_t * pager, unsigned int pgno)
{
	/* ҳͷ��Ϣ�洢�ռ� + ҳ����ռ� + �û����ݿռ� + ҳ����У��ʹ洢�ռ� */
	pghd_t * pg = (pghd_t *)MyMemPoolMalloc(pager->hm, sizeof(pghd_t) + pager->extra_size +
		pager_get_journal_rec_len(pager));

	if(NULL == pg)
		return NULL;

	pager_init_page(pager, pg, pgno);

	return pg;
}

/**
 * @brief �ͷ�һ��ҳ����
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
 * @brief ͬ��������־�����
 */
static __INLINE__ int pager_sync_journal(pager_t * pager)
{
	pghd_t * pg = NULL;

	assert(pager && pager->hjf);

	/* �������Ҫͬ��,ֱ�ӷ��� */
	if(!pager->need_sync_journal)
		return 0;

	/* ����־ͷд��,��ҪΪ�����˶���ҳ */
	if(0 != OsFileSeek(pager->hjf, pager->jouranl_prev_hdr + PAGER_JOURNAL_REC_COUNT_OFFSET))
		return -1;

	if(0 != pager_write_uint_to_file(pager->hjf, pager->in_journal_pages_count))
		return -1;

	if(0 != OsFileSeek(pager->hjf, pager->journal_end))
		return -1;

	/* ����os��file sync�ӿ�ͬ������� */
	if(0 != OsFileSyn(pager->hjf))
		return -1;

	pager->need_sync_journal = 0;
	pager->bsync_journal_has_done = 1;

	if(NULL == pager->pg_dirty)
		return 0;

	/* ������dirtyҳ��need_sync_journal�ó�0 */
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
 * @brief ��ȡ����ļ������ж���ҳ
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
 * @brief ����ҳ�ļ�����ҳ��
 */
#define pager_set_total_pages_count(__pgr, __pgno) do{ \
	unsigned int total_pages_count = pager_get_total_pages_count(__pgr);\
	assert(__pgno <= total_pages_count + 1);\
	if(total_pages_count < __pgno) \
		__pgr->total_page_count = __pgno;\
	}while(0)

/**
 * @brief ��ȡҳ������û����׵�ַ
 */
#define pager_get_page_bufdata(_pg) ((unsigned char *)(&(_pg[1])))

/**
 * @brief ����ָ��ҳ�ŵ�ҳ����ҳ�ļ��е�ƫ��
 */
#define pager_cal_page_hdr_offset(__pgr, __pgno) (__pgr->page_size * (__pgno - 1))

/**
 * @brief ����ҳ��,��ȡĳһҳ������
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
 * @brief ����ҳ��,��ȡһҳ������
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
 * @brief ��ָ��������д��ĳһҳ
 */
static __INLINE__ int pager_write_page_data(pager_t * pager, unsigned char * buf, unsigned int buf_size, unsigned int pgno)
{
	assert(pager && buf && buf_size);
	assert(pager->hf);
	assert(buf_size <= pager->page_size);

	/* ��ҳд�����ҳ�ļ� */
	if(0 != OsFileSeek(pager->hf, pager_cal_page_hdr_offset(pager, pgno)))
		return -1;

	if(0 != OsFileWrite(pager->hf, buf, buf_size, NULL))
		return -1;

	return 0;
}

/**
 * @brief ��ĳһ������д��ҳ�ļ�
 */
static __INLINE__ int pager_write_page(pager_t * pager, pghd_t * pg)
{
	assert(pager && pg && pager == pg->pager);
	assert(pg->page_num);
	assert(pager->hf);

	assert(pager_page_is_dirty(pg));

	/* ��ҳд�����ҳ�ļ� */
	if(0 != pager_write_page_data(pager, pg->pg_buf_start, pager->page_size, pg->page_num))
		return -1;

	return 0;
}

/**
 * @brief Ѱ��jfͷ��λ��
 * jfͷ��λ��Ӧ���뵽sector_size��ֵ,����sector_sizeΪ512
 * 0 512 1024Ϊjfͷ����ʼλ��
 */
#define pager_locate_jf_hdr_pos(__pgr) \
	((__pgr->journal_end % __pgr->sector_size)?(__pgr->journal_end + CAL_ALIGMENT(__pgr->journal_end, __pgr->sector_size)):__pgr->journal_end)

/**
 * @brief дjfͷ
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
 * @brief дjfͷ
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
	* journal��־�ļ�ͷ����
	* offset bytes          des
	* 0      4              ͷ�Ĵ�Сhd_sz
	* 4      16             ��ʽ�汾
	* 20     4              ��־�ﱸ���˼�ҳ
	* 24     4              У��ͳ�ʼ���ֵ
	* 28     4              ������־ǰҳ�ļ��Ĵ�С
	* 32     hd_sz-24       ����
	*/
	pager->jouranl_prev_hdr = pager_locate_jf_hdr_pos(pager);
	assert((pager->jouranl_prev_hdr % pager->sector_size) == 0);
	assert(pager->jouranl_prev_hdr >= pager->journal_end);

	if(0 != OsFileSeek(pager->hjf, pager->jouranl_prev_hdr))
		goto pager_write_jf_head_end_;

	/* д��ͷ�Ĵ�С */
	uint_to_big_endian(pager->sector_size, hdr->sector_size, sizeof(hdr->sector_size));

	/* ����ħ���� */
	memcpy(hdr->magic, JOURNAL_MAGIC_STRING, sizeof(hdr->magic));

	/* д��¼��,��ʼӦΪ�� */
	uint_to_big_endian(0, hdr->rec_count, sizeof(hdr->rec_count));

	/* д��У��ͳ�ʼ���ֵ */
	pager_reset_cksum(pager);
	uint_to_big_endian(pager->cur_cksum_init, hdr->cksum_init, sizeof(hdr->cksum_init));

	/* д�뱸��jfǰҳ�ļ��ж���ҳ,���ڻع�ʱ�Լ� */
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
 * @brief ��jfͷ
 * @return 0:�ɹ� -1:ʧ�� 1:���ݲ���ȷ,���ǲ���Ӧ������
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
	* journal��־�ļ�ͷ����
	* offset bytes          des
	* 0      4              ͷ�Ĵ�Сhd_sz
	* 4      16             ��ʽ�汾
	* 20     4              ��־�ﱸ���˼�ҳ
	* 24     4              У��ͳ�ʼ���ֵ
	* 28     4              ������־ǰҳ�ļ��Ĵ�С
	* 32     hd_sz-24       ����
	*/

	/* ��λjfͷ��ƫ�� */
	pager->journal_end = pager_locate_jf_hdr_pos(pager);

	if(pager->journal_end + PAGER_JOURNAL_REC_COUNT_LEN >= file_size)
		return RHAPSODY_DONE;

	if(OsFileSeek(pager->hjf, pager->journal_end))
		return RHAPSODY_FAIL;

	/* ����ҳͷ�Ĵ�С */
	if(0 != pager_read_uint_from_file(pager->hjf, &sector_size))
		return RHAPSODY_FAIL;
	if(sector_size < PAGER_MIN_SECTOR_SIZE)
		return RHAPSODY_DONE;
	if(pager->journal_end + PAGER_JOURNAL_REC_COUNT_LEN >= file_size)
		return RHAPSODY_DONE;

	/* ����magic�ַ���,�ж����Ƿ�Ϸ� */
	if(0 != OsFileRead(pager->hjf, magic, sizeof(magic), NULL))
		return RHAPSODY_FAIL;
	if(memcmp(magic, JOURNAL_MAGIC_STRING, JOURNAL_MAGIC_LEN) != 0)
		return RHAPSODY_DONE;

	/* ������־�ﱸ���˼�ҳ */
	if(0 != pager_read_uint_from_file(pager->hjf, pin_journal_count))
		return RHAPSODY_FAIL;

	/* ����У��ͳ�ʼֵ */
	if(0 != pager_read_uint_from_file(pager->hjf, pcksum_init))
		return RHAPSODY_FAIL;

	/* ��������ǰ��ҳ�ļ���С */
	if(0 != pager_read_uint_from_file(pager->hjf, porig_pages_count))
		return RHAPSODY_FAIL;

	pager->journal_end += sector_size;

	/* ��λ�ļ�ָ��λ�� */
	if(0 != OsFileSeek(pager->hjf, pager->journal_end))
		return RHAPSODY_FAIL;
	
	return RHAPSODY_OK;
}

/**
 * @brief ��ֻ���ķ�ʽ��journal
 */
static __INLINE__ int pager_open_journal_readonly(pager_t * pager)
{
	assert(pager && NULL ==	pager->hjf);

	/* ��jf�ļ� */
	pager->hjf = OsFileOpenReadOnly((char *)MyBufferGet(pager->hb_jf_name, NULL), pager->hm);
	if(NULL == pager->hjf)
		return -1;

	/* ���ԭ�е���ɫ�� */
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
 * @brief ��jf
 */
static __INLINE__ int pager_open_journal(pager_t * pager)
{
	assert(pager && NULL ==	pager->hjf);

	/* ��jf�ļ� */
	pager->hjf = OsFileOpenExclusive((char *)MyBufferGet(pager->hb_jf_name, NULL), pager->hm);
	if(NULL == pager->hjf)
		return -1;

	pager->jouranl_prev_hdr = 0;
	pager->journal_end = 0;

	/* д��jfͷ,У��ͳ�ʼ�������ֵ,��ʼ���ݿ�Ĵ�С */
	if(0 != pager_write_jf_head(pager))
		return -1;

	/* ���ԭ�е���ɫ�� */
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
 * @brief �ر�jf��ɾ��jf�ļ�,������и�jf�йص�
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
 * @brief ����һҳ����У���
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
 * @brief ���һҳҪjf��s
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
 * @brief ����һҳ����
 */
static __INLINE__ pghd_t * pager_recycle_page(pager_t * pager)
{
	int need_sync = 0;
	pghd_t * pg = NULL;

	assert(pager);
	assert(pager->pg_free_first);

	/*
	* ����������ü���Ϊ���ҳ����,����̭��ҳ����,����װ���µ�ҳ,��ʱ��Ҫ����ҳд��fd,
	*������ȡ��Щͬ����jfd�Ŀ���ҳ����,���û��,��ͬ��jfd,ͬʱ�ӹ�ϣ����ɾ����Ӧ�ļ�¼
	*  ���ҳ���ڿ���������,����
	*  ���ҳ����ͬ����jfd�Ŀ���������,����
	*/

	if(pager->pg_syncjf_first)
	{
		/* ����ȡͬ����jfd�Ŀ��л���ҳ */
		pg = pager->pg_syncjf_first;
		need_sync = 0;
	}
	else
	{
		/* ���û��,��ȡûͬ�����Ŀ���ҳ���� */
		pg = pager->pg_free_first;
		need_sync = 1;
	}

	assert(pg);

	if(pager_page_is_dirty(pg))
	{
		if(need_sync)
		{
			/* ͬ��jf */
			if(0 != pager_sync_journal(pager))
				return NULL;
			pager->in_journal_pages_count = 0;

			if(0 != pager_write_jf_head(pager))
				return NULL;
		}

		/* д��fd */
		if(0 != pager_write_page(pager, pg))
			return NULL;

		/* Ӧ��¼�Ѿ���ҳ����д����ҳ�ļ�,�����ݻ����������� */
		pager->page_file_is_integrity = 0;

		/* ��ҳ�óɸɾ�,����dirty���������� */
		pager_page_out_of_dirty_list(pager, pg);

		/* ��ҳ��״̬�ó�д��״̬ */
		pager_change_page_state(pg, PAGE_EVT_WRITE_BACK_SYNCJF);
	}
	else
		assert(pager_page_is_clean_not_in_jf(pg) || (pager_page_in_journal(pager, pg->page_num) && pager_page_is_sync_journal(pg)));

	/* ����ڿ��������������� */
	pager_out_free_list(pager, pg);
	assert(!pager_page_is_in_free_list(pg));
	assert(!pager_page_is_in_syncjf_list(pg));

	/* �ӹ�ϣ����ɾ��,���Ӽ�¼����ҳ�����������ɾ�� */
	pager_outof_page_from_hash_and_list(pager, pg);

	return pg;
}

/**
 * @brief ����һҳ����,����ʼ��
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
 * @brief ��ĳһ��ҳ�����ó���,��д��jf
 */
static __INLINE__ void * pager_make_page_writable(pghd_t * pg)
{
	pager_t * pager = NULL;
	void * buf_write = NULL;

	/*
	* �ж�jfd�����Ƿ�������,���δ������Ӧ�ô�jfd�ļ�,��������ɫ��
	* �жϴ�ҳ�Ƿ��Ѿ�����jfd����(����ɫ�����),
	*    ���û������Ҫд��jfd,��ҳ��,��дҳ��������,���д��У���,���Ҹ���ҳ��������injournal��ʶ,������ɫ����Ӧ��λ���ñ�ʶ
	*    ��ҳ�����ó�dirty,����dirty����.
	* ���ҳ�űȵ�ǰ��ҳ����,
	* ��ʱӦ��ҳ�ļ�����ҳ����1
	* ����ҳ������׵�ַ���ϲ�д��(ע��Խ����������)
	*/

	assert(pg && pg->pager);
	assert(pg->ref_count > 0);

	buf_write = pager_get_page_bufdata(pg);

	pager = pg->pager;

	/* ���δ����jf����,��Ӧ���� */
	if(NULL == pager->hjf)
	{
		if(0 != pager_open_journal(pager))
			return NULL;
	}

	/* �����ҳ����jf��������(pg->page_state != PAGE_CLEAN) */
	if(pager_page_is_in_journal(pg))
	{
		assert(pg->page_num <= pager_get_total_pages_count(pager));

		/* ��ҳ�ó�dirty,������dirty list */
		if(!pager_page_is_dirty(pg))
			pager_make_page_dirty(pager, pg, PAGE_EVT_WRITE_BUF_BK);

		return buf_write;
	}

	/* ���(pg->page_state == PAGE_CLEAN)���������´��� */
	assert(!pager_page_is_dirty(pg));

	assert(pager->orig_pages_count != (unsigned int)-1);

	if(pg->page_num > pager->orig_pages_count)
	{
		/*
		* ҳ���޸�֮ǰ�����ǲ����ڵ�,����Ҳ����������д��,������Ǽ���
		* ����Ӧ�����ж��ҳ�ĳ���ҳ�ļ���С�����,��Ϊ�����ļ���С�����,ֻ��Ҫͬ��
		* jfͷ�Ϳ�����,�������ֻ��Ҫͬ��һ��,��֮���ٷ����������,��Ϊ������д�������
		* jf�ǲ��ò���,��Ϊ�Ѿ�ͬ������
		*/
		pager_set_total_pages_count(pager, ((unsigned int)pg->page_num));

		pager_make_page_dirty(pager, pg, PAGE_EVT_WRITE_BUF_BK);

		/* ����Ѿ�����ͬ��,����Ҫ�޸�״̬ */
		if(pager->bsync_journal_has_done)
			pager_change_page_state(pg, PAGE_EVT_SYNC_JOURNAL);
	}
	else
	{
		unsigned int cksum = pager_cal_page_cksum(pg->pg_buf_start, pager->page_size, pager->cur_cksum_init);
		/*
		* ���û������Ҫд��jfd, <ҳ��������,��ҳ��,���д��У���>,���Ҹ���ҳ��������injournal��ʶ,������ɫ����Ӧ��λ���ñ�ʶ
		*/
		uint_to_big_endian((unsigned int)pg->page_num, (pg->pg_buf_start + pager->page_size), sizeof(unsigned int));
		uint_to_big_endian(cksum,
			pg->pg_buf_start + pager->page_size + sizeof(unsigned int),
			sizeof(unsigned int));

		if(0 != OsFileWrite(pager->hjf, pg->pg_buf_start, pager_get_journal_rec_len(pager), NULL))
			return NULL;

		pager->in_journal_pages_count ++;
		pager->journal_end += pager_get_journal_rec_len(pager);

		/* ��ҳ�ó�dirty,������dirty list */
		pager_make_page_dirty(pager, pg, PAGE_EVT_WRITE_BUF_BK);

		assert(!pager_page_is_sync_journal(pg));
	}

	pager_mark_page_in_journal(pager, pg);

	if(!pager_page_is_sync_journal(pg))
		pager->need_sync_journal = 1;

	return buf_write;
}

/**
 * @brief ��jf��ع�
 */
static __INLINE__ int pager_file_truncate(pager_t * pager, unsigned int page_count)
{
	assert(pager && pager->hf);

	if(0 != OsFileTruncate(pager->hf, page_count * pager->page_size))
		return -1;

	return 0;
}

/**
 * @brief ��jf��ع�
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

	/* ����У���,�жϴ�ҳ�Ƿ�����ȷ�� */
	array_to_uint_as_big_endian(data_buf + pager->page_size + sizeof(unsigned int), sizeof(unsigned int), cksum);
	if(cksum != pager_cal_page_cksum(data_buf, pager->page_size, cksum_init))
		return RHAPSODY_DONE;

	/* ��ҳ�����ϣ������� */
	array_to_uint_as_big_endian(data_buf + pager->page_size, sizeof(unsigned int), pg_no);
	pg = pager_hash_lookup(pager, pg_no);

	if(NULL == pg || pager_page_is_written_before_page_sync(pg))
	{
		/*
		* �Ҳ���,��Ҫд���ļ�
		* �ҵ�,���ж��Ƿ����ύ֮ǰд�ع��ļ�,�����ҲҪд���ļ�
		*/
		if(0 != pager_write_page_data(pager, data_buf, pager->page_size, pg_no))
			return RHAPSODY_FAIL;
	}

	/* ��ˢ�ڴ� */
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

		/* �ص��û���reload�ص� */
		if(pager->page_reload_cb)
			pager->page_reload_cb(pg);
	}

	return RHAPSODY_OK;
}

/**
 * @brief ��jf��ع�
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

	/* ���䴦��ع���������Ҫ���ڴ� */
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
		/* �ȶ�ȡjfͷ */
		ret = pager_read_jf_head(pager, file_size, &injournal_count, &orig_page_count, &cksum_init);
		if(RHAPSODY_OK != ret)
			break;

		if(bfirst)
		{
			/* ��������ĵ�һ��jfͷ,��Ҫ��ҳ�ļ��Լ���ԭ���Ĵ�С */
			if(0 != pager_file_truncate(pager, orig_page_count))
			{
				ret = -1;
				goto pager_roll_back_from_journal_end_;
			}

			pager->total_page_count = orig_page_count;

			bfirst = 0;
		}

		/* ��������һ��jfͷ,���Ҽ�¼����ʾΪ��,���п���jf��δͬ�� */
		if(0 == injournal_count && pager->journal_end == (pager->jouranl_prev_hdr + pager->sector_size))
		{
			/* ˵����ʱ�п���jf����ͬ��,ʣ�����ҳ����ֱ����ҳ�ļ���ȡ,��Ϊû��д�� */
			assert(pager->journal_end + pager->in_journal_pages_count * pager_get_journal_rec_len(pager) == file_size
				|| 0 == pager->ref_count);

			assert((pager->journal_end < file_size && pager->need_sync_journal && pager->pg_dirty) || 0 == pager->ref_count || NULL == pager->pg_dirty);

			/*
			* ��ʱ���Ե�ҳ�ļ���ҳ�Ǹɾ���,����ʱ��Ȼ��jf��ȡ
			* �����Ƕ���ļ���У��͵Ŀ���,
			* ����ĳ�ֱ�Ӵ���ҳҳ�ļ���,��������ص�,�Ե�os���ļ�����֯��ʽ.
			* ���������flash,���ַ���Ч�ʽϵ�.
			* ��������ǻ�е����,��Ҫ��ҳ�ļ��Ĵ�С,�Լ�os���ļ�����֯.Ҳ���jf���,���Լ��ٴ���ioʱ��Ѱ��ʱ��.
			*/
			injournal_count = pager->in_journal_pages_count;
		}

		/* ����jfͷ,ѭ������ÿһ�����ݼ�¼ */
		for(i = injournal_count; i > 0; i --)
		{
			ret = pager_roll_back_one_page(pager, orig_page_count, cksum_init, data_buf, data_buf_len);
			if(RHAPSODY_OK != ret)
				break;
		}
	}

	{
		/*
		* ����Ҫ��ԭ�ڴ�����Щ����ҳ�ļ���С��ҳ����,
		* �����ǵ�����ˢ��0
		* todo:���ȴ˶δ���
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

	/* ���ret != RHAPSODY_FAIL ɾ��������־�ļ� */
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
 * @brief ��һҳ��ȡҳ��������,�ж��Ƿ����jf�ļ�,������ִ�лع�����
 */
static __INLINE__ int pager_check_and_roll_back(pager_t * pager)
{
	assert(pager && pager->hf);

	/* �ж�jf�ļ��Ƿ����,������,���سɹ� */
	if(!OsFileExists((char *)MyBufferGet(pager->hb_jf_name, NULL)))
		return 0;

	/*
	* ����,��Ҫ�������лع�
	* ��ֻ����ʽ��jf�ļ�
	*/
	if(0 != pager_open_journal_readonly(pager))
		return -1;

	/* �ع� */
	if(0 != pager_roll_back_from_journal(pager))
		return -1;

	return 0;
}

/**
 * @brief ���¼���ҳ�����е���ҳ
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

		/* ������ļ���ȡ��Ӧ��ҳ */
		if(pg->page_num <= pager->orig_pages_count)
		{
			if(0 != pager_read_page_data(pager, pg->page_num, buf, pager->page_size))
				return -1;

			/* ������ҳ���� */
			memcpy(pg->pg_buf_start, buf, pager->page_size);
		}
		else
			memset(pg->pg_buf_start, 0, pager->page_size);

		/* �ı�ҳ��״̬ */
		pager_page_out_of_dirty_list(pager, pg);

		pager_change_page_state(pg, PAGE_EVT_ROLL_BACK);
		assert(PAGE_CLEAN == pg->pg_state);

		if(pager->page_reload_cb)
			pager->page_reload_cb(pg);
	}

	assert(buf);
	MyMemPoolFree(pager->hm, buf);

	assert(NULL == pager->pg_dirty);

	/* ɾ�����ݵ�jf�ļ� */
	if(0 != pager_close_and_del_journal(pager))
		return -1;

	return 0;
}

/**
 * @brief ȡ�����е�ҳ�ĸ���
 */
static __INLINE__ int pager_rollback(pager_t * pager)
{
	int ret = -1;

	assert(pager);

	/*
	* ��PagerSyn֮ǰ,RollBack����Ч��,PagerSyn֮��,RollBackû��������.
	* RollBack�ж�ҳ�ļ��Ƿ��Ѿ�ͬ����jfd,������fd��д������.��ʱӦ��jfd��ع�����ҳ
	* ���û��,�����¼���fd����Ӧ��ҳ����ҳ���漴��,
	* ɾ���ݵ�jfd�ļ�.
	* jfd��ɫ���Լ�ҳ��injournal��ʶ���ó�0,��Ϊ���ݻָ�������״̬.
	*/

	if(!pager->page_file_is_integrity)/* ������ύ֮ǰ,�в���ҳ������д����ҳ�ļ�,��jf�ﻹԭ */
		ret = pager_roll_back_from_journal(pager);
	else if(NULL == pager->pg_dirty)/* ���û����Ļ���ҳ,��jfɾ������ */
		ret = pager_close_and_del_journal(pager);
	else/* ������ύ֮ǰ,û��ҳ����д��ҳ�ļ�,���½��໺�����һ�μ��� */
		ret = pager_reload_cache(pager);

	/* ��ɻع�֮��,jfӦɾ�� */
	return ret;
}

/**
 * @brief ��ҳ�ŵ�����������ıȽϻص�����
 *  > 0  ��ʾ key1 ���� key2
 *  == 0 ��ʾ key1 ���� key2
 *  < 0  ��ʾ key1 С�� key2
 *
 * @param context:�û��Զ��������������
 */
static int pager_sort_dirty_compare(const void * data1, const void * data2, const void * context)
{
	pghd_t * pg1 = *((pghd_t **)data1);
	pghd_t * pg2 = *((pghd_t **)data2);
	assert(pg1 && pg2);
	assert(pg2->page_num && pg1->page_num);

	/* 2008-8-22 ҳ�Ű�������,�����ύʱ������ʱ�� */
	if(pg1->page_num <= pg2->page_num)
		return pg2->page_num - pg1->page_num;
	else
		return -1;
}

/**
 * @brief ͬ�������໺��ҳ
 */
static __INLINE__ int pager_sort_dirty_list(pager_t * pager)
{
	/*
	* sqlite���õ��ǹ鲢����
	* ����ҳ��������,��֤����С��ҳ��ʼд�����
	* �ڽ���ҳд�����ҳ�ļ�֮ǰ,��ҳ�Ž��������Ǻ���Ҫ��
	*
	* ����һ���ڴ�����
	* �����е�ҳ������д����ڴ�����
	* ��ҳ��Ϊ�ȽϹؼ��ֶ����������п�������
	* ������õ����鰴���������dirty list
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
 * @brief ͬ�������໺��ҳ
 */
static __INLINE__ int pager_syn(pager_t * pager)
{
	pghd_t * pg = NULL;

	/*
	* ���Ƚ�jfd�ļ�ͬ�������,������ҳ�����ó�ͬ����jfd
	* ��ͬ�����Ŀ���ҳ�������ָ��������(����ҳ������ʱ,��ҳ���ȴ����������ȡ����ҳ����)
	* �����е�dirtyҳд��fd��,��dirty�����ÿ�.ÿҳ��dirty��ʶҲ�ÿ�
	* ɾ���ݵ�jfd�ļ�.
	* jfd��ɫ���Լ�ҳ��injournal��ʶ���ó�0,��Ϊһ�������ύ�Ѿ������.
	*
	* �ɹ�֮��,���ļ����������ó�1
	*
	*/

	assert((pager->pg_dirty && pager->hjf) || 
		(!pager->page_file_is_integrity && pager->hjf) ||
		(!pager->hjf && NULL == pager->pg_dirty && pager->page_file_is_integrity));

	/* �����������ҳ,������ҳ��������������,ͬ���Ͳ���Ҫ���� */
	if(NULL == pager->pg_dirty && pager->page_file_is_integrity)
		return 0;

	/* ��jfͬ������� */
	if(0 != pager_sync_journal(pager))
		return -1;

	/* ����ҳд��ҳ�ļ� */
	if(pager->pg_dirty)
		pager_sort_dirty_list(pager);
	for(pg = pager->pg_dirty; pg; pg = pager->pg_dirty)
	{
		assert(pager_page_is_dirty(pg));

		/* дҳ����д��ҳ�ļ� */
		if(0 != pager_write_page(pager, pg))
			return -1;

		/* ��dirty list������ */
		pager_page_out_of_dirty_list(pager, pg);

		/* �ı�ҳ״̬ */
		pager_change_page_state(pg, PAGE_EVT_WRITE_BACK_SYNCJF);
	}

	assert(NULL == pager->pg_dirty);

	/* ͬ��ҳ�ļ������ */
	if(0 != OsFileSyn(pager->hf))
		return -1;

	/*
	* �������ҳ��clean_sync���
	* todo ���ȴ˶δ���
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

	/* �ر�jouranl */
	if(0 != pager_close_and_del_journal(pager))
		return -1;

	return 0;
}

/**
 * @brief ���ٻ������
 */
static __INLINE__ int pager_destroy(pager_t * pager)
{
	assert(pager);

	/* ���ع����� */
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
		/* �����ͷ�ҳ���� */
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
 * @brief ҳ�����ϣ���ϣ����
 */
static size_t pg_hash_fun(const void * key)
{
	return (size_t)key;
}

/**
 * @brief ҳ�����ϣ��,�Ƚϼ�ֵ�Ƿ���� 1:��� 0:�����
 */
static int pg_hash_keyequl_fun(const void * key1, const void * key2)
{
	return key1 == key2;
}

/**
 * @brief ����ҳ��,��ȡһҳ����
 */
static __INLINE__ pghd_t * pager_get_page(pager_t * pager, unsigned int pgno)
{
	/*
	* ����ǵ�һ�λ�ȡҳ,��Ӧ�ж�journal��־���Ƿ��,�����������Ҫִ�лع�����
	* ��journal��־�ļ��еı���,���ζ���,���У���,�����ȷ,д��ҳ�ļ�fd
	* �ع�������,Ӧɾ��jfd�ļ�
	* ���ȴ�ҳ����Ĺ�ϣ�������,����ҵ�,ҳ���ü�����1,��������
	* ����ڻ������Ҳ�����Ӧ��ҳ,����Ҫ����ҳ����,������ȡ,��ʼ���Ƿ�injournal�ı�ʶ
	* ��ʱӦע��ҳ�����������:
	* ������������ü���Ϊ���ҳ����,����Ȼ����ҳ����.
	* ����������ü���Ϊ���ҳ����,����̭��ҳ����,����װ���µ�ҳ,��ʱ��Ҫ����ҳд��fd,
	*������ȡ��Щͬ����jfd�Ŀ���ҳ����,���û��,��ͬ��jfd,ͬʱ�ӹ�ϣ����ɾ����Ӧ�ļ�¼
	*  ��ҳ����ҳ����ָ����ӽ���ϣ��
	*  ���ҳ���ڿ���������,����
	*  ���ҳ����ͬ����jfd�Ŀ���������,����
	*  ҳ���ü�����1
	*/

	pghd_t * pg = NULL;

	assert(pager && pager->hash_page);

	if(0 == pager->ref_count && NULL == pager->hjf)
	{
		/*
		* ����ǵ�һ�λ�ȡҳ,��Ӧ�ж�journal��־���Ƿ��,�����������Ҫִ�лع�����
		* ��journal��־�ļ��еı���,���ζ���,���У���,�����ȷ,д��ҳ�ļ�fd
		* �ع�������,Ӧɾ��jfd�ļ�
		*/
		if(0 != pager_check_and_roll_back(pager))
			return NULL;
	}

	pg = pager_hash_lookup(pager,  pgno);

	if(NULL == pg)
	{
		if((pager->pg_free_first || pager->pg_syncjf_first) && pager_get_cached_pages_count(pager) >= pager->max_cache_pages)
			pg = pager_recycle_and_init_page(pager, pgno);/* ����һҳ���� */
		else
			pg = pager_allocate_page(pager, pgno);/* ����һҳ���� */

		if(NULL == pg)
			goto pager_get_page_err_;

		assert(pg->page_num == pgno);

		/* ��ȡҳ */
		if(0 != pager_read_page(pager, pgno, pg))
			goto pager_get_page_err_;

		/* �����ϣ��,�Լ��ܵ�ҳ�������� */
		if(0 != pager_add_page_to_hash_and_list(pager, pg))
			goto pager_get_page_err_;
	}

	pager_ref_page(pager, pg);
	return pg;

pager_get_page_err_:

	/* ����û�м�¼�ڰ�,��ҳ��������ͷ� */
	if(pg)
		pager_free_page(pager, pg);

	return NULL;
}

/**
 * @brief ȡ����ĳ��ҳ���������
 */
static __INLINE__ int pager_release_page(pghd_t * pg)
{
	pager_t * pager = NULL; 

	assert(pg && pg->pager);
	assert(pg->ref_count > 0);

	pager = pg->pager;

	/*
	* ҳ���ü�����1,
	* ���ҳ�����ü���Ϊ��,����Խ����������ҳ��������,�����ҳ�����Ѿ�ͬ����jfd,����Լ�����е�jfd��������,
	*   ������Ҫ�����û���release�ص�����֪ͨ�ϲ�
	*/

	/* ���ü�����1 */
	assert(pg->ref_count);
	pg->ref_count --;

	assert(pager->ref_count);
	pager->ref_count --;

	if(pg->ref_count > 0)
		return 0;

	assert(!pager_page_is_in_free_list(pg));
	assert(!pager_page_is_in_syncjf_list(pg));

	/* ������ü���Ϊ�� */
	/* �����Ҫͬ����jf,�����free list */
	if(pager_page_is_sync_journal(pg))
		pager_add_to_syncjf_list(pager, pg);/* �������Ҫͬ����jf,���뵽syncjf_list,�Ա����Ȼ��� */
	else
		pager_add_to_free_list(pager, pg);

	/* �����û��Ļص����� */
	if(pager->page_release_cb)
		pager->page_release_cb(pg);

	return 0;
}

/**
 * @brief д��ҳͷ��Ϣ
 */
static __INLINE__ int pager_write_file_page_info(pager_t * pager)
{
	/*
	* ҳ�ĵ�һҳ���ڴ��ҳ�ļ��������Ϣ
	* ��һҳ
	*  offset bytes        des
	*  0      16           ��ʽ�汾
	*  16     4            ҳ��С
	*  20     4            ��һ������ҳ
	*  24     40           ����
	*  64   pg_sz - 64     �û�����     
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
 * @brief ��ȡҳ�ļ���Ϣ,��ȡҳ��С,�Լ���������ҳ,���ж��ļ��汾
 */
static __INLINE__ int pager_read_file_page_info(pager_t * pager)
{
	/*
	* ҳ�ĵ�һҳ���ڴ��ҳ�ļ��������Ϣ
	* ��һҳ
	*  offset bytes        des
	*  0      16           ��ʽ�汾
	*  16     4            ҳ��С
	*  20     4            ��һ������ҳ
	*  24     40           ����
	*  64   pg_sz - 64     �û�����     
	*/
	char ver[sizeof(VERSION)];
	unsigned int pg_sz = 0;
	int64 pg_file_sz = 0;

	assert(pager && pager->hf);

	/* �ж�ҳ�ļ���С�Ƿ�����һҳ,������Ҫд��ҳ�ļ�ͷ��Ϣ */
	if(0 != OsFileSize(pager->hf, &pg_file_sz))
		return -1;

	if(pg_file_sz < sizeof(ver))
	{
		/* �޷������㹻����Ϣ,��Ӧ��Ϊ��ҳ�ļ�����Ч��,��Ҫд��ͷ��Ϣ */
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
		/* ҳ�ļ���ʽ�����ǵ�ǰ����֧�ֵ� */
		LOG_WARN(("page file version err,this page maybe create by the later pager verion,[file version:%s], [program ver:%s]",
			ver, VERSION));

		return -1;
	}

	if(0 != OsFileSeek(pager->hf, PAGER_HDR_VER_LEN))
		return -1;

	/* ����ҳ�Ĵ�С */
	if(0 != pager_read_uint_from_file(pager->hf, &pg_sz))
		return -1;

	if(pg_file_sz < pg_sz)
	{
		/* 
		* ���ҳ�ļ��Ĵ�С��һҳ�Ĵ�С 
		* ��Ϊ��һ���µ�ҳ�ļ�,��Ҫд��ҳ�ļ�ͷ��Ϣ
		*/
		if(0 != pager_write_file_page_info(pager))
			return -1;
	}
	else
	{
		pager->page_size = pg_sz;
	}

	/* ��ȡ��ǰ�ж���ҳ */
	pager->total_page_count = pager_get_total_pages_count(pager);

	return 0;
}


/**
 * @brief ����һ������ҳ������
 * @param hm:�ڴ�ؾ��
 * @param file_name:������ļ���
 * @param page_release_cb:ҳ�����ص�����
 * @param page_reload_cb:ҳ���ػص�����
 * @param extra_size:ÿҳ���ӵ���չ�û����ݴ�С
 * @param max_page:�����е����ҳ��
 * @param page_size:ÿһҳ�Ĵ�С,�����Ӧ��ҳ�ļ��Ѵ���,�򷵻�ҳ�Ĵ�С,��Ϊ��,Ĭ��Ϊ1024
 * @param used_rate:����ļ�ʹ����, 0:��ʾ����Ҫ�����Լ�, 1-9�ֱ��ʾʹ����,һ������ʹ����,������Զ��Լ�,>=10��ʾ����Ҫ�Լ�
 * @param rand_seed:�������ʼ������
 * @param rand_seed_size:���ӵĳ���
 * @param sector_size:����ϵͳ��������С
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

	/* ������־�ļ���������ļ��� */
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
	/* ������ļ� */
	pager->hf = OsFileOpenReadWrite(file_name, hm);
	if(NULL == pager->hf)
	{
		LOG_WARN(("fail open file : %s", file_name));
		goto PagerConstructor_err_;
	}
	pager->page_file_is_integrity = 1;

	pager->journal_end = 0;
	pager->jouranl_prev_hdr = 0;

	/* ��������������� */
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

	/* ҳ�����ϣ���ʼ�� */
	pager->hash_page = MyHashTableConstruct(hm, pg_hash_fun, pg_hash_keyequl_fun, 0);
	if(NULL == pager->hash_page)
		goto PagerConstructor_err_;

	if(max_cache_pages)
		pager->max_cache_pages = max_cache_pages;
	else
		pager->max_cache_pages = DEFAULT_CACHE_PAGES_COUNT;

	/* ҳ�Ƿ���journal״̬��λ��ɫ�� */
	pager->hash_pgno_journal = MyHashTableConstruct(hm, pg_hash_fun, pg_hash_keyequl_fun, 0);
	if(NULL == pager->hash_pgno_journal)
		goto PagerConstructor_err_;

	if(sector_size > PAGER_MIN_SECTOR_SIZE)
		pager->sector_size = sector_size;
	else
		pager->sector_size = PAGER_MIN_SECTOR_SIZE;

	pager->extra_size = extra_size;

	/* ҳ�ļ�����ȡ������ʱ�Ļص����� */
	pager->page_release_cb = page_release_cb;
	pager->page_reload_cb = page_reload_cb;

	/* ÿҳ��С */
	assert(page_size);
	pager->page_size = *page_size;
	if(pager->page_size < MIN_PAGE_SIZE)
		pager->page_size = MIN_PAGE_SIZE;
	else if(pager->page_size > MAX_PAGE_SIZE)
		pager->page_size = MAX_PAGE_SIZE;

	/* ��ȡҳ�Ĵ�С,�Լ��ж���ҳ */
	pager->total_page_count = (unsigned int)-1;
	if(-1 == pager_read_file_page_info(pager))
		goto PagerConstructor_err_;

	/* �����û���������ҳ��С */
	*page_size = PAGER_PAGE_USER_SIZE(pager);

	assert(pager_get_total_pages_count(pager) >= PAGER_FIRST_PAGE_NO);

	return pager;

PagerConstructor_err_:

	if(pager)
		pager_destroy(pager);

	return NULL;
}

/**
 * @brief ����һ������ҳ������
 */
void PagerDestruct(HPAGER hpgr)
{
	if(hpgr)
		pager_destroy(hpgr);
}

/**
 * @brief ͬ�������໺��ҳ
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
 * @brief ȡ�����е�ҳ�ĸ���
 */
int PagerRollBack(HPAGER hpgr)
{
	if(NULL == hpgr)
		return -1;

	return pager_rollback(hpgr);
}

/**
 * @brief �ж�ĳһҳ�Ƿ�Ϊ����,��ֹ�ظ��ͷ�
 * @return 0:�ɹ�, -1:ʧ��
 * @param pbfree:����ֵ 1:��ʾ����, 0:��ʾ����ʹ��
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
 * @brief ��ĳһҳ�����,���л��Ƿǿ���
 */
#define pager_write_page_free_flag(__pg, __f) do{\
		(__pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET)[0] = (__f);\
		(__pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET)[1] = 0xff;\
		(__pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET)[2] = 0xff;\
		(__pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET)[3] = 0xff;\
	}while(0)

/**
 * @brief ��ĳһҳ�����,���л��Ƿǿ���
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
 * @brief ����һ��ҳ��
 */
static unsigned int pager_get_page_no(pager_t * pager)
{
	unsigned int first_free_no = 0;
	unsigned int ret_pg_no = 0;
	pghd_t * first_pg = NULL;

	/*
	* ���ȴ�ҳ�ļ��ĵ�һҳѰ�ҵ�һ������ҳ��ҳ��.
	* �������ҳ����,��ȡ����һ������ҳ(������ڹ���Ŀ���ҳû������ҳ,����ĵ�һ������ҳ����������һ������ҳ),
	* ��Ӧ��ҳӦҪ��д�����(������jfd,���ύʱ����д��),��һҳ(�п���Ҫд),��һ��������ҳ(һ��Ҫд)
	* ��Щҳ��Ҫ�����ؽ�����
	* ����������κο���ҳ,��ζ��ҳ�ļ���Ҫ������,����ҳ�ļ��ܹ��ж���ҳ,��ֵ��1��Ϊ�µ�ҳ��
	* �ҵ�ҳ�ŵĻ�����ҳ��������ϣ��(���������),��������������ó�0
	* ����ҳ�ĵ�һ���ֽ��ó�0xff��ʾ��ҳ�ѱ�ʹ��.
	*
	* todo �����ҳ,��ǰ����jf������,����Ҫ�ж��Ƿ���Ҫ��¼����ԭʼҳ�ļ���С��ҳ
	*/

	assert(pager && pager->hf);

	/*
	* ���ҳ�ļ���ʽ����
	* ��һҳ
	*  offset bytes        des
	*  0      16           ��ʽ�汾
	*  16     4            ҳ��С
	*  20     4            ��һ������ҳ
	*  24     40           ����
	*  64   pg_sz - 64     �û�����
	*/

	/* ��ȡ��һҳ */
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
		* û�п���ҳ
		* ȡҳ�ļ���ҳ����1
		*/
		ret_pg_no = pager_get_total_pages_count(pager) + 1;

		/* ��ǳɷǿ���ҳ */
		if(0 != pager_mark_page_is_free_or_no(pager, ret_pg_no, 0))
			ret_pg_no = 0;
		else
			assert(ret_pg_no == pager_get_total_pages_count(pager));
	}
	else/* �������ҳ���� */
	{
		/*
		* ����ҳ:
		* ��һ������ҳ
		* offset bytes   des
		* 0      4       �ܵĿ���ҳ��
		* 4      4       ����ҳ"����"����һ������ҳ��
		* 8      4       ��ǰҳ������˶��ٵĿ���ҳ��
		* 12             �������鿪ʼ,ÿ4���ֽڱ�ʾһ������ҳ��
		* �ǵ�һ������ҳ
		* 0      4       ������
		* 4      4       ����ҳ"����"����һ������ҳ��
		* 8      4       ��ǰҳ������˶��ٵĿ���ҳ��
		* 12             �������鿪ʼ,ÿ4���ֽڱ�ʾһ������ҳ��
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
			/* ����ҳ��������Ԫ��,����ȡһ��������û� */
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

			/* ���õ��Ŀ���ҳ��ǳɷǿ��� */
			if(0 != pager_mark_page_is_free_or_no(pager, ret_pg_no, 0))
			{
				pager_release_page(pg_first_free);
				ret_pg_no = 0;
				goto pager_get_page_no_end_;
			}

			/* �ı䵱ǰ����ҳ����Ĵ�С */
			free_count -= 1;
			uint_to_big_endian(free_count, pager_get_page_bufdata(pg_first_free) + PAGER_FREE_FREE_COUNT_OFFSET, sizeof(unsigned int));

			assert(total_free_count);
			total_free_count -= 1;
			uint_to_big_endian(total_free_count, pager_get_page_bufdata(pg_first_free) + PAGER_FREE_TOTAL_FREE_COUNT_OFFSET, sizeof(unsigned int));
		}
		else
		{
			/* û��Ԫ��,Ҫ����ҳ������û� */

			unsigned int next_free_link = 0;

			ret_pg_no = first_free_no;

			array_to_uint_as_big_endian(pager_get_page_bufdata(pg_first_free) + PAGER_FREE_NEXT_FREE_LINK_OFFSET,
				sizeof(next_free_link),
				next_free_link);

			if(0 == next_free_link)
			{
				/* Ҫ�����ĵ�һҳ���д��һ������ҳ��ҳ���ó�0 */
				if(NULL == pager_make_page_writable(first_pg))
				{
					pager_release_page(pg_first_free);
					ret_pg_no = 0;
					goto pager_get_page_no_end_;
				}

				/* ���õ��Ŀ���ҳ��ǳɷǿ��� */
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

				/* ���µĵ�һ������ҳ���¼�ܹ��ж��ٿ���ҳ */
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

				/* ��ҳ�óɷǿ��� */
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

				/* Ҫ�����ĵ�һҳ���д��һ������ҳ��ҳ���ó��µĵ�һ������ҳҳ�� */
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
 * @brief ����һ��ҳ��
 * @return ����һ�����õĿ���ҳ��
 */
unsigned int PagerGetPageNo(HPAGER hpgr)
{
	if(NULL == hpgr)
		return 0;

	return pager_get_page_no(hpgr);
}

/**
 * @brief ����һҳ�������ɶ��ٸ�ҳ��
 */
#define pager_cal_pgno_count(__pgr) ((PAGER_PAGE_USER_SIZE(__pgr) - PAGER_FREE_FREE_PGNO_ARRAY_OFFSET)/sizeof(unsigned int))

/**
 * @brief ��һ������ҳ�������ҳ"����"��"ͷ��"
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

	/* ����ҳ��Ϊ��һ������ҳ��д���һҳ */
	if(NULL == pager_make_page_writable(first_page))
	{
		ret = -1;
		goto pager_add_free_to_list_end_;
	}

	uint_to_big_endian(pgno,
		first_page->pg_buf_start + PAGER_HDR_FIRST_FREE_OFFSET,
		sizeof(unsigned int));

	/* ����ص���Ϣд��˿���ҳ */
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
 * @brief �ͷ�һ��ҳ��,��ҳ���������ҳ,�����Կ���ҳҳ�Ž�������
 */
static __INLINE__ int pager_release_pgno_no_sort(pager_t * pager, unsigned int pgno)
{
	/*
	* ���û��,�򽫿���ҳ������������ҳ����,���������ҳ����,����չ����ҳ"����",�ı�������ҳ����
	* ��������Ҫд��(���δ��), ���������ҳ����:��һҳ��Ҫд��,����ҳ->������ҳ(������ҳ������Ҫд��)
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

	/* �������ҳ������,����ҳ��Ϊ��һ������ҳ */
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

			/* ��ҳ��д��������ҳ���� */
			uint_to_big_endian(pgno,
				pager_get_page_bufdata(pg_first_free) + PAGER_FREE_FREE_PGNO_ARRAY_OFFSET + free_count * sizeof(unsigned int),
				sizeof(unsigned int));

			/* ������ҳ�����������1 */
			free_count ++;
			uint_to_big_endian(free_count,
				pager_get_page_bufdata(pg_first_free) + PAGER_FREE_FREE_COUNT_OFFSET,
				sizeof(unsigned int));

			/* ����ҳ������1 */
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
 * @brief �ͷ�һ��ҳ��,���óɿ���ҳ
 */
static __INLINE__ int pager_release_pgno(pager_t * pager, unsigned int pgno)
{
	/*
	* ��ҳ�ĵ�һ���ֽ��ó�0,��ʾ��ҳ����ʹ����
	* �ж��Ƿ�򿪵���ȥ������ҳ�Ļ���
	*  ���û��,�򽫿���ҳ������������ҳ����,���������ҳ����,����չ����ҳ"����",�ı�������ҳ����
	*      ��������Ҫд��(���δ��), ���������ҳ����:��һҳ��Ҫд��,����ҳ->������ҳ(������ҳ������Ҫд��)
	*  �����,
	*      ����Ҫ�Կ���ҳ����ҳ������.��֤ҳ��С�Ŀ���ҳ����ǰ��,��֤����ҳ�����Ч��.���ͷ�һҳʱ,����"����"����,Ҫ���ʵ�ҳ���
	*      ���ҳ����,Ӧ��ҳ�ſ������ԭ����Ѹ�ҳ.
	*         ÿ�ͷ�һҳ,����ҳ����Ӧ����һ,��д�����ҳ�ļ�.
	*         ���������Զ���Ƭ����ʱ,���ڹ���Ŀ���ҳ��ҳ��Ӧ���������صĿ���ҳ������(�ڷ�����ʱ���Լ���д��,�ᵼ���ͷ�ʱ���һ��"����").
	*      δ��:ҳ����������Ǹ�����ҳ, ������:����д���Ѻ��µĳ���ҳ
	* ����������Զ�truncate,��ʱ�ж��Ƿ�ﵽ�˰ٷֱ�,����ﵽ��,truncate��ʼִ��.
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
 * @brief �ͷ�һ��ҳ��,���óɿ���ҳ
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
 * @brief ��ȡ��ǰҳ�ļ����ж���ҳ
 */
unsigned int PagerGetTotalPagesCount(HPAGER hpgr)
{
	if(NULL == hpgr)
		return 0;

	return pager_get_total_pages_count(hpgr);
}

/**
 * @brief ��ȡ��ǰҳ�ļ����ж��ٿ���ҳ
 */
unsigned int PagerGetFreePages(HPAGER hpgr)
{
	/**
	* ��һҳ
	*  offset bytes        des
	*  0      16           ��ʽ�汾
	*  16     4            ҳ��С
	*  20     4            ��һ������ҳ
	*  24     40           ����
	*  64   pg_sz - 64     �û�����     
	*
	* ����ҳ:
	* ��һ������ҳ
	* offset bytes   des
	* 0      4       �ܵĿ���ҳ��
	* 4      4       ����ҳ"����"����һ������ҳ��
	* 8      4       ��ǰҳ������˶��ٵĿ���ҳ��
	* 12             �������鿪ʼ,ÿ4���ֽڱ�ʾһ������ҳ��
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
 * @brief ��ҳ��������л�ȡһҳ ��PagerReleasePage�Ƕ�ż�Ĳ���
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
 * @brief ��һ��ҳ�������ü�������,��Ҫ��PagerReleasePage������������
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
 * @brief �ӻ���������ָ��ҳ�ŵ�ҳ����,���ҳ���ڻ�����,����NULL
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
 * @brief ȡ���Ե�ǰҳ������ ��PagerGetPage�Ƕ�ż�Ĳ���
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
 * @brief ��ȡҳ���ݵĻ�����,����д��
 */
void * PageHeadMakeWritable(HPAGE_HEAD pg)
{
	if(NULL == pg || NULL == pg->pager)
		return NULL;

	return pager_make_page_writable(pg);
}

/**
 * @brief ��ȡĳһ��ҳ����,���ڶ�ȡ
 */
const void * PageHeadMakeReadable(HPAGE_HEAD pg)
{
	if(NULL == pg || NULL == pg->pager)
		return NULL;

	return pager_get_page_bufdata(pg);
}

/**
 * @brief ��ȡĳһҳ���û�����
 */
void * PageHeadGetUserData(HPAGE_HEAD pg)
{
	if(NULL == pg)
		return NULL;

	return pg->pg_buf_start + pager_get_journal_rec_len(pg->pager);
}

/**
 * @brief ��ȡĳһҳ���û����ݵĳ���
 */
unsigned PageHeadGetUserDataSize(HPAGE_HEAD pg)
{
	if(NULL == pg || NULL == pg->pager)
		return 0;

	return pg->pager->extra_size;
}

/**
 * @brief ���ҳ�����״̬
 */
unsigned int PagerGetRefCount(HPAGER hpgr)
{
	assert(hpgr);

	return hpgr->ref_count;
}

/**
 * @brief ��ȡĳһҳ��������ü���
 */
unsigned int PagerGetPageRefCount(HPAGE_HEAD pg)
{
	assert(pg);

	return pg->ref_count;
}

/**
 * @brief ���ĳ��ָ���Ƿ�Խ����
 */
int PagePtrIsInpage(HPAGE_HEAD pg, const void * ptr)
{
	if(NULL == pg)
		return 0;

	assert((unsigned char *)ptr >= pager_get_page_bufdata(pg) && (unsigned char *)ptr <= (pg->pg_buf_start + pg->pager->page_size));
	return ((unsigned char *)ptr >= pager_get_page_bufdata(pg) && (unsigned char *)ptr <= (pg->pg_buf_start + pg->pager->page_size));
}


/**
 * @brief for debug,���jf�ļ��Ƿ����
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

		/* ����ҳͷ�Ĵ�С */
		assert(0 == pager_read_uint_from_file(hpgr->hjf, &sector_size));

		assert(sector_size == hpgr->sector_size);

		if(sector_size < PAGER_MIN_SECTOR_SIZE)
			break;

		if(i + PAGER_JOURNAL_REC_COUNT_LEN >= jsz)
			break;

		/* ����magic�ַ���,�ж����Ƿ�Ϸ� */
		assert(0 == OsFileRead(hpgr->hjf, magic, sizeof(magic), NULL));

		assert(memcmp(magic, JOURNAL_MAGIC_STRING, JOURNAL_MAGIC_LEN) == 0);

		/* ������־�ﱸ���˼�ҳ */
		assert(0 == pager_read_uint_from_file(hpgr->hjf, &in_journal_count));


		/* ����У��ͳ�ʼֵ */
		assert(0 == pager_read_uint_from_file(hpgr->hjf, &cksum_init));

		/* ��������ǰ��ҳ�ļ���С */
		assert(0 == pager_read_uint_from_file(hpgr->hjf, &orig_pages_count));

		i = (unsigned int)(offset + sector_size);

		/* ��λ�ļ�ָ��λ�� */
		assert(0 == OsFileSeek(hpgr->hjf, i));

		/* ��������һ��jfͷ,���Ҽ�¼����ʾΪ��,���п���jf��δͬ�� */
		if(0 == in_journal_count && i == (hpgr->jouranl_prev_hdr + hpgr->sector_size))
		{
			/* ˵����ʱ�п���jf����ͬ��,ʣ�����ҳ����ֱ����ҳ�ļ���ȡ,��Ϊû��д�� */
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

			/* ����У���,�жϴ�ҳ�Ƿ�����ȷ�� */
			array_to_uint_as_big_endian(data_buf + hpgr->page_size + sizeof(unsigned int), sizeof(unsigned int), cksum);
			assert(cksum == pager_cal_page_cksum(data_buf, hpgr->page_size, cksum_init));

			/* ��ҳ�����ϣ������� */
			array_to_uint_as_big_endian(data_buf + hpgr->page_size, sizeof(unsigned int), pg_no);
			assert(MyHashTableSearch(hpgr->hash_pgno_journal, (void *)pg_no));

			/*
			* ��֤ҳ��������Ƿ���ԭ���,�Ǻ�
			* ������д��page,��������ͨ����Ŷ
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
 * @brief ���������ҳ"����"�Ƿ���ȷ
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

		/* ÿһҳ����used��ʶӦΪ0xff */
		for(i = 1; i <= total_page_count; i ++)
		{
			HPAGE_HEAD pg = NULL;

			if(i == PAGER_FIRST_PAGE_NO)
				continue;

			pg = PagerGetPage(hpgr, i);
			assert(pg);

			/* �����б�ʶλ */
			assert(*(pg->pg_buf_start + PAGER_PAGE_IN_USE_OFFSET) == 0xff);

			PagerReleasePage(pg);
		}
	}
	else
	{
		unsigned int total_free_pages_count = 0;/* �ӵ�һ������ҳ���ȡ�����Ŀ���ҳ���� */
		unsigned int lnk_free_pages_count = 0;/* �ӿ���ҳ����ͳ�Ƴ����Ŀ���ҳ���� */
		unsigned int real_free_pages_count = 0;/* �����ҳ�ļ���ͳ�Ƴ����Ŀ���ҳ���� */

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

				/* �����б�ʶλ */
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
			/* ѭ��ͳ�����е����ҳ�ļ� */
			for(i = 1; i <= total_pages_count; i ++)
			{
				HPAGE_HEAD pg = NULL;

				if(i == PAGER_FIRST_PAGE_NO)
					continue;

				pg = PagerGetPage(hpgr, i);
				assert(pg);

				/* �����б�ʶλ */
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
 * @brief ���ҳ�����״̬
 */
void PagerExamin(HPAGER hpgr, int check_jf, int check_free, void * buf, unsigned int buf_sz)
{
	pghd_t * pg = NULL;
	unsigned int pg_count = 0;
	unsigned int ref_count = 0;

	assert(hpgr);

	/* ���free���� */
	for(pg = hpgr->pg_free_first; pg; pg = pg->pg_free_next)
	{
		assert(0 == pg->ref_count);
		assert(pager_page_is_in_all_list(pg));
	}

	/* ���synjf���� */
	for(pg = hpgr->pg_syncjf_first; pg; pg = pg->pg_syncjf_next)
	{
		assert(pager_page_is_sync_journal(pg));
		assert(0 == pg->ref_count);
		assert(pager_page_is_in_all_list(pg));
		assert(MyHashTableSearch(hpgr->hash_pgno_journal, (void *)pg->page_num));
	}

	/* ���dirty���� */
	for(pg = hpgr->pg_dirty; pg ; pg = pg->pg_dirty_next)
	{
		assert(pager_page_is_in_all_list(pg));
		assert(pager_page_is_dirty(pg));

		assert(MyHashTableSearch(hpgr->hash_pgno_journal, (void *)pg->page_num));
	}

	/* ���all���� */
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

	/* ����ϣ�������Ԫ�ظ�����all������ĸ�����ͬ */
	assert(MyHashTableGetElementCount(hpgr->hash_page) == pg_count);
	assert(ref_count == hpgr->ref_count);

	/* ���jf���� */
	if(hpgr->pg_dirty)
		assert(hpgr->hjf);

	/* ���ͬ����,���ʾ��д������� */
	if(hpgr->bsync_journal_has_done && hpgr->hjf)
		assert(0 == hpgr->page_file_is_integrity);

	/* ���jf */
	if(check_jf)
		pager_check_jf(hpgr, buf, buf_sz);

	/* ���������ҳ"��"�� */
	/* ����ҳ����,ÿҳ��Ӧ�ı�ʶ,����ҳ����,��ѭ����ҳ,�����б�ʶ */
	if(check_free)
		pager_check_free(hpgr);
}




















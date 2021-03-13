/**
 * @file page.h ���ҳ������� 2008-1-29 23:59
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.
 *        ��������ҳ�������Ľӿ�,��ͨ�����ݵĻ���ʵ�����ݵ�������.
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

/* ҳ��������� */
struct __pager_t_;
typedef struct __pager_t_ * HPAGER;

/* ҳͷ��Ϣ��� */
struct __pghd_t_;
typedef struct __pghd_t_ * HPAGE_HEAD;

/* ҳ�����ص����� */
typedef int(*PAGE_RELEASE)(HPAGE_HEAD pg);

/* ҳ��������ص����� */
typedef int(*PAGE_RELOAD)(HPAGE_HEAD pg);

/* ҳ���ƶ�ʱ�ص����� */
typedef int(*PAGE_MOVE)(HPAGE_HEAD pg, unsigned int old_pgno, unsigned int new_pgno);


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
 * @brief ����һ������ҳ������
 */
extern void PagerDestruct(HPAGER hpgr);

/**
 * @brief ͬ�������໺��ҳ
 */
extern int PagerSyn(HPAGER hpgr);

/**
 * @brief ȡ�����е�ҳ�ĸ���
 */
extern int PagerRollBack(HPAGER hpgr);

/**
 * @brief ����һ��ҳ��
 * @return ����һ�����õĿ���ҳ��
 */
extern unsigned int PagerGetPageNo(HPAGER hpgr);

/**
 * @brief �ͷ�һ��ҳ��,���óɿ���ҳ
 */
extern int PagerReleasePageNo(HPAGER hpgr, unsigned int pgno);

/**
 * @brief ��ȡ��ǰҳ�ļ����ж���ҳ
 */
extern unsigned int PagerGetTotalPagesCount(HPAGER hpgr);

/**
 * @brief ��ȡ��ǰҳ�ļ����ж��ٿ���ҳ
 */
extern unsigned int PagerGetFreePages(HPAGER hpgr);

/**
 * @brief ��ҳ��������л�ȡһҳ ��PagerReleasePage�Ƕ�ż�Ĳ���
 */
extern HPAGE_HEAD PagerGetPage(HPAGER hpgr, unsigned int pgno);

/**
 * @brief ��һ��ҳ�������ü�������,��Ҫ��PagerReleasePage������������
 */
extern int PageHeadRef(HPAGE_HEAD pg);

/**
 * @brief �ӻ���������ָ��ҳ�ŵ�ҳ����,���ҳ���ڻ�����,����NULL
 */
extern HPAGE_HEAD PagerGetCachedPage(HPAGER hpgr, unsigned int pgno);

/**
 * @brief ȡ���Ե�ǰҳ������ ��PagerGetPage�Ƕ�ż�Ĳ���
 */
extern int PagerReleasePage(HPAGE_HEAD pg);

/**
 * @brief ��ȡҳ���ݵĻ�����,����д��
 */
extern void * PageHeadMakeWritable(HPAGE_HEAD pg);

/**
 * @brief ��ȡĳһ��ҳ����,���ڶ�ȡ
 */
extern const void * PageHeadMakeReadable(HPAGE_HEAD pg);

/**
 * @brief ��ȡĳһҳ���û�����
 */
extern void * PageHeadGetUserData(HPAGE_HEAD pg);

/**
 * @brief ��ȡĳһҳ���û����ݵĳ���
 */
extern unsigned PageHeadGetUserDataSize(HPAGE_HEAD pg);

/**
 * @brief ��ȡҳ��������������ü���
 */
extern unsigned int PagerGetRefCount(HPAGER hpgr);

/**
 * @brief ��ȡĳһҳ��������ü���
 */
extern unsigned int PagerGetPageRefCount(HPAGE_HEAD pg);

/**
 * @brief ���ĳ��ָ���Ƿ�Խ����
 */
extern int PagePtrIsInpage(HPAGE_HEAD pg, const void * ptr);

/**
 * @brief ���ҳ�����״̬
 */
extern void PagerExamin(HPAGER hpgr, int check_jf, int check_free, void * buf, unsigned int buf_sz);


#endif




















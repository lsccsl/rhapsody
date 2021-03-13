/**
* @file mymmap_win32.c 描述win32的共享内存 2008-03-25 23:25
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
#include "mymmap.h"

#include <windows.h>
#include <assert.h>


typedef struct __uos_mmap_t_
{
	/* 内存映射文件句柄 */
	HANDLE hmapfile;
	/* 文件大小 */
	unsigned int file_sz;

	/* 内存映射的起始地址 */
	void * buf;

	HMYMEMPOOL hm;
}uos_mmap_t;


/**
 * @brief 创建/打开内存映射
 */
static __INLINE__ int uos_mmap_destroy(uos_mmap_t * m)
{
	assert(m);
	if(m->buf)
		UnmapViewOfFile(m->buf);

	if(INVALID_HANDLE_VALUE != m->hmapfile)
		CloseHandle(m->hmapfile);

	MyMemPoolFree(m->hm, m);

	return 0;
}

/**
 * @brief 创建/打开内存映射
 */
HUOS_MMAP UOS_MMapCreate(HMYMEMPOOL hm, const char * file_name, unsigned int file_sz, unsigned int * real_sz)
{
	uos_mmap_t * m = NULL;

	if(NULL == file_name || 0 == file_sz)
		return NULL;

	m = MyMemPoolMalloc(hm, sizeof(*m));
	if(NULL == m)
		return NULL;

	m->hm = hm;
	m->buf = NULL;

	m->hmapfile = CreateFileMapping(INVALID_HANDLE_VALUE,
		NULL, PAGE_READWRITE, 0, file_sz, file_name);
	if(NULL == m->hmapfile)
		goto UOS_MMapCreate_err_;

	m->file_sz = file_sz;

	return m;

UOS_MMapCreate_err_:

	if(m)
		uos_mmap_destroy(m);

	return NULL;
}

HUOS_MMAP UOS_MMapOpen(HMYMEMPOOL hm, const char * file_name, unsigned int file_sz, unsigned int * real_sz)
{
	uos_mmap_t * m = NULL;

	if(NULL == file_name || 0 == file_sz)
		return NULL;

	m = MyMemPoolMalloc(hm, sizeof(*m));
	if(NULL == m)
		return NULL;

	m->hm = hm;
	m->buf = NULL;

	m->hmapfile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, file_name);
	if(NULL == m->hmapfile)
		goto UOS_MMapOpen_err_;

	m->file_sz = file_sz;

	return m;

UOS_MMapOpen_err_:

	if(m)
		uos_mmap_destroy(m);

	return NULL;
}

/**
 * @brief 销毁内存映射
 */
void UOS_MMapClose(HUOS_MMAP hmmap)
{
	if(NULL == hmmap)
		return;

	uos_mmap_destroy(hmmap);
}

/**
 * @brief 获取内存映射的指针
 */
void * UOS_MMapGetBuf(HUOS_MMAP hmmap)
{
	if(NULL == hmmap)
		return NULL;

	if(hmmap->buf)
		return hmmap->buf;

	hmmap->buf = MapViewOfFile(hmmap->hmapfile, FILE_MAP_ALL_ACCESS, 0, 0, hmmap->file_sz);

	return hmmap->buf;
}







/**
* @file mymmap_linux.c 描述linux的共享内存 2008-03-25 23:25
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

#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <termio.h>
#include <stdlib.h>
#include <errno.h>

#include "os_def.h"


typedef struct __uos_mmap_t_
{
	/* 内存映射文件句柄 */
	int fd;
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
		munmap(m->buf, m->file_sz);

	close(m->fd);

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

	m->fd = open(file_name, O_CREAT|O_RDWR|O_TRUNC, 00777);
	if(INVALID_FD == m->fd)
		goto UOS_MMapCreate_err_;

#ifdef __uClinux__
	m->buf = mmap(NULL, file_sz, PROT_READ|PROT_WRITE, 0, m->fd, 0);
#else
	m->buf = mmap(NULL, file_sz, PROT_READ|PROT_WRITE, MAP_SHARED, m->fd, 0);
#endif
	if(NULL == m->buf)
		goto UOS_MMapCreate_err_;

	lseek(m->fd,file_sz,SEEK_SET);
	write(m->fd,"",1);

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

	m->fd = open(file_name, O_CREAT|O_RDWR, 00777);
	if(INVALID_FD == m->fd)
		goto UOS_MMapCreate_err_;

#ifdef __uClinux__
	m->buf = mmap(NULL, file_sz, PROT_READ|PROT_WRITE, 0, m->fd, 0);
#else
	m->buf = mmap(NULL, file_sz, PROT_READ|PROT_WRITE, MAP_SHARED, m->fd, 0);
#endif

	if(NULL == m->buf)
		goto UOS_MMapCreate_err_;

	m->file_sz = file_sz;

	return m;

UOS_MMapCreate_err_:

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

	return hmmap->buf;
}









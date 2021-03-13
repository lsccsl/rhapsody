/**
 * @file myOsFile_linux.c 封装linux系统的文件操作接口 2008-4-14 00:43
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.
 *        封装不同系统的文件操作接口
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
#include "myOsFile.h"

#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#ifndef O_BINARY
# define O_BINARY 0
#endif


typedef struct __os_file_t_
{
	/* 文件句柄 */
	int fd;

	/* 当前文件的读写偏移 */
	int64 off_set;

	/* 内存池句柄 */
	HMYMEMPOOL hm;
}os_file_t;


/**
 * @brief 打开文件,读写
 */
HMYOSFILE myOsFileOpenReadWrite(const char * file_name, HMYMEMPOOL hm)
{
	os_file_t * f = MyMemPoolMalloc(hm, sizeof(*f));
	if(NULL == f)
		return NULL;

	f->hm = hm;
	f->off_set = 0;
	f->fd = open(file_name, O_RDWR | O_CREAT | O_BINARY, 0644);
	if(f->fd < 0)
	{
		MyMemPoolFree(hm, f);
		return NULL;
	}

	return f;
}

/**
 * @brief 以只读的方式打开文件
 */
HMYOSFILE myOsFileOpenReadOnly(const char * file_name, HMYMEMPOOL hm)
{
	os_file_t * f = MyMemPoolMalloc(hm, sizeof(*f));
	if(NULL == f)
		return NULL;

	f->hm = hm;
	f->off_set = 0;
	f->fd = open(file_name, O_RDONLY | O_BINARY);
	if(f->fd < 0)
	{
		MyMemPoolFree(hm, f);
		return NULL;
	}

	return f;
}

/**
 * @brief 以独占的方式打开文件
 */
HMYOSFILE myOsFileOpenExclusive(const char * file_name, HMYMEMPOOL hm)
{
	os_file_t * f = MyMemPoolMalloc(hm, sizeof(*f));
	if(NULL == f)
		return NULL;

	f->hm = hm;
	f->off_set = 0;
	f->fd = open(file_name, O_EXCL | O_RDWR | O_CREAT | O_BINARY, 0644);
	if(f->fd < 0)
	{
		MyMemPoolFree(hm, f);
		return NULL;
	}

	return f;
}

/**
 * @brief 关闭文件
 */
int myOsFileClose(HMYOSFILE hf)
{
	if(NULL == hf)
		return -1;

	if(0 != close(hf->fd))
		return -1;

	MyMemPoolFree(hf->hm, hf);

	return 0;
}

/**
 * @brief 同步文件至辅存
 * @return 0:成功 -1:失败
 */
int myOsFileSyn(HMYOSFILE hf)
{
	/*
	* fdatasync:只同步文件的数据,但不被某些系统支持,如果freebsd, mac os x10.3
	* fsync:功能与fdatasync类似,除了同步文件数据,还会同步文件的属性(如果文件修改时间之类的)
	* fcntl(fd, F_FULLFSYNC, 0): 似乎只被mac os x支持
	*/
	if(NULL == hf || hf->fd < 0)
		return -1;

	if(0 != fsync(hf->fd))
		return -1;

	return 0;
}


/**
 * @brief 同步文件至辅存
 * @return 0:成功 -1:失败
 */
static __INLINE__ int seek_and_write(os_file_t * f, const void * data, size_t data_size)
{
	int wrote = 0;

	assert(f && f->fd > 0);

	lseek(f->fd, f->off_set, SEEK_SET);

	wrote = write(f->fd, data, data_size);
	if(wrote > 0)
		f->off_set += wrote;

	return wrote;
}

/**
 * @brief 写文件
 * @return 0:成功 -1:失败, -2:未写满指定字节
 */
int myOsFileWrite(HMYOSFILE hf, const void * data, size_t data_size, size_t * write_size)
{
	int wrote = 0;
	size_t total_wrote = 0;

	if(NULL == hf || hf->fd < 0)
		return -1;

	if(NULL == data || 0 == data_size)
		return -1;

	while(data_size > 0 && (wrote = seek_and_write(hf, data, data_size)) > 0)
	{
		total_wrote += wrote;

		data_size -= wrote;
		data = &((unsigned char*)data)[wrote];
	}

	if(write_size)
		*write_size = total_wrote;

	if(data_size > 0)
	{
		if(wrote < 0)
			return -1;
		else
			return -2;
	}

	return 0;
}


/**
 * @brief 读文件
 * @return 0:成功 -1:失败
 */
static __INLINE__ int seek_and_read(os_file_t * f, void *pBuf, int cnt)
{
	int got;

	assert(f && f->fd > 0);

	lseek(f->fd, f->off_set, SEEK_SET);

	got = read(f->fd, pBuf, cnt);
	if(got > 0)
		f->off_set += got;

	return got;
}

/**
 * @brief 读文件
 * @return 0:成功 -1:失败
 */
int myOsFileRead(HMYOSFILE hf, void * data, size_t data_size, size_t * read_size)
{
	int got;

	if(NULL == hf || hf->fd < 0)
		return -1;

	if(NULL == data || 0 == data_size)
		return -1;

	got = seek_and_read(hf, data, data_size);

	if(got > 0 && read_size)
		*read_size = got;

	if(got == data_size)
		return 0;
	else if(got < 0)
		return -1;
	else
		return -2;
}

/**
 * @brief 移动当前的文件指针至off_set(相对于文件头)
 * @return 0:成功 -1:失败
 */
int myOsFileSeek(HMYOSFILE hf, int64 off_set)
{
	if(NULL == hf || hf->fd < 0)
		return -1;

	hf->off_set = off_set;

	return 0;
}

/**
 * @brief 删除文件
 * @return 0:成功 -1:失败
 */
int myOsFileDel(const char * file_name)
{
	if(0 != unlink(file_name))
		return -1;

	return 0;
}

/**
 * @brief 获取文件的大小
 * @return 0:成功 -1:失败
 */
int myOsFileSize(HMYOSFILE hf, int64 * file_size)
{
	int rc;
	struct stat buf;

	if(NULL == hf || hf->fd < 0)
		return -1;

	rc = fstat(hf->fd, &buf);

	if(rc != 0)
		return -1;

	*file_size = buf.st_size;

	return 0;
}

/**
 * @brief 判断文件是否存在
 * @return 0:文件不存在 非零:文件存在
 */
int myOsFileExists(const char * file_name)
{
	return access(file_name, 0)==0;
}

/**
 * @brief 栽减文件
 * @return 0:成功, -1:失败
 */
int myOsFileTruncate(HMYOSFILE hf, int64 nByte)
{
	if(NULL == hf || hf->fd < 0)
		return -1;

	if(0 != ftruncate(hf->fd, nByte))
		return -1;

	return 0;
}





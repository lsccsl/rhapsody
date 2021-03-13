/**
 * @file myOsFile_linux.c ��װlinuxϵͳ���ļ������ӿ� 2008-4-14 00:43
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.
 *        ��װ��ͬϵͳ���ļ������ӿ�
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
	/* �ļ���� */
	int fd;

	/* ��ǰ�ļ��Ķ�дƫ�� */
	int64 off_set;

	/* �ڴ�ؾ�� */
	HMYMEMPOOL hm;
}os_file_t;


/**
 * @brief ���ļ�,��д
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
 * @brief ��ֻ���ķ�ʽ���ļ�
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
 * @brief �Զ�ռ�ķ�ʽ���ļ�
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
 * @brief �ر��ļ�
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
 * @brief ͬ���ļ�������
 * @return 0:�ɹ� -1:ʧ��
 */
int myOsFileSyn(HMYOSFILE hf)
{
	/*
	* fdatasync:ֻͬ���ļ�������,������ĳЩϵͳ֧��,���freebsd, mac os x10.3
	* fsync:������fdatasync����,����ͬ���ļ�����,����ͬ���ļ�������(����ļ��޸�ʱ��֮���)
	* fcntl(fd, F_FULLFSYNC, 0): �ƺ�ֻ��mac os x֧��
	*/
	if(NULL == hf || hf->fd < 0)
		return -1;

	if(0 != fsync(hf->fd))
		return -1;

	return 0;
}


/**
 * @brief ͬ���ļ�������
 * @return 0:�ɹ� -1:ʧ��
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
 * @brief д�ļ�
 * @return 0:�ɹ� -1:ʧ��, -2:δд��ָ���ֽ�
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
 * @brief ���ļ�
 * @return 0:�ɹ� -1:ʧ��
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
 * @brief ���ļ�
 * @return 0:�ɹ� -1:ʧ��
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
 * @brief �ƶ���ǰ���ļ�ָ����off_set(������ļ�ͷ)
 * @return 0:�ɹ� -1:ʧ��
 */
int myOsFileSeek(HMYOSFILE hf, int64 off_set)
{
	if(NULL == hf || hf->fd < 0)
		return -1;

	hf->off_set = off_set;

	return 0;
}

/**
 * @brief ɾ���ļ�
 * @return 0:�ɹ� -1:ʧ��
 */
int myOsFileDel(const char * file_name)
{
	if(0 != unlink(file_name))
		return -1;

	return 0;
}

/**
 * @brief ��ȡ�ļ��Ĵ�С
 * @return 0:�ɹ� -1:ʧ��
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
 * @brief �ж��ļ��Ƿ����
 * @return 0:�ļ������� ����:�ļ�����
 */
int myOsFileExists(const char * file_name)
{
	return access(file_name, 0)==0;
}

/**
 * @brief �Լ��ļ�
 * @return 0:�ɹ�, -1:ʧ��
 */
int myOsFileTruncate(HMYOSFILE hf, int64 nByte)
{
	if(NULL == hf || hf->fd < 0)
		return -1;

	if(0 != ftruncate(hf->fd, nByte))
		return -1;

	return 0;
}





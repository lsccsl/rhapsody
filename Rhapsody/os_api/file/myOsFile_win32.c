/**
 * @file myOsFile.c ��װ��ͬϵͳ���ļ������ӿ� 2008-1-31 00:43
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
#include <windows.h>

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER -1
#endif


typedef struct __os_file_t_
{
	/* �ļ���� */
	HANDLE hfile;

	/* �ڴ�ؾ�� */
	HMYMEMPOOL hm;
}os_file_t;


/**
 * @brief ���ļ�,��д
 */
HMYOSFILE myOsFileOpenReadWrite(const char * file_name, HMYMEMPOOL hm)
{
	os_file_t * f = NULL;

	if(NULL == file_name)
		return NULL;

	f = MyMemPoolMalloc(hm, sizeof(*f));
	if(NULL == f)
		return NULL;

	f->hm = hm;
	f->hfile = CreateFile(file_name,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
		NULL);

	if(INVALID_HANDLE_VALUE == f->hfile)
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
	os_file_t * f = NULL;

	if(NULL == file_name)
		return NULL;

	f = MyMemPoolMalloc(hm, sizeof(*f));
	if(NULL == f)
		return NULL;

	f->hm = hm;
	f->hfile = CreateFile(file_name,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
		NULL);

	if(INVALID_HANDLE_VALUE == f->hfile)
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
	os_file_t * f = NULL;

	if(NULL == file_name)
		return NULL;

	f = MyMemPoolMalloc(hm, sizeof(*f));
	if(NULL == f)
		return NULL;

	f->hm = hm;
	f->hfile = CreateFile(file_name,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_FLAG_RANDOM_ACCESS,
		NULL);

	if(INVALID_HANDLE_VALUE == f->hfile)
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

	if(0 == CloseHandle(hf->hfile))
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
	if(NULL == hf || INVALID_HANDLE_VALUE == hf->hfile)
		return -1;

	if(FlushFileBuffers(hf->hfile))
		return 0;

	return -1;
}

/**
 * @brief д�ļ�
 * @return 0:�ɹ� -1:ʧ��, -2:δд��ָ���ֽ�
 */
int myOsFileWrite(HMYOSFILE hf, const void * data, size_t data_size, size_t * write_size)
{
	int rc = 0;
	DWORD wrote = 0;
	DWORD total_wrote = 0;

	if(NULL == hf || INVALID_HANDLE_VALUE == hf->hfile)
		return -1;

	assert(data_size > 0);

	while(data_size 
		&& (rc = WriteFile(hf->hfile, data, (DWORD)data_size, &wrote, 0))
		&& wrote > 0 
		&& wrote <= data_size)
	{
		data_size -= wrote;

		total_wrote += wrote;

		data = &((char*)data)[wrote];
	}

	if(!total_wrote)
		return -1;

	if(write_size)
		*write_size = total_wrote;

	if(!rc || data_size > wrote)
		return -2;

	return 0;
}

/**
 * @brief ���ļ�
 * @return 0:�ɹ� -1:ʧ��
 */
int myOsFileRead(HMYOSFILE hf, void * data, size_t data_size, size_t * read_size)
{
	DWORD got;
	if(NULL == hf || INVALID_HANDLE_VALUE == hf->hfile)
		return -1;

	if(read_size)
		*read_size = 0;

	if(!ReadFile(hf->hfile, data, (DWORD)data_size, &got, 0))
		got = 0;

	if(read_size)
		*read_size = got;

	if(got == (DWORD)data_size)
		return 0;
    
	return -1;
}

/**
 * @brief �ƶ���ǰ���ļ�ָ����off_set(������ļ�ͷ)
 * @return 0:�ɹ� -1:ʧ��
 */
int myOsFileSeek(HMYOSFILE hf, int64 off_set)
{
	DWORD rc;
	LONG upperBits = (LONG)(off_set>>32);
	LONG lowerBits = (LONG)(off_set & 0xffffffff);

	if(NULL == hf || INVALID_HANDLE_VALUE == hf->hfile)
		return -1;

	rc = SetFilePointer(hf->hfile, lowerBits, &upperBits, FILE_BEGIN);

	if(rc==INVALID_SET_FILE_POINTER && GetLastError()!=NO_ERROR)
		return -1;

	return 0;
}

/**
 * @brief ɾ���ļ�
 * @return 0:�ɹ� -1:ʧ��
 */
int myOsFileDel(const char * file_name)
{
	if(NULL == file_name)
		return -1;

	if(0 == DeleteFile(file_name))
		return -1;

	return 0;
}

/**
 * @brief ��ȡ�ļ��Ĵ�С
 * @return 0:�ɹ� -1:ʧ��
 */
int myOsFileSize(HMYOSFILE hf, int64 * file_size)
{
	DWORD upperBits, lowerBits;

	if(NULL == hf || NULL == file_size)
		return -1;

	lowerBits = GetFileSize(hf->hfile, &upperBits);
	*file_size = (((int64)upperBits)<<32) + lowerBits;

	return 0;
}

/**
 * @brief �ж��ļ��Ƿ����
 * @return 0:�ļ������� ����:�ļ�����
 */
int myOsFileExists(const char * file_name)
{
	return GetFileAttributes(file_name) != 0xffffffff;
}

/**
 * @brief �Լ��ļ�
 * @return 0:�ɹ�, -1:ʧ��
 */
int myOsFileTruncate(HMYOSFILE hf, int64 nByte)
{
	LONG upperBits = (LONG)(nByte>>32);

	if(NULL == hf)
		return -1;

	SetFilePointer(hf->hfile, (LONG)nByte, &upperBits, FILE_BEGIN);
	SetEndOfFile(hf->hfile);

	return 0;
}
























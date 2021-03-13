/**
 * @file myOsFile.h ��װ��ͬϵͳ���ļ������ӿ� 2008-1-30 00:43
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
#ifndef __MYOSFILE_H__
#define __MYOSFILE_H__


#include "myconfig.h"
#include "mymempool.h"


struct __os_file_t_;
typedef struct __os_file_t_ * HMYOSFILE;


/**
 * @brief ���ļ�,��д
 */
extern HMYOSFILE myOsFileOpenReadWrite(const char * file_name, HMYMEMPOOL hm);

/**
 * @brief ��ֻ���ķ�ʽ���ļ�
 */
extern HMYOSFILE myOsFileOpenReadOnly(const char * file_name, HMYMEMPOOL hm);

/**
 * @brief �Զ�ռ�ķ�ʽ���ļ�
 */
extern HMYOSFILE myOsFileOpenExclusive(const char * file_name, HMYMEMPOOL hm);

/**
 * @brief �ر��ļ�
 */
extern int myOsFileClose(HMYOSFILE hf);

/**
 * @brief ͬ���ļ�������
 * @return 0:�ɹ� -1:ʧ��
 */
extern int myOsFileSyn(HMYOSFILE hf);

/**
 * @brief д�ļ�
 * @return 0:�ɹ� -1:ʧ��, -2:δд��ָ���ֽ�
 */
extern int myOsFileWrite(HMYOSFILE hf, const void * data, size_t data_size, size_t * write_size);

/**
 * @brief ���ļ�
 * @return 0:�ɹ� -1:ʧ��
 */
extern int myOsFileRead(HMYOSFILE hf, void * data, size_t data_size, size_t * read_size);

/**
 * @brief �ƶ���ǰ���ļ�ָ����off_set(������ļ�ͷ)
 * @return 0:�ɹ� -1:ʧ��
 */
extern int myOsFileSeek(HMYOSFILE hf, int64 off_set);

/**
 * @brief ɾ���ļ�
 * @return 0:�ɹ� -1:ʧ��
 */
extern int myOsFileDel(const char * file_name);

/**
 * @brief ��ȡ�ļ��Ĵ�С
 * @return 0:�ɹ� -1:ʧ��
 */
extern int myOsFileSize(HMYOSFILE hf, int64 * file_size);

/**
 * @brief �ж��ļ��Ƿ����
 * @return 0:�ļ������� ����:�ļ�����
 */
extern int myOsFileExists(const char * file_name);

/**
 * @brief �Լ��ļ�
 * @return 0:�ɹ�, -1:ʧ��
 */
extern int myOsFileTruncate(HMYOSFILE hf, int64 nByte);


#endif

























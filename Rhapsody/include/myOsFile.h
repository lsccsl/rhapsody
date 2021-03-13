/**
 * @file myOsFile.h 封装不同系统的文件操作接口 2008-1-30 00:43
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
#ifndef __MYOSFILE_H__
#define __MYOSFILE_H__


#include "myconfig.h"
#include "mymempool.h"


struct __os_file_t_;
typedef struct __os_file_t_ * HMYOSFILE;


/**
 * @brief 打开文件,读写
 */
extern HMYOSFILE myOsFileOpenReadWrite(const char * file_name, HMYMEMPOOL hm);

/**
 * @brief 以只读的方式打开文件
 */
extern HMYOSFILE myOsFileOpenReadOnly(const char * file_name, HMYMEMPOOL hm);

/**
 * @brief 以独占的方式打开文件
 */
extern HMYOSFILE myOsFileOpenExclusive(const char * file_name, HMYMEMPOOL hm);

/**
 * @brief 关闭文件
 */
extern int myOsFileClose(HMYOSFILE hf);

/**
 * @brief 同步文件至辅存
 * @return 0:成功 -1:失败
 */
extern int myOsFileSyn(HMYOSFILE hf);

/**
 * @brief 写文件
 * @return 0:成功 -1:失败, -2:未写满指定字节
 */
extern int myOsFileWrite(HMYOSFILE hf, const void * data, size_t data_size, size_t * write_size);

/**
 * @brief 读文件
 * @return 0:成功 -1:失败
 */
extern int myOsFileRead(HMYOSFILE hf, void * data, size_t data_size, size_t * read_size);

/**
 * @brief 移动当前的文件指针至off_set(相对于文件头)
 * @return 0:成功 -1:失败
 */
extern int myOsFileSeek(HMYOSFILE hf, int64 off_set);

/**
 * @brief 删除文件
 * @return 0:成功 -1:失败
 */
extern int myOsFileDel(const char * file_name);

/**
 * @brief 获取文件的大小
 * @return 0:成功 -1:失败
 */
extern int myOsFileSize(HMYOSFILE hf, int64 * file_size);

/**
 * @brief 判断文件是否存在
 * @return 0:文件不存在 非零:文件存在
 */
extern int myOsFileExists(const char * file_name);

/**
 * @brief 栽减文件
 * @return 0:成功, -1:失败
 */
extern int myOsFileTruncate(HMYOSFILE hf, int64 nByte);


#endif

























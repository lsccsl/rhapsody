/**
* @file mymmap.h 描述进程间共享内存 2008-03-25 23:25
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
#ifndef __MYMMAP_H__
#define __MYMMAP_H__


#include "mymempool.h"


struct __uos_mmap_t_;
typedef struct __uos_mmap_t_ * HUOS_MMAP;


/**
 * @brief 创建/打开内存映射
 */
extern HUOS_MMAP UOS_MMapCreate(HMYMEMPOOL hm, const char * file_name, unsigned int file_sz, unsigned int * real_sz);
extern HUOS_MMAP UOS_MMapOpen(HMYMEMPOOL hm, const char * file_name, unsigned int file_sz, unsigned int * real_sz);

/**
 * @brief 销毁内存映射
 */
extern void UOS_MMapClose(HUOS_MMAP hmmap);

/**
 * @brief 获取内存映射的指针
 */
extern void * UOS_MMapGetBuf(HUOS_MMAP hmmap);


#endif












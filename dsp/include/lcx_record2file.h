/**
 * @file lcx_record2file.h wrapper os audio input read 2006-12-27 23:26
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
#ifndef __LCX_RECORD2FILE_H__
#define __LCX_RECORD2FILE_H__


#include "lin_config.h"


struct __wave_file_t_;
typedef struct __wave_file_t_ * HWAVEFILE;

/**
 * @brief 打开文件
 */
extern DLLEXPORT HWAVEFILE lcx_wave_file_open(const char * filename, unsigned int sampleRate, unsigned int bitWidth, unsigned int channels);

/**
 * @brief 关闭文件
 */
extern DLLEXPORT void lcx_wave_file_close(HWAVEFILE hwf);

/**
 * @brief 关闭文件
 */
extern DLLEXPORT int lcx_wave_file_append_data(HWAVEFILE hwf, const void * data, unsigned int data_sz);


#endif




















/**
 * @file myrand.h 随机数发生器 2008-02-18 21:59
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
#ifndef __MYRAND_H__
#define __MYRAND_H__


#include "mymempool.h"

struct __myrand_t_;
typedef struct __myrand_t_ * HMYRAND;


/**
 * @brief 创建随机数发生器
 * @param bshare:是否多线程共享
 */
extern HMYRAND myrandConstruct(HMYMEMPOOL hm, void * rand_seed, size_t rand_seed_len, int bshare);

/**
 * @brief 销毁随机数发生器
 */
extern void myrandDestruct(HMYRAND hr);

/**
 * @brief 获取一个8bit的随机数
 */
extern unsigned char myrandGetByte(HMYRAND hr);

/**
 * @brief 获取初始化随机种子
 * @return 0:成功 -1:失败
 */
extern int myrandSeed(void * rand_seed, size_t rand_seed_size);


#endif












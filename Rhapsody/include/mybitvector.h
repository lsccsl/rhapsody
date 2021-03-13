/**
 * @file mybitvector.h 位数组 2008-2-11 21:34
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
#ifndef __MYBITVECTOR_H__
#define __MYBITVECTOR_H__


#include "mymempool.h"


/* 句柄定义 */
struct __mybitvector_t_;
typedef struct __mybitvector_t_ * HMYBITVECTOR;


/**
 * @brief 创建一个位数组
 * @param hm:内存池句柄
 * @param vector_size:数组的初始大小,表示有多少bit
 * @param ini_val:位的初始值定义(0:0, 非零:1)
 */
extern HMYBITVECTOR mybitVectorConstruct(HMYMEMPOOL hm, const size_t vector_size, const unsigned int ini_val);

/**
 * @brief 销毁位数组
 */
extern void mybitVectorDestruct(HMYBITVECTOR hbv);

/**
 * @brief 设置某一个pos的值
 * @param val:位的值 0:0 非零:1
 * @param pos:位置,表示位偏移
 */
extern int mybitVectorSetbit(HMYBITVECTOR hbv, const unsigned int val, const size_t pos);

/**
 * @brief 获取某一个pos的值
 * @param pos:位置,表示位偏移
 * @return 返指定位的值,如果指定位不存在,则返回初始值定义,即用户在创建时传入的ini_val的值(0:0, 非零:1, 失败:-1)
 */
extern int mybitVectorGetbit(HMYBITVECTOR hbv, const size_t pos);

/**
 * @brief 获取某一个pos的值,并判断是否越界访问(并不一定超过初始位数组大小就越界,mybitVectorSetbit的最大pos为标准)
 * @param pos:位置,表示位偏移
 * @return 返指定位的值(0:0, 非零:1, 越界:-1)
 */
extern int mybitVectorGetbitAndJudgeOver(HMYBITVECTOR hbv, const size_t pos);

/**
 * @brief 重设位数组的大小
 * @param vector_size:数组,表示有多少bit,vector_size比缓冲区小,多余的bit位信息丢失(变为ini_val)
 */
extern int mybitVectorSetSize(HMYBITVECTOR hbv, const size_t vector_size);

/**
 * @brief 打印出数组
 */
extern int mybitVectorPrint(HMYBITVECTOR hbv);


#endif
















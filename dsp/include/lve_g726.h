/**
 * @file lve_g726.h g726 code/decode 2010-08-06 23:51
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
#ifndef __LVE_G726_H__
#define __LVE_G726_H__

#include "lin_config.h"

struct __g726_encode_state_t_;
typedef struct __g726_encode_state_t_ * HG726ENCODE;

typedef struct __g726_encode_state_t_ * HG726DECODE;


/**
 * @brief 生成726编码状态机
 */
extern DLLEXPORT HG726ENCODE lve_gen_g726_encode();

/**
 * @brief 销毁726编码状态机
 */
extern DLLEXPORT void lve_destroy_g726_encode(HG726ENCODE hstate);

/**
 * @brief 726编码,将输入的 8bit ulaw / alaw转换成4bit的压缩包
 * @param inp_buf:输入的 ulaw / alaw码
 * @param out_buf:输出的4bit差分压缩码
 * @param buf_sz:缓冲区大小
 * @param is_alaw:1:输入是alaw 0:ulaw
 * @param reset:是否将寄存器置成初始状态
 * @param hstate:编码状态机
 */
extern DLLEXPORT void lve_G726_encode(short * inp_buf, short *out_buf, long buf_sz,
	int is_alaw, HG726ENCODE hstate);


/**
 * @brief 生成726解码状态机,两者是相同的
 */
#define lve_gen_g726_decode lve_gen_g726_encode

/**
 * @brief 销毁726解码状态机
 */
#define lve_destroy_g726_decode lve_destroy_g726_encode

/**
 * @brief 726解码,将输入4bit的压缩包转换成8bit ulaw / alaw
 * @param inp_buf:输入的4bit差分压缩码
 * @param out_buf:输出的 ulaw / alaw码
 * @param buf_sz:缓冲区大小
 * @param is_alaw:1:输出是alaw 0:ulaw
 * @param reset:是否将寄存器置成初始状态
 * @param hstate:解码状态机
 */
extern DLLEXPORT void lve_G726_decode(short *inp_buf, short *out_buf, long buf_sz,
	int is_alaw, HG726DECODE hstate);

#endif












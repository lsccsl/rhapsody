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
 * @brief ����726����״̬��
 */
extern DLLEXPORT HG726ENCODE lve_gen_g726_encode();

/**
 * @brief ����726����״̬��
 */
extern DLLEXPORT void lve_destroy_g726_encode(HG726ENCODE hstate);

/**
 * @brief 726����,������� 8bit ulaw / alawת����4bit��ѹ����
 * @param inp_buf:����� ulaw / alaw��
 * @param out_buf:�����4bit���ѹ����
 * @param buf_sz:��������С
 * @param is_alaw:1:������alaw 0:ulaw
 * @param reset:�Ƿ񽫼Ĵ����óɳ�ʼ״̬
 * @param hstate:����״̬��
 */
extern DLLEXPORT void lve_G726_encode(short * inp_buf, short *out_buf, long buf_sz,
	int is_alaw, HG726ENCODE hstate);


/**
 * @brief ����726����״̬��,��������ͬ��
 */
#define lve_gen_g726_decode lve_gen_g726_encode

/**
 * @brief ����726����״̬��
 */
#define lve_destroy_g726_decode lve_destroy_g726_encode

/**
 * @brief 726����,������4bit��ѹ����ת����8bit ulaw / alaw
 * @param inp_buf:�����4bit���ѹ����
 * @param out_buf:����� ulaw / alaw��
 * @param buf_sz:��������С
 * @param is_alaw:1:�����alaw 0:ulaw
 * @param reset:�Ƿ񽫼Ĵ����óɳ�ʼ״̬
 * @param hstate:����״̬��
 */
extern DLLEXPORT void lve_G726_decode(short *inp_buf, short *out_buf, long buf_sz,
	int is_alaw, HG726DECODE hstate);

#endif












/**
 * @file lve_g711.c g711 code/decode 2006-12-27 23:26
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
#include "lve_g711.h"


#define G711_BIAS (0x84)
#define G711_MAX_SIGN_VAL (0x1FFF - (G711_BIAS >> 2))


/* 大刻度区间 */
static int seg_uend[8] = {0x3F, 0x7F, 0xFF, 0x1FF,
	0x3FF, 0x7FF, 0xFFF, 0x1FFF};

static int search_seg(int val)
{
	unsigned char i = 0;

	for(; i < sizeof(seg_uend) / sizeof(seg_uend[0]); i ++)
	{
		if(val <= seg_uend[i])
			return i;
	}

	return (sizeof(seg_uend) / sizeof(seg_uend[0]));
}

/**
 * @brief g711 encode
 */
int lve_g711_line2ulaw(int frame)
{
	/*
	* g711压缩原理
	*
	* 值号值首先滤掉了低二位字节(g711解码后的值号值都能被4整除)
	* 值号值是正还是负?选择适的的mask值(0x7f或者0xff,以便解码时对正负值能统一处理)
	* 去除一些过高的值,如大于0x1fdf,(0x1fdf + BIAS >> 2刚好溢出了)
	* 信号值加上BIAS>>2,以免,当信号值低于0x3f时,无法判断它的最高位是0,还是1
	* 匹配信号值处于哪个刻度区间,将区间的值(0 - 7)置于压缩码的次高3位,最高1位为符号位
	* 取出信号值的次高4位,存于压缩码的低4位
	*
	* 各bit位含义如下
	*	-------------------------
	*   | 0 |  1 2 3 |  4 5 6 7 |
	*     |    --|--    ---|----
	*   符号位 大刻度  次高的4位细节
	*
	* 通过匹配大刻度区间,保留高bit位,忽略低bit位对信号的影响(随着信号值的变大,bit特位数也增多),从而g711达到了压缩的目的.
	* 如 11 中的个位 1 与 10001 中的个位数1,在信号值的比重是不同的,10001基本可以认为等同于10000,而11则不能等同于10.
	* 压缩比为2:1
	* 由压缩的过程可见,当信号低于5位时,会被对齐到4,
	* 当信号高于5bit时 设信号bit位数为 S, 则被忽略的低bit位数为 S - 5,
	* 原始信号为15bit(除掉符号位) 压缩信号为 7bit,
	* 15bit 因为统一忽略最低2bit,压缩是针对高13bit,即最多丢失的bit位为13 - 5 = 8,也就是大刻度区间元素的个数.
	*/

	int mask = 0xff;
	int seg = 0;

	frame = frame >> 2;
	if(frame < 0)
	{
		/* 如果是负值,转成正值处理 */
		frame = -frame;
		mask = 0x7f;
	}

	/* 截取信号的最大值 */
	if(frame > G711_MAX_SIGN_VAL)
		frame = G711_MAX_SIGN_VAL;

	/* 增强信号,在0x3f这个刻度以上时,最高位能统一成1 */
	frame += G711_BIAS >> 2;

	/* 匹配信号所处的刻度区间 */
	seg = search_seg(frame);

	/* 将seg存入压缩码的次高3位,保原始码的次高4位存入压缩码的低4位,并对符号位做处理. */
	if(seg >=  8)
		return (0x7f ^ mask);
	else
		return ((seg << 4) | (frame >> (seg + 1)) & 0xf) ^ mask;
}

/**
 * @brief g711 decode
 */
int lve_g711_ulaw2line(int frame)
{
	int ret = 0;

	/* 得到符号位(最高的那个bit,以及另外压缩过的7位原始值) */
	frame = ~frame;

	/* 取出低四位,补上最高位,与信号补偿位 */
	ret = ((frame & 0xf) << 3) + G711_BIAS;

	/* 取出次高3位,并移动,seg + 3,此时已经还原了绝对值 */
	ret <<= ((frame & 0x70) >> 4);

	/* 最后判断符号位 */
	return (frame & 0x80) ? (G711_BIAS - ret) : (ret - G711_BIAS);
}















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


/* ��̶����� */
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
	* g711ѹ��ԭ��
	*
	* ֵ��ֵ�����˵��˵Ͷ�λ�ֽ�(g711������ֵ��ֵ���ܱ�4����)
	* ֵ��ֵ�������Ǹ�?ѡ���ʵĵ�maskֵ(0x7f����0xff,�Ա����ʱ������ֵ��ͳһ����)
	* ȥ��һЩ���ߵ�ֵ,�����0x1fdf,(0x1fdf + BIAS >> 2�պ������)
	* �ź�ֵ����BIAS>>2,����,���ź�ֵ����0x3fʱ,�޷��ж��������λ��0,����1
	* ƥ���ź�ֵ�����ĸ��̶�����,�������ֵ(0 - 7)����ѹ����Ĵθ�3λ,���1λΪ����λ
	* ȡ���ź�ֵ�Ĵθ�4λ,����ѹ����ĵ�4λ
	*
	* ��bitλ��������
	*	-------------------------
	*   | 0 |  1 2 3 |  4 5 6 7 |
	*     |    --|--    ---|----
	*   ����λ ��̶�  �θߵ�4λϸ��
	*
	* ͨ��ƥ���̶�����,������bitλ,���Ե�bitλ���źŵ�Ӱ��(�����ź�ֵ�ı��,bit��λ��Ҳ����),�Ӷ�g711�ﵽ��ѹ����Ŀ��.
	* �� 11 �еĸ�λ 1 �� 10001 �еĸ�λ��1,���ź�ֵ�ı����ǲ�ͬ��,10001����������Ϊ��ͬ��10000,��11���ܵ�ͬ��10.
	* ѹ����Ϊ2:1
	* ��ѹ���Ĺ��̿ɼ�,���źŵ���5λʱ,�ᱻ���뵽4,
	* ���źŸ���5bitʱ ���ź�bitλ��Ϊ S, �򱻺��Եĵ�bitλ��Ϊ S - 5,
	* ԭʼ�ź�Ϊ15bit(��������λ) ѹ���ź�Ϊ 7bit,
	* 15bit ��Ϊͳһ�������2bit,ѹ������Ը�13bit,����ඪʧ��bitλΪ13 - 5 = 8,Ҳ���Ǵ�̶�����Ԫ�صĸ���.
	*/

	int mask = 0xff;
	int seg = 0;

	frame = frame >> 2;
	if(frame < 0)
	{
		/* ����Ǹ�ֵ,ת����ֵ���� */
		frame = -frame;
		mask = 0x7f;
	}

	/* ��ȡ�źŵ����ֵ */
	if(frame > G711_MAX_SIGN_VAL)
		frame = G711_MAX_SIGN_VAL;

	/* ��ǿ�ź�,��0x3f����̶�����ʱ,���λ��ͳһ��1 */
	frame += G711_BIAS >> 2;

	/* ƥ���ź������Ŀ̶����� */
	seg = search_seg(frame);

	/* ��seg����ѹ����Ĵθ�3λ,��ԭʼ��Ĵθ�4λ����ѹ����ĵ�4λ,���Է���λ������. */
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

	/* �õ�����λ(��ߵ��Ǹ�bit,�Լ�����ѹ������7λԭʼֵ) */
	frame = ~frame;

	/* ȡ������λ,�������λ,���źŲ���λ */
	ret = ((frame & 0xf) << 3) + G711_BIAS;

	/* ȡ���θ�3λ,���ƶ�,seg + 3,��ʱ�Ѿ���ԭ�˾���ֵ */
	ret <<= ((frame & 0x70) >> 4);

	/* ����жϷ���λ */
	return (frame & 0x80) ? (G711_BIAS - ret) : (ret - G711_BIAS);
}















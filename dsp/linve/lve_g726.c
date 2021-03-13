/**
 * @file lve_g726.c g726 code/decode 2010-08-06 23:51
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
#include "lve_g726.h"
#include "lve_g711.h"
#include <stdlib.h>
#include <math.h>

/**
 * @brief ����״̬��
 */
typedef struct __g726_encode_state_t_
{
	/**
	* @brief ����ǰ�����ؽ��ź���ǰ6�����������źŲ�ֵ
	* �洢�ĸ�ʽΪ <1,4,6> ��1:Ϊ����λ 4:Ϊ�� 6:Ϊβ�� [-32768] - [+32767] ֮��
	*/
	int sr0, sr1;

	/**
	 * @brief ����Ԥ��ϵ�� ����ʱ[-3] - [+3]֮��,Ӧ����ֵ�Ŵ�2^14λ a2������Ϊ����ֵС��0.75,a1��ֵҲ������
	 */
	int a1, a2;
	int b1, b2, b3, b4, b5, b6;

	/**
	 * @brief ����ǰ�����ؽ��ź���ǰ6�����������źŲ�ֵ
	 * �洢�ĸ�ʽΪ <1,4,6> ��1:Ϊ����λ 4:Ϊ�� 6:Ϊβ�� [-32768] - [+32767] ֮��
	 */
	int dq5, dq4, dq3, dq2, dq1, dq0;

	/**
	* @brief ������ֵ�źŶ�ʱƽ��,����f[i[n]]����һ����ͨ�˲����õ�
	*/
	int dmsp;
	/**
	* @brief ������ֵ�źų�ʱƽ��,����f[i[n]]����һ����ͨ�˲����õ�,����˥���ñ�dmsp����
	*/
	int dmlp;

	/**
	* @brief ��������Ӧ�ٶȿ�������,���ֵ�ڼ����ʱ��������256��
	*/
	int apr;

	/**
	 * @brief ��������Ӧ��������,����һ����ʱ,�ڼ���ʱ,���ֵ��������512��,���ֵ��������,��Լ��5120��544֮��
	 */
	int yup;

	/**
	* @brief ��Ƶ�źż��,������fsk���,��a2����ĳ��ֵʱ,ȡֵΪ1. �� (b1* Z^-1 + ... + b6 * Z^-6)/(a2 * Z^-2 + a1 * Z^-1) ϵͳ�����ǵ�a2, 
	*/
	int tdr;

	/**
	* @brief ������Ϊdq + sezֵ�ķ���,����������Ԥ��ϵ����ֵʱ�ļ���(�������ݶ��㷨�����õ�������ֵ����ʱ)
	*/
	int pk0, pk1;

	/**
	 * @brief ��������Ӧ��������,����һ����ʱ,�ڼ���ʱ,���ֵ��������512 * 64(2^15)��,������yup��64��,�����˲���ʽ�й�,��64�����ò��漰��С����֮�������
	 */
	int ylp;
}g726_encode_state_t;


/**
 * @brief ����726����״̬��
 */
HG726ENCODE lve_gen_g726_encode()
{
	/**
	 * �������������мĴ����ĳ�ֵ
	 */

	g726_encode_state_t * state = (g726_encode_state_t *)malloc(sizeof(*state));

	/*
	 * ����Ԥ��ϵ��ȫ���ó�0
	 */
	state->a1 = 0;
	state->a2 = 0;

	state->b1 = 0;
	state->b2 = 0;
	state->b3 = 0;
	state->b4 = 0;
	state->b5 = 0;
	state->b6 = 0;

	/*
	 * ��ʷ�ؽ��ź�ֵ�Լ���������ֵ,�Ӽ�������п��Կ���32��ʵ����0
	 */
	state->sr0 = 32;
	state->sr1 = 32;

	state->dq0 = 32;
	state->dq1 = 32;
	state->dq2 = 32;
	state->dq3 = 32;
	state->dq4 = 32;
	state->dq5 = 32;

	/**
	 * @brief ��/��������Ӧ�������Ӹ���ֵ
	 */
	state->yup = 544;/* �Ӽ�������п�����ֵΪyup��ȡֵ���� */
	state->ylp = 34816;/* �� 544 * 64,�Ϳ��Եõ������ֵ,�����˲���ʽ������������64�������ܱ���һ��С����֮������� */


	/*
	 * pk0 pk1��ʼ��
	 */
	state->pk0 = 0;
	state->pk1 = 0;

	/*
	 * ����ʱƽ�� ����Ӧ�ٶȿ��� ������ⴥ�� ��0
	 */
	state->dmsp = 0;
	state->dmlp = 0;
	state->apr = 0;
	state->tdr = 0;

	return state;
}

/**
 * @brief ����726����״̬��
 */
void lve_destroy_g726_encode(HG726ENCODE hstate)
{
	if(hstate)
		free(hstate);
}

/**
 * @brief ������ֵ��4bit(1 + 3)
 */
static void __quan_dln(int dln, int ds, int * i)
{ 	
 	if (dln >= 400)
 		*i = 7;
 	else if (dln >= 349)
 		*i = 6;
 	else if (dln >= 300)
 		*i = 5;
 	else if (dln >= 246)
 		*i = 4;
 	else if (dln >= 178)
 		*i = 3;
 	else if (dln >= 80)
 		*i = 2;
	else if (dln >= -124)
		*i = 1;
 	else
 		*i = 0;

	/* Adjust for sign */
 	if (ds)
 		*i = 15 - *i;
}

/**
 * @brief ����i���������
 */
static void __anti_quan(int i, int * dqln, int * dqs)
{
	/* �������ֵ,����itu��׼����ڹ�128�� */
	static int tab[16] = {-2048, 4, 135, 213, 273, 323, 373, 425,
		425, 373, 323, 273, 213, 135, 4, -2048};
	
	/* Extract sign */
	*dqs = (i >> 3);
	
	/* Table look-up */
	*dqln = tab[i];
}

/**
 * @brief ������2Ϊ�׵Ķ���, (4 + 7) ���������Ŵ��� 2^7(128)��
 */
static void __log(int d, int * dl, int * ds)
{
	int dmag = 0;
	int exp = 0;
	int mant = 0;

	/* ȡ������ֵ�����λ */
	*ds = (d > 0) ? 0 : 1;
	dmag = (d > 0) ? d : (-d);

	/* ����log2����������,��������ֵ�ֳ� 2^exp * (1 + 0.x) */
	if (dmag >= 16384)
		exp = 14;
	else if (dmag >= 8192)
		exp = 13;
	else if (dmag >= 4096)
		exp = 12;
	else if (dmag >= 2048)
		exp = 11;
	else if (dmag >= 1024)
		exp = 10;
	else if (dmag >= 512)
		exp = 9;
	else if (dmag >= 256)
		exp = 8;
	else if (dmag >= 128)
		exp = 7;
	else if (dmag >= 64)
		exp = 6;
	else if (dmag >= 32)
		exp = 5;
	else if (dmag >= 16)
		exp = 4;
	else if (dmag >= 8)
		exp = 3;
	else if (dmag >= 4)
		exp = 2;
	else if (dmag >= 2)
		exp = 1;
	else
		exp = 0;

	/* ����log(1 + x) Լ����x,����С������ */
	mant = (((dmag << 7) >> exp) & 0x7f); /* ����С�����7λ */

	/* ��ϳɶ����� ��7λΪС�� */
	*dl = (exp << 7) + mant;
}

/**
 * @brief ��������� 
 * @param s:Ϊ(1,4,6) 
 * @param a:Ϊ�Ŵ��ϵ��
 * @param ret:����ֵ
 */
static void __fmult(int a, int s, int * ret)
{
	int amag = 0;
	int as = 0;
	int aexp = 0;
	int amant = 0;

	int ss = 0;
	int smant = 0;
	int sexp = 0;

	int rs = 0;
	int rexp = 0;
	int rmant = 0;

	/* a�ķ���λ */
	as = (a > 0) ? 0 : 1;

	/* ����a�Ķ��� */
	/* amagΪa�ľ���ֵ */
	amag = (a > 0) ? (a) : ((-a) & 0xffff);

	/* ��amag�Ķ��� */
	if (amag >= 162384)
		aexp = 15;
	else if (amag >= 8192)
		aexp = 14;
	else if (amag >= 4096)
		aexp = 13;
	else if (amag >= 2048)
		aexp = 12;
	else if (amag >= 1024)
		aexp = 11;
	else if (amag >= 512)
		aexp = 10;
	else if (amag >= 256)
		aexp = 9;
	else if (amag >= 128)
		aexp = 8;
	else if (amag >= 64)
		aexp = 7;
	else if (amag >= 32)
		aexp = 6;
	else if (amag >= 16)
		aexp = 5;
	else if (amag >= 8)
		aexp = 4;
	else if (amag >= 4)
		aexp = 3;
	else if (amag >= 2)
		aexp = 2;
	else if (amag == 1)
		aexp = 1;
	else
		aexp = 0;

	/* ȡ��a��6λβ�� */
	amant = (amag << 6) >> aexp;

	/* ��s��(1,4,6)��ʽ��� */
	ss = s >> 10;
	sexp = ((s >> 6) & 0xf);
	smant = (s & (0x3f));

	/* ָ����� */
	rexp = aexp + sexp ;
	/* β�����,6 + 6 β����߿ɴ� 12-4=8 λ */
	rmant = ((amant * smant) + 0xc0) >> 4;

	/* ������ */
	/* β����ָ���ƶ�, 8(β��) + 14(a����������2^14) = 22 ��2^22��,����ֻ����2^21��,�����ս��ֵ��ԭ����2�� */
	*ret = (rexp > 21) ? (((rmant) << (rexp - 21)) & 0xffff) : ((rmant) >> (21 - rexp));
	/* �������λ */
	rs = as ^ ss;

	*ret = (rs == 0) ? (*ret) : (-(*ret));
}

/**
 * @brief �����ֵ�ź�
 */
static void __cal_se(int wa1, int wa2, int wb1, int wb2, int wb3, int wb4, int wb5, int wb6, int * se, int * sez)
{
	*sez = wb1 + wb2 + wb3 + wb4 + wb5 + wb5 + wb6;

	*se = *sez + wa1 + wa2;
	
	*se = *se >> 1;
	*sez = *sez >> 1;
}

/**
 * @brief ����ap��al
 */
static void __limt_al(int ap, int * al)
{
	*al = (ap >= 256) ? 64 : (ap >> 2);
}

/**
 * @brief ��alΪ�������ӻ�Ͽ�����������Ӧ��������,�����Ϻ������Ӧ��������
 * ��ʽ y=al * yu[n - 1] + (1 - al)yl[n - 1]
 */
static void __mix_scale_factor(int yl, int yu, int al, int * y)
{
	int diff = 0;
	diff = yu - (yl >> 6);

	diff = diff * al;
	diff = diff >> 6;

	*y = (yl >> 6) + diff;
}

/**
 * @brief �ƴ� dl �� y�Ĳ�ֵ,�õ�������Ķ���dln
 */
static void __cal_dln(int dl, int y, int * dln)
{
	*dln = dl - (y >> 2);
}

/**
 * @brief 
 */
static void __cal_dql(int dqln, int y, int * dql)
{
	*dql = dqln + (y >> 2);
}

/**
 * @brief ������ f(i[n])
 */
static void __function_f(int i, int * fi)
{
	/* Initialized data */
	static int tab[8] = {0, 0, 0, 1, 1, 1, 3, 7};

	int im, is;
	
	is = (i >> 3);
	
	im = (is == 0) ? (i & 0xf) : ((15 - i) & 0xf);
	
	*fi = tab[im];
}

/**
 * @brief ������ w(i[n])
 */
static void __function_w(int i, int * wi)
{
	/* Initialized data */
	static int tab[8] = {-12, 18, 41, 64, 112, 198, 355, 1122};
	int im, is;
	
	is = (i >> 3);
	
	im = (is == 0) ? (i & 0xf) : ((15 - i) & 0xf);
	
	/* Scale factor multiplier */
	/* �����Ѿ�������16�� */
	*wi = tab[im];
}

/**
 * @brief ����������(��2Ϊ��)
 * @param dql:dq�Ķ���,ע�����ǷŴ���2^7=128����,����7λ��С��
 * @param dqs:dq�ķ���λ
 * @param dq:����ķ�������ֵ,Ϊ����������ԭֵ
 */
static void __anti_log(int dql, int dqs, int * dq)
{
	/* ��itu�ı�׼�ĵ����ṩ�ı����Կ���,dql�ǿ���Ϊ����(��i=0ʱ,����Ǹ�����) */
	int exp = 0;
	int mant = 0;

	if(dql > 0)
	{
		/* ȡ���������� */
		exp = (dql >> 7);

		/* С������, ��xС��1ʱ 2^(x) Լ���� (1 + x) */
		mant = (dql & 0x7f) + 128;

		/* mant << exp >> 7,��ô��������Ϊ�˷�ֹһЩλ����û�� */
		mant = (mant << 7) >> (14 - exp);

		/* �����λ��ϴ������ */
		*dq = (dqs == 0) ? mant : (-mant);
	}
	else
	{
		/* 2�ĸ�����η�,ֱ����Ϊ���� */
		*dq = 0;
	}
}

/**
 * @brief �����(1,4,6)��ʽ�ĸ����� 1:����λ 4:�� 6:β��
 */
static void __pack_146_float(int f_org, int * f_pack, int bdq)
{
	int mant = 0;
	int s = 0;
	int exp = 0;
	int mag = 0;

	/* ȡ������λ */
	s = (f_org > 0) ? 0 : 1;
	/* �������ֵ */
	mag = (s == 0) ? f_org : (-f_org);
	
	/* ����� */
	if (mag >= 16384 && !bdq)
		exp = 15;
	else if (mag >= 8192)
		exp = 14;
	else if (mag >= 4096)
		exp = 13;
	else if (mag >= 2048)
		exp = 12;
	else if (mag >= 1024)
		exp = 11;
	else if (mag >= 512)
		exp = 10;
	else if (mag >= 256)
		exp = 9;
	else if (mag >= 128)
		exp = 8;
	else if (mag >= 64)
		exp = 7;
	else if (mag >= 32)
		exp = 6;
	else if (mag >= 16)
		exp = 5;
	else if (mag >= 8)
		exp = 4;
	else if (mag >= 4)
		exp = 3;
	else if (mag >= 2)
		exp = 2;
	else if (mag == 1)
		exp = 1;
	else
		exp = 0;
	
	/* ����β�� */
	mant = ((mag << 6) >> exp);
	
	/* ��� */
	*f_pack = ((s << 10) + (exp << 6) + mant);
}

/**
 * @brief ����wi y ����yu,���ֵ������512��
 * yu[n] = (1-2^(-5)) * y[n - 1] + (2^(-5)) * W(i[n])
 * @param y:������512����
 * @param wi:������16��
 */
static void __filter_yu(int y, int wi, int * yu)
{
	int dif = 0;

	/* ��������32��,�ܹ�����Ϊԭ����512�� */
	wi = wi << 5;

	dif = (wi - y);
	dif = dif >> 5;
	*yu = y + dif;
}

/**
* @brief yl[n] = (1-(2^-6))yl[n - 1] + (2^-6)yu[n]
*/
static void __filter_yl(int ylp, int yu, int * yl)
{
	int dif = 0;
	dif = yu - (ylp >> 6);
	*yl = (ylp + dif);
}

/**
* @brief ��yu��ֵ��һ������
*/
static void __limit_yu(int yut, int * yu)
{
	/* LIMB */
	if (yut < 544)	/* 544 <= yu <= 5120,����itu�ĵ� yu ��1.06 - 10.0֮��,�˴�yu��������512���� */
		*yu = 544;
	else if (yut > 5120)
		*yu = 5120;
	else
		*yu = yut;
}

/**
 * @brief ����dms��ʱƽ�� dms = (1 - 2^(-5)) * dms +  (2^(-5)) * fi;
 */
static void __cal_dms(int dmsp, int fi, int * dms)
{
	int dif = 0;
	/* ��fi����2^9�� */
	fi = fi << 9;
	/* ��dmsp��� */
	dif = fi - dmsp;
	dif = dif >> 5;
	*dms = dmsp + dif;
}

/**
 * @brief ����dml��ʱƽ�� dml = (1 -  2^(-7)) * dml + (2^(-7)) * fi;
 */
static void __cal_dml(int dmlp, int fi, int * dml)
{
	int dif = 0;
	/* ��fi����2^11�� */
	fi = fi << 11;
	/* ��dmsp��� */
	dif = fi - dmlp;
	dif = dif >> 7;
	*dml = dmlp + dif;
}

/**
 * @brief ����ax:0,1 ���ڼ���apֵ,��Ϊmixʱal��ȡֵ����
 */
static void __cal_ax(int dms, int dml, int tdp, int y, int * ax)
{
	int dif = 0;
	int dthrd = 0;

	/* �ж��Ƿ���� */
	dif = (dms << 1) - dml;
	if(dif < 0)
		dif = -dif;

	dthrd = dml >> 3;
	*ax = ((y >= 1536) && tdp == 0 && dif < dthrd) ? 0 : 1;
}

/**
 * @brief ����p,�ڼ���a1 a2ʱ,��Ҫ�õ�p p = sgn(dq + sez)
 */
static void __cal_pk(int dq, int sez, int * p)
{
	/* ������ԭֵ,��Ӽ��� */
	*p = dq + sez;
	*p = (*p < 0) ? 1: 0;
}

/**
 * @brief ����a2 ��ʽ a2 = (1 - 2^(-7)) * a2[n-1] + (2^(-7)) * { sgn(p[k]) * sgn(p[k - 2]) - f(a1[n-1]) * sgn(p[k]) * sgn(p[k-1]) }
 * p[k] = dq[k] + sez[k]
 * pk0 pk2 pk2 ����0,��1�ķ�ʽ��¼��p[k]����ʷ����
 */
static void __cal_a2(int pk0, int pk1, int pk2, int a2, int a1, int * a2t)
{
	int s01, s02;
	int t1, t2, fa1;

	/* ������� */
	s01 = pk0 ^ pk1;
	s02 = pk0 ^ pk2;

	/* sgn(p[k]) * sgn(p[k - 2])��ֵ,��1��1,��������2^14�� */
	t1 = (s02 == 0) ? (16384) : (-16384);

	if(a1 >= 0)
	{
		fa1 = (a1 < 8192) ? (a1 * 4) : (2 << 15);
	}
	else
	{
		int a1mag = -a1;
		fa1 = (a1mag < 8192) ? (a1mag * 4) : (2 << 15);
		fa1 = -fa1;
	}

	t2 = (s01 == 0) ? fa1 : (-fa1);

	*a2t = ((t1 - t2 - a2) >> 7) + a2;
}

/**
* @brief ����a1
*/
static void __cal_a1(int a1p, int pk0, int pk1, int * a1)
{
	int temp = 0;
	int sign = pk0 ^ pk1;

	temp = a1p - (a1p >> 8);

	*a1 = (sign == 1) ? (-192 + temp) : (192 + temp);
}
/**
* @brief ����a2����a1 |a1| <= 1-2^(-4) - a2
*/
static void __limit_a1(int a1t, int a2, int * a1)
{
	int up = 0;
	int low = 0;

	/* ���ʽ�еĳ����� 1-(2^-4)����2^14����,��Ϊ0x3c00 a2����ֵС��0.75,���Բ������ұ���ʼΪ��*/
	up = 0x3c00 - a2;
	low = a2 - 0x3c00;

	if(a1t > up)
		*a1 = up;
	else if(a1t < low)
		*a1 = low;
	else
		*a1 = a1t;
}

/**
 * @brief ��a2��ֵ�и����� itu��a2�������������ֵС��0.75
 */
static void __limit_a2(int a, int * a_limit)
{
	/* 0.75����2^14����Ϊ12288 */
	if(a > 12288)
	{
		*a_limit = 12288;
	}
	else if(a < -12288)
	{
		*a_limit = -12288;
	}
	else
		*a_limit = a;
}

/**
* @brief ����trֵ
*/
static void __cal_tr(int td, int yl, int dq, int * tr)
{
	/* dq > 24 * 2^ylʱ,��Ϊ�źų��ֵ�˲��,����a2 < -0.71875 */
	int ylint = 0;
	int ylfac = 0;
	int dthr = 0;
	int thr1 = 0;
	int thr2 = 0;

	/* yl��������2^15���� */

	/* ȡ��yl���������� */
	ylint = yl >> 15;
	/* ȡ��yl��С������,ֻȡ��λ */
	ylfac = ((yl >> 10) & 0x1f);

	/* �� 2^(ylint + ylfac), 2^ylfacȡ����Ϊylfac */
	thr1 = (ylfac + 32) << ylint;/* �õ���ֵΪ��ʵ��32�� */
	thr2 = (ylint > 8) ? 0x3e00 : thr1;
	dthr = ((thr1 + (thr1 >> 1)) >> 1);/* �õ���ֵΪ��ʵֵ��24�� */
	
	/* ��dq����ֵ���бȽ� */
	if(dq < 0)
		dq = -dq;

	*tr = ((dq > dthr) && (td == 1)) ? 1 : 0;
}

/**
* @brief tone detect
*/
static void __tone_detect(int a2, int * td)
{
	/* ���a2 < -0.71875ʱ,tdֵΪ1,��Ϊ��ʱ���ֵ��ź��������ź� */
	if(a2 < -11776)
		*td = 1;
	else
		*td = 0;
}

/**
* @brief ����ap ����ax��,����ʽ�ݱ�� (1-(2^-4))ap[n-1] + ax * (2^-3),���������ֵ��ԭ����2^8��
*/
static void __cal_ap(int ax, int app, int * ap)
{
	int dif = (ax << 9) - app;
	*ap = (dif >> 4) + app;
}

/**
* @brief ����bi (i = 1..6)  bi = (1 - (2^-8))*bi[n-1] + (2^-7)*(sgn(dq[n]) * sgn(dq[n-i]))
*/
static void __cal_bi(int dq, int dqki, int bip, int * bi)
{
	/* ��������dqֵ�Ѿ����������146��ʽ�ĸ�����,ȡ������λ */
	int s;
	int ski;
	int sbi;
	int part2 = 0;

	s = dq >> 10;
	ski = dqki >> 10;

	sbi = s ^ ski;

	part2 = (sbi == 0) ? (128) : (-128);

	part2 = part2 - (bip >> 8);

	*bi = bip + part2;
}

/**
 * @brief 726����,������� 8bit ulaw / alawת����4bit��ѹ����
 * @param inp_buf:����� ulaw / alaw��
 * @param out_buf:�����4bit���ѹ����
 * @param buf_sz:��������С
 * @param is_alaw:1:������alaw 0:ulaw
 * @param reset:�Ƿ񽫼Ĵ����óɳ�ʼ״̬
 * @param hstate:����״̬��
 */
void lve_G726_encode(short * inp_buf, short *out_buf, long buf_sz,
	int is_alaw, HG726ENCODE hstate)
{
	int j = 0;
	int sr2 = 0;
	int dq6 = 0;
	int stemp = 0;
	int sez = 0;
	int se = 0; /* ��ֵ�ź� */
	int s = 0; /* ԭʼpcm�ź� */
	int sr = 0;

	int a2 = 0;
	int a1 = 0;
	int b6 = 0;
	int b5 = 0;
	int b4 = 0;
	int b3 = 0;
	int b2 = 0;
	int b1 = 0;

	int a2t = 0;

	int d = 0; /* ԭʼpcm���ֵse�Ĳ�ֵ */
	int dl = 0; /* ��ֵ�Ķ��� */
	int ds = 0; /* d�ķ��� */
	int dln = 0; /* ����ֵ/�����׵Ķ��� */

	int dq = 0; /* ������ֵ */
	int dql = 0; /* ������ֵ�Ķ��� */
	int dqs = 0; /* dq�ķ��� */
	int dqln = 0; /* ������ֵ/�����׵Ķ��� */

	int ap = 0;
	int al = 0;
	int pk2 = 0;
	int ax = 0;

	/* ����y��صı��� */
	int yu = 0;
	int yl = 0;
	int y = 0;
	int yut = 0;

	int i = 0;
	int wi = 0;
	int fi = 0;

	int wa1 = 0;
	int wa2 = 0;
	int wb1 = 0;
	int wb2 = 0;
	int wb3 = 0;
	int wb4 = 0;
	int wb5 = 0;
	int wb6 = 0;

	int td = 0;
	int tr = 0;

	if(is_alaw)
	{
		/* ������alaw,����ԭ���alaw�ı��뷽ʽ */
		for (j = 0; j < buf_sz; j++)
			inp_buf[j] ^= 85;
	}
	
	for(j = 0; j < buf_sz; j ++)
	{
		/*
		* ���ȸ�����ʷ���ؽ��ź�ֵ�뷴�������źŲ�ֵ(��Ϊ���,ֻ��Ҫ���������Ĳ�ֵ�ź�,��������ɽ���)�����ֵ�ź�
		* se = a1 * sr1 + a2 * sr2 + b1 * dq1 + ... + b6 * dq6
		* ���е���ʷֵ�Ӻ�,Ȼ��������
		*/

		/*
		 * �ؽ��ź��Ӻ�,��������Ԥ��ϵ��������
		 * ע��,Ӧ��se ��sez���������,��Ϊ֮��Ҫ�õ�sez + dq�ķ���������pk0 pk1,����������Ԥ�������ֵ
		 */

		/* ÿ��ֵ�Ӻ�һ��ʱ�䵥λ */
		sr2 = hstate->sr1;
		hstate->sr1 = hstate->sr0;

		a2 = hstate->a2;
		a1 = hstate->a1;

		__fmult(a1, hstate->sr1, &wa1);
		__fmult(a2, sr2, &wa2);

		/**
		 * ����sez
		 */

		/* ÿ��ֵ�Ӻ�һ����λ */
		b6 = hstate->b6;
		b5 = hstate->b5;
		b4 = hstate->b4;
		b3 = hstate->b3;
		b2 = hstate->b2;
		b1 = hstate->b1;

		dq6 = hstate->dq5;
		hstate->dq5 = hstate->dq4;
		hstate->dq4 = hstate->dq3;
		hstate->dq3 = hstate->dq2;
		hstate->dq2 = hstate->dq1;
		hstate->dq1 = hstate->dq0;

		__fmult(b1, hstate->dq1, &wb1);
		__fmult(b2, hstate->dq2, &wb2);
		__fmult(b3, hstate->dq3, &wb3);
		__fmult(b4, hstate->dq4, &wb4);
		__fmult(b5, hstate->dq5, &wb5);
		__fmult(b6, dq6, &wb6);

		/* ��ֵ�ź� */
		__cal_se(wa1, wa2, wb1, wb2, wb3, wb4, wb5, wb6, &se, &sez);

		/* ��ulaw/alawչ����pcm�ź�,�������ֵ,ע��˴�Ҫ��4(����Ϊû�г�4,����æ���˺ü��찡) */
		s = (lve_g711_ulaw2line(inp_buf[j])) / 4;//Ҫ��4,����711һ��,���Ե���λ,��4�������ñ��������Ż���>>2��,����

		/* ����ԭʼ�ź����ֵ�źŵĲ�ֵ */
		d = s - se;
		
		/* �ƴ���ֵ����2Ϊ�׵Ķ���ֵ dl,����ȡ����Ӧ�ķ��� */
		__log(d, &dl, &ds);

		/* ����dl,�����������y,����ת��������,�����Ӧ�ļ��� */
		ap = hstate->apr;

		/* �������׵Ķ���y */
		__limt_al(ap, &al);

		/* ��alΪ�������ӻ�Ͽ�����������Ӧ�������� */
		yu = hstate->yup;
		yl = hstate->ylp;		
		__mix_scale_factor(yl, yu, al, &y);

		/* ��dl��y�Ĳ�ֵ */
		__cal_dln(dl, y, &dln);

		/* ������� */
		__quan_dln(dln, ds, &i);

		/* ����������� */
		out_buf[j] = i;

		/* ������i,���¸����Ĵ��� */
		__anti_quan(i, &dqln, &dqs);

		/* dqln��y���,�õ�dql */
		__cal_dql(dqln, y, &dql);

		/* dql������������,����Ϸ���λ�õ�dq */
		__anti_log(dql, dqs, &dq);

		/* ��dqֵ��������dq0(1,4,6)��ʽ */
		__pack_146_float(dq, &hstate->dq0, 1);

		sr = se + dq;
		__pack_146_float(sr, &hstate->sr0, 0);

		/* ����w(i[n]),����yl yu */
		__function_w(i, &wi);
		__filter_yu(y, wi, &yut);
		/* yut�и����� */
		__limit_yu(yut, &hstate->yup);

		/* ����yl */
		__filter_yl(yl, hstate->yup, &hstate->ylp);

		/* ����i���� f(i[n]),����dml dms */
		__function_f(i, &fi);
		__cal_dml(hstate->dmlp, fi, &hstate->dmlp);
		__cal_dms(hstate->dmsp, fi, &hstate->dmsp);

		/* ����ap,���ݸ�������,���ò�ͬ�ı��ʽ����ap(����ʱƽ��֮��,�Ƿ��⵽td,�Ƿ�tr,�Լ�y[n]��ֵ) */
		/* ��ʷpkֵ�Ӻ�,������pk0 */
		pk2 = hstate->pk1;
		hstate->pk1 = hstate->pk0;
		__cal_pk(dq, sez, &hstate->pk0);
		/* �ȼ���a2,�����õ�td, */
		__cal_a2(hstate->pk0, hstate->pk1, pk2, a2, a1, &a2t);
		/* ��a2������ */
		__limit_a2(a2t, &a2);

		/* �ù�ȥ��td yl ��ǰ��dq (����itu���ĵ�Ӧ�ö����õ�ǰ�ĲŶ�)����tr��ȡֵ */
		td = hstate->tdr;
		__cal_tr(td, yl, dq, &tr);

		/* ����td��ֵ(δ��tr�޸ĵ�)dms dml y����ax,��������ap */
		__cal_ax(hstate->dmsp, hstate->dmlp, td, y, &ax);
		__cal_ap(ax, ap, &hstate->apr);

		/* ����tr��apֵ�����޸� */
		if(1 == tr)
			hstate->apr = 256;/* 1����2^8�� */

		/* ����a2ֵ(δ��tr�޸ĵ�),����td����ֵ */
		__tone_detect(a2, &td);
		/* ����trֵ,����td��ֵ */
		if(1 == tr)
			td = 0;

		/* ����trֵ,��a2��ֵ������,����itu tr=0,��Ϊ�źų���˲��,��a2 a1 b1 ... b6ȫ���ó�0 */
		if(1 == tr)
			hstate->a2 = 0;
		else
			hstate->a2 = a2;

		/* ����a1 */
		if(1 == tr)
		{
			/* ����tr�޸�a1 */
			hstate->a1 = 0;
		}
		else
		{
			__cal_a1(a1, hstate->pk0, hstate->pk1, &hstate->a1);
			/* ����a2����a1 */
			__limit_a1(a1, a2, &a1);
		}

		/* b1 ... b2(���÷����ݶȷ�����������ӦԤ��ϵ��) */
		if(1 == tr)
		{
			hstate->b1 = 0;
			hstate->b2 = 0;
			hstate->b3 = 0;
			hstate->b4 = 0;
			hstate->b5 = 0;
			hstate->b6 = 0;
		}
		else
		{
			/* ����dq[n] �� dq[n - i]���������bi */
			__cal_bi(hstate->dq0, hstate->dq1, hstate->b1, &hstate->b1);
			__cal_bi(hstate->dq0, hstate->dq2, hstate->b2, &hstate->b2);
			__cal_bi(hstate->dq0, hstate->dq3, hstate->b3, &hstate->b3);
			__cal_bi(hstate->dq0, hstate->dq4, hstate->b4, &hstate->b4);
			__cal_bi(hstate->dq0, hstate->dq5, hstate->b5, &hstate->b5);
			__cal_bi(hstate->dq0, dq6, hstate->b6, &hstate->b6);
		}
	}
}


/**
 * @brief 726����,������4bit��ѹ����ת����8bit ulaw / alaw
 * @param inp_buf:�����4bit���ѹ����
 * @param out_buf:����� ulaw / alaw��
 * @param buf_sz:��������С
 * @param is_alaw:1:�����alaw 0:ulaw
 * @param reset:�Ƿ񽫼Ĵ����óɳ�ʼ״̬
 * @param hstate:����״̬��
 */
void lve_G726_decode(short *inp_buf, short *out_buf, long buf_sz,
	int is_alaw, HG726DECODE hstate)
{
	/* �����㷨�����Ѿ������ڱ�������,ֻҪ���ع��ź���������� */
	/* ����ع��źź�,���ź�ת��alaw����ulaw,��ת����,���õ����źŵĹ�ֵ�ź����,�����¼���i,
	�����µ�i������i�Ĵ�С,�������alaw����ulaw��ֵ���� +1 -1
	*/
	int j = 0;
	int sr2 = 0;
	int dq6 = 0;
	int stemp = 0;
	int sez = 0;
	int se = 0; /* ��ֵ�ź� */
	int s = 0; /* ԭʼpcm�ź� */
	int sr = 0;

	int a2 = 0;
	int a1 = 0;
	int b6 = 0;
	int b5 = 0;
	int b4 = 0;
	int b3 = 0;
	int b2 = 0;
	int b1 = 0;

	int a2t = 0;

	int d = 0; /* ԭʼpcm���ֵse�Ĳ�ֵ */
	int dl = 0; /* ��ֵ�Ķ��� */
	int ds = 0; /* d�ķ��� */
	int dln = 0; /* ����ֵ/�����׵Ķ��� */

	int dq = 0; /* ������ֵ */
	int dql = 0; /* ������ֵ�Ķ��� */
	int dqs = 0; /* dq�ķ��� */
	int dqln = 0; /* ������ֵ/�����׵Ķ��� */

	int ap = 0;
	int al = 0;
	int pk2 = 0;
	int ax = 0;

	/* ����y��صı��� */
	int yu = 0;
	int yl = 0;
	int y = 0;
	int yut = 0;

	int i = 0;
	int wi = 0;
	int fi = 0;

	int wa1 = 0;
	int wa2 = 0;
	int wb1 = 0;
	int wb2 = 0;
	int wb3 = 0;
	int wb4 = 0;
	int wb5 = 0;
	int wb6 = 0;

	int td = 0;
	int tr = 0;

	for(j = 0; j < buf_sz; j ++)
	{
		/* �����ѹ���뷴������dq */
		/* ����dq���¼���sr */

		/* �����ֵ�ź� */
		/* ÿ��ֵ�Ӻ�һ��ʱ�䵥λ */
		sr2 = hstate->sr1;
		hstate->sr1 = hstate->sr0;

		a2 = hstate->a2;
		a1 = hstate->a1;

		__fmult(a1, hstate->sr1, &wa1);
		__fmult(a2, sr2, &wa2);

		/**
		* ����sez
		*/

		/* ÿ��ֵ�Ӻ�һ����λ */
		b6 = hstate->b6;
		b5 = hstate->b5;
		b4 = hstate->b4;
		b3 = hstate->b3;
		b2 = hstate->b2;
		b1 = hstate->b1;

		dq6 = hstate->dq5;
		hstate->dq5 = hstate->dq4;
		hstate->dq4 = hstate->dq3;
		hstate->dq3 = hstate->dq2;
		hstate->dq2 = hstate->dq1;
		hstate->dq1 = hstate->dq0;

		__fmult(b1, hstate->dq1, &wb1);
		__fmult(b2, hstate->dq2, &wb2);
		__fmult(b3, hstate->dq3, &wb3);
		__fmult(b4, hstate->dq4, &wb4);
		__fmult(b5, hstate->dq5, &wb5);
		__fmult(b6, dq6, &wb6);

		/* ��ֵ�ź� */
		__cal_se(wa1, wa2, wb1, wb2, wb3, wb4, wb5, wb6, &se, &sez);

		/* ����y */
		/* ����dl,�����������y,����ת��������,�����Ӧ�ļ��� */
		ap = hstate->apr;
		__limt_al(ap, &al);
		yu = hstate->yup;
		yl = hstate->ylp;		
		__mix_scale_factor(yl, yu, al, &y);

		/* ������i,�õ�dq */
		i = inp_buf[j];
		__anti_quan(i, &dqln, &dqs);
		__cal_dql(dqln, y, &dql);
		__anti_log(dql, dqs, &dq);

		/* ��dqֵ��������dq0(1,4,6)��ʽ */
		__pack_146_float(dq, &hstate->dq0, 1);

		/* ���ݹ�ֵ�źż���sr */
		sr = se + dq;
		__pack_146_float(sr, &hstate->sr0, 0);

		/* ���õ����ź�ѹ����ulaw(�ݲ�֧��alaw,��Ϊ���˻�ûд��alaw�ı�����㷨) */
		sr = sr * 4;/* �������Ὣ���Ż��� <<2,�Ǻ� */
		out_buf[j] = lve_g711_line2ulaw(sr);
		/* todo:�˴�����Ҫ��ͬ���������,������һЩ������726������,�е�Ҳû��...����,�п����� */

		/* ������Ӧ�ļĴ���,����뷽ͬ������ */
		/* �����������������ʱ����ͬ */

		/* ����w(i[n]),����yl yu */
		__function_w(i, &wi);
		__filter_yu(y, wi, &yut);
		/* yut�и����� */
		__limit_yu(yut, &hstate->yup);

		/* ����yl */
		__filter_yl(yl, hstate->yup, &hstate->ylp);

		/* ����i���� f(i[n]),����dml dms */
		__function_f(i, &fi);
		__cal_dml(hstate->dmlp, fi, &hstate->dmlp);
		__cal_dms(hstate->dmsp, fi, &hstate->dmsp);

		/* ����ap,���ݸ�������,���ò�ͬ�ı��ʽ����ap(����ʱƽ��֮��,�Ƿ��⵽td,�Ƿ�tr,�Լ�y[n]��ֵ) */
		/* ��ʷpkֵ�Ӻ�,������pk0 */
		pk2 = hstate->pk1;
		hstate->pk1 = hstate->pk0;
		__cal_pk(dq, sez, &hstate->pk0);
		/* �ȼ���a2,�����õ�td, */
		__cal_a2(hstate->pk0, hstate->pk1, pk2, a2, a1, &a2t);
		/* ��a2������ */
		__limit_a2(a2t, &a2);

		/* �ù�ȥ��td yl ��ǰ��dq (����itu���ĵ�Ӧ�ö����õ�ǰ�ĲŶ�)����tr��ȡֵ */
		td = hstate->tdr;
		__cal_tr(td, yl, dq, &tr);

		/* ����td��ֵ(δ��tr�޸ĵ�)dms dml y����ax,��������ap */
		__cal_ax(hstate->dmsp, hstate->dmlp, td, y, &ax);
		__cal_ap(ax, ap, &hstate->apr);

		/* ����tr��apֵ�����޸� */
		if(1 == tr)
			hstate->apr = 256;/* 1����2^8�� */

		/* ����a2ֵ(δ��tr�޸ĵ�),����td����ֵ */
		__tone_detect(a2, &td);
		/* ����trֵ,����td��ֵ */
		if(1 == tr)
			td = 0;

		/* ����trֵ,��a2��ֵ������,����itu tr=0,��Ϊ�źų���˲��,��a2 a1 b1 ... b6ȫ���ó�0 */
		if(1 == tr)
			hstate->a2 = 0;
		else
			hstate->a2 = a2;

		/* ����a1 */
		if(1 == tr)
		{
			/* ����tr�޸�a1 */
			hstate->a1 = 0;
		}
		else
		{
			__cal_a1(a1, hstate->pk0, hstate->pk1, &hstate->a1);
			/* ����a2����a1 */
			__limit_a1(a1, a2, &a1);
		}

		/* b1 ... b2(���÷����ݶȷ�����������ӦԤ��ϵ��) */
		if(1 == tr)
		{
			hstate->b1 = 0;
			hstate->b2 = 0;
			hstate->b3 = 0;
			hstate->b4 = 0;
			hstate->b5 = 0;
			hstate->b6 = 0;
		}
		else
		{
			/* ����dq[n] �� dq[n - i]���������bi */
			__cal_bi(hstate->dq0, hstate->dq1, hstate->b1, &hstate->b1);
			__cal_bi(hstate->dq0, hstate->dq2, hstate->b2, &hstate->b2);
			__cal_bi(hstate->dq0, hstate->dq3, hstate->b3, &hstate->b3);
			__cal_bi(hstate->dq0, hstate->dq4, hstate->b4, &hstate->b4);
			__cal_bi(hstate->dq0, hstate->dq5, hstate->b5, &hstate->b5);
			__cal_bi(hstate->dq0, dq6, hstate->b6, &hstate->b6);
		}
	}
}




















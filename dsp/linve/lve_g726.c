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
 * @brief 编码状态机
 */
typedef struct __g726_encode_state_t_
{
	/**
	* @brief 保留前两个重建信号与前6个反量化的信号差值
	* 存储的格式为 <1,4,6> 即1:为符号位 4:为阶 6:为尾数 [-32768] - [+32767] 之间
	*/
	int sr0, sr1;

	/**
	 * @brief 线性预测系数 理论时[-3] - [+3]之间,应将此值放大2^14位 a2被限制为绝对值小于0.75,a1的值也被限制
	 */
	int a1, a2;
	int b1, b2, b3, b4, b5, b6;

	/**
	 * @brief 保留前两个重建信号与前6个反量化的信号差值
	 * 存储的格式为 <1,4,6> 即1:为符号位 4:为阶 6:为尾数 [-32768] - [+32767] 之间
	 */
	int dq5, dq4, dq3, dq2, dq1, dq0;

	/**
	* @brief 量化差值信号短时平均,它由f[i[n]]经过一个低通滤波器得到
	*/
	int dmsp;
	/**
	* @brief 量化差值信号长时平均,它由f[i[n]]经过一个低通滤波器得到,但它衰减得比dmsp缓慢
	*/
	int dmlp;

	/**
	* @brief 量化自适应速度控制因子,这个值在计算的时候被扩大了256倍
	*/
	int apr;

	/**
	 * @brief 快速自适应量化因子,保留一个延时,在计算时,这个值被扩大了512倍,这个值被限制了,大约在5120至544之间
	 */
	int yup;

	/**
	* @brief 单频信号检测,即用于fsk检测,当a2大于某个值时,取值为1. 即 (b1* Z^-1 + ... + b6 * Z^-6)/(a2 * Z^-2 + a1 * Z^-1) 系统函数是的a2, 
	*/
	int tdr;

	/**
	* @brief 这两个为dq + sez值的符号,用于在线性预测系数估值时的计算(即符号梯度算法中运用到这两个值的延时)
	*/
	int pk0, pk1;

	/**
	 * @brief 慢速自适应量化因子,保留一个延时,在计算时,这个值被扩大了512 * 64(2^15)倍,即它是yup的64倍,这与滤波公式有关,扩64倍正好不涉及到小数点之后的运算
	 */
	int ylp;
}g726_encode_state_t;


/**
 * @brief 生成726编码状态机
 */
HG726ENCODE lve_gen_g726_encode()
{
	/**
	 * 这个函数完成所有寄存器的初值
	 */

	g726_encode_state_t * state = (g726_encode_state_t *)malloc(sizeof(*state));

	/*
	 * 线性预测系数全部置成0
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
	 * 历史重建信号值以及反量化差值,从计算过程中可以看出32其实就是0
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
	 * @brief 快/慢速自适应量化因子赋初值
	 */
	state->yup = 544;/* 从计算过程中看出此值为yup的取值下限 */
	state->ylp = 34816;/* 把 544 * 64,就可以得到这个数值,正如滤波公式所看到的扩大64倍正好能避免一个小数点之后的运算 */


	/*
	 * pk0 pk1初始化
	 */
	state->pk0 = 0;
	state->pk1 = 0;

	/*
	 * 长短时平均 自适应速度控制 单音检测触发 置0
	 */
	state->dmsp = 0;
	state->dmlp = 0;
	state->apr = 0;
	state->tdr = 0;

	return state;
}

/**
 * @brief 销毁726编码状态机
 */
void lve_destroy_g726_encode(HG726ENCODE hstate)
{
	if(hstate)
		free(hstate);
}

/**
 * @brief 量化差值至4bit(1 + 3)
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
 * @brief 根据i反量化输出
 */
static void __anti_quan(int i, int * dqln, int * dqs)
{
	/* 这里面的值,按照itu标准标放在顾128倍 */
	static int tab[16] = {-2048, 4, 135, 213, 273, 323, 373, 425,
		425, 373, 323, 273, 213, 135, 4, -2048};
	
	/* Extract sign */
	*dqs = (i >> 3);
	
	/* Table look-up */
	*dqln = tab[i];
}

/**
 * @brief 计算以2为底的对数, (4 + 7) 即对数被放大了 2^7(128)倍
 */
static void __log(int d, int * dl, int * ds)
{
	int dmag = 0;
	int exp = 0;
	int mant = 0;

	/* 取出绝对值与符号位 */
	*ds = (d > 0) ? 0 : 1;
	dmag = (d > 0) ? d : (-d);

	/* 计算log2的整数部分,即将绝对值分成 2^exp * (1 + 0.x) */
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

	/* 根据log(1 + x) 约等于x,计算小数部分 */
	mant = (((dmag << 7) >> exp) & 0x7f); /* 保留小数点后7位 */

	/* 组合成定点数 后7位为小数 */
	*dl = (exp << 7) + mant;
}

/**
 * @brief 浮点数相乘 
 * @param s:为(1,4,6) 
 * @param a:为放大的系数
 * @param ret:返回值
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

	/* a的符号位 */
	as = (a > 0) ? 0 : 1;

	/* 计算a的对数 */
	/* amag为a的绝对值 */
	amag = (a > 0) ? (a) : ((-a) & 0xffff);

	/* 求amag的对数 */
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

	/* 取出a的6位尾数 */
	amant = (amag << 6) >> aexp;

	/* 将s按(1,4,6)格式解包 */
	ss = s >> 10;
	sexp = ((s >> 6) & 0xf);
	smant = (s & (0x3f));

	/* 指数相加 */
	rexp = aexp + sexp ;
	/* 尾数相乘,6 + 6 尾数最高可达 12-4=8 位 */
	rmant = ((amant * smant) + 0xc0) >> 4;

	/* 打包结果 */
	/* 尾数与指数移动, 8(尾数) + 14(a本身扩大了2^14) = 22 即2^22倍,这里只回缩2^21倍,即最终结果值是原来的2倍 */
	*ret = (rexp > 21) ? (((rmant) << (rexp - 21)) & 0xffff) : ((rmant) >> (21 - rexp));
	/* 计算符号位 */
	rs = as ^ ss;

	*ret = (rs == 0) ? (*ret) : (-(*ret));
}

/**
 * @brief 计算估值信号
 */
static void __cal_se(int wa1, int wa2, int wb1, int wb2, int wb3, int wb4, int wb5, int wb6, int * se, int * sez)
{
	*sez = wb1 + wb2 + wb3 + wb4 + wb5 + wb5 + wb6;

	*se = *sez + wa1 + wa2;
	
	*se = *se >> 1;
	*sez = *sez >> 1;
}

/**
 * @brief 根据ap求al
 */
static void __limt_al(int ap, int * al)
{
	*al = (ap >= 256) ? 64 : (ap >> 2);
}

/**
 * @brief 以al为比例因子混合快速慢速自适应量化因子,输出混合后的自适应量化因子
 * 公式 y=al * yu[n - 1] + (1 - al)yl[n - 1]
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
 * @brief 计处 dl 与 y的差值,得到量化后的对数dln
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
 * @brief 查表计算 f(i[n])
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
 * @brief 查表计算 w(i[n])
 */
static void __function_w(int i, int * wi)
{
	/* Initialized data */
	static int tab[8] = {-12, 18, 41, 64, 112, 198, 355, 1122};
	int im, is;
	
	is = (i >> 3);
	
	im = (is == 0) ? (i & 0xf) : ((15 - i) & 0xf);
	
	/* Scale factor multiplier */
	/* 本身已经扩大了16倍 */
	*wi = tab[im];
}

/**
 * @brief 反对数运算(以2为底)
 * @param dql:dq的对数,注意这是放大了2^7=128倍的,即后7位是小数
 * @param dqs:dq的符号位
 * @param dq:输入的反量化差值,为无扩大倍数的原值
 */
static void __anti_log(int dql, int dqs, int * dq)
{
	/* 从itu的标准文档中提供的表格可以看出,dql是可能为负的(当i=0时,输出是负无穷) */
	int exp = 0;
	int mant = 0;

	if(dql > 0)
	{
		/* 取出整数部分 */
		exp = (dql >> 7);

		/* 小数部分, 当x小于1时 2^(x) 约等于 (1 + x) */
		mant = (dql & 0x7f) + 128;

		/* mant << exp >> 7,这么做可能是为了防止一些位被移没了 */
		mant = (mant << 7) >> (14 - exp);

		/* 与符号位组合打包返回 */
		*dq = (dqs == 0) ? mant : (-mant);
	}
	else
	{
		/* 2的负无穷次方,直接认为是零 */
		*dq = 0;
	}
}

/**
 * @brief 打包成(1,4,6)格式的浮点数 1:符号位 4:阶 6:尾数
 */
static void __pack_146_float(int f_org, int * f_pack, int bdq)
{
	int mant = 0;
	int s = 0;
	int exp = 0;
	int mag = 0;

	/* 取出符号位 */
	s = (f_org > 0) ? 0 : 1;
	/* 计算绝对值 */
	mag = (s == 0) ? f_org : (-f_org);
	
	/* 计算阶 */
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
	
	/* 计算尾数 */
	mant = ((mag << 6) >> exp);
	
	/* 打包 */
	*f_pack = ((s << 10) + (exp << 6) + mant);
}

/**
 * @brief 根据wi y 计算yu,输出值扩大了512倍
 * yu[n] = (1-2^(-5)) * y[n - 1] + (2^(-5)) * W(i[n])
 * @param y:扩大了512倍的
 * @param wi:扩大了16倍
 */
static void __filter_yu(int y, int wi, int * yu)
{
	int dif = 0;

	/* 继续扩大32倍,总共扩大为原来的512倍 */
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
* @brief 对yu的值做一个限制
*/
static void __limit_yu(int yut, int * yu)
{
	/* LIMB */
	if (yut < 544)	/* 544 <= yu <= 5120,按照itu文档 yu 在1.06 - 10.0之间,此处yu是扩大了512倍的 */
		*yu = 544;
	else if (yut > 5120)
		*yu = 5120;
	else
		*yu = yut;
}

/**
 * @brief 计算dms短时平均 dms = (1 - 2^(-5)) * dms +  (2^(-5)) * fi;
 */
static void __cal_dms(int dmsp, int fi, int * dms)
{
	int dif = 0;
	/* 将fi扩大2^9倍 */
	fi = fi << 9;
	/* 与dmsp相减 */
	dif = fi - dmsp;
	dif = dif >> 5;
	*dms = dmsp + dif;
}

/**
 * @brief 计算dml长时平均 dml = (1 -  2^(-7)) * dml + (2^(-7)) * fi;
 */
static void __cal_dml(int dmlp, int fi, int * dml)
{
	int dif = 0;
	/* 将fi扩大2^11倍 */
	fi = fi << 11;
	/* 与dmsp相减 */
	dif = fi - dmlp;
	dif = dif >> 7;
	*dml = dmlp + dif;
}

/**
 * @brief 计算ax:0,1 用于计算ap值,做为mix时al的取值依据
 */
static void __cal_ax(int dms, int dml, int tdp, int y, int * ax)
{
	int dif = 0;
	int dthrd = 0;

	/* 判断是否大于 */
	dif = (dms << 1) - dml;
	if(dif < 0)
		dif = -dif;

	dthrd = dml >> 3;
	*ax = ((y >= 1536) && tdp == 0 && dif < dthrd) ? 0 : 1;
}

/**
 * @brief 计算p,在计算a1 a2时,需要用到p p = sgn(dq + sez)
 */
static void __cal_pk(int dq, int sez, int * p)
{
	/* 两都是原值,相加即可 */
	*p = dq + sez;
	*p = (*p < 0) ? 1: 0;
}

/**
 * @brief 计算a2 公式 a2 = (1 - 2^(-7)) * a2[n-1] + (2^(-7)) * { sgn(p[k]) * sgn(p[k - 2]) - f(a1[n-1]) * sgn(p[k]) * sgn(p[k-1]) }
 * p[k] = dq[k] + sez[k]
 * pk0 pk2 pk2 以正0,负1的方式记录了p[k]的历史符号
 */
static void __cal_a2(int pk0, int pk1, int pk2, int a2, int a1, int * a2t)
{
	int s01, s02;
	int t1, t2, fa1;

	/* 计算符号 */
	s01 = pk0 ^ pk1;
	s02 = pk0 ^ pk2;

	/* sgn(p[k]) * sgn(p[k - 2])的值,正1或负1,这里扩大2^14倍 */
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
* @brief 计算a1
*/
static void __cal_a1(int a1p, int pk0, int pk1, int * a1)
{
	int temp = 0;
	int sign = pk0 ^ pk1;

	temp = a1p - (a1p >> 8);

	*a1 = (sign == 1) ? (-192 + temp) : (192 + temp);
}
/**
* @brief 根据a2限制a1 |a1| <= 1-2^(-4) - a2
*/
static void __limit_a1(int a1t, int a2, int * a1)
{
	int up = 0;
	int low = 0;

	/* 表达式中的常数项 1-(2^-4)扩大2^14倍后,即为0x3c00 a2绝对值小于0.75,所以不等于右边终始为正*/
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
 * @brief 对a2的值有个限制 itu对a2的限制是其绝对值小于0.75
 */
static void __limit_a2(int a, int * a_limit)
{
	/* 0.75扩大2^14倍后为12288 */
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
* @brief 计算tr值
*/
static void __cal_tr(int td, int yl, int dq, int * tr)
{
	/* dq > 24 * 2^yl时,认为信号出现的瞬变,或者a2 < -0.71875 */
	int ylint = 0;
	int ylfac = 0;
	int dthr = 0;
	int thr1 = 0;
	int thr2 = 0;

	/* yl是扩大了2^15倍的 */

	/* 取出yl的整数部分 */
	ylint = yl >> 15;
	/* 取出yl的小数部分,只取五位 */
	ylfac = ((yl >> 10) & 0x1f);

	/* 即 2^(ylint + ylfac), 2^ylfac取近似为ylfac */
	thr1 = (ylfac + 32) << ylint;/* 得到的值为真实的32倍 */
	thr2 = (ylint > 8) ? 0x3e00 : thr1;
	dthr = ((thr1 + (thr1 >> 1)) >> 1);/* 得到的值为真实值的24倍 */
	
	/* 与dq绝对值进行比较 */
	if(dq < 0)
		dq = -dq;

	*tr = ((dq > dthr) && (td == 1)) ? 1 : 0;
}

/**
* @brief tone detect
*/
static void __tone_detect(int a2, int * td)
{
	/* 如果a2 < -0.71875时,td值为1,认为此时出现的信号是数据信号 */
	if(a2 < -11776)
		*td = 1;
	else
		*td = 0;
}

/**
* @brief 计算ap 引入ax后,计算式演变成 (1-(2^-4))ap[n-1] + ax * (2^-3),计算出来的值是原来的2^8倍
*/
static void __cal_ap(int ax, int app, int * ap)
{
	int dif = (ax << 9) - app;
	*ap = (dif >> 4) + app;
}

/**
* @brief 计算bi (i = 1..6)  bi = (1 - (2^-8))*bi[n-1] + (2^-7)*(sgn(dq[n]) * sgn(dq[n-i]))
*/
static void __cal_bi(int dq, int dqki, int bip, int * bi)
{
	/* 传进来的dq值已经被打包成了146格式的浮点数,取出符号位 */
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
 * @brief 726编码,将输入的 8bit ulaw / alaw转换成4bit的压缩包
 * @param inp_buf:输入的 ulaw / alaw码
 * @param out_buf:输出的4bit差分压缩码
 * @param buf_sz:缓冲区大小
 * @param is_alaw:1:输入是alaw 0:ulaw
 * @param reset:是否将寄存器置成初始状态
 * @param hstate:编码状态机
 */
void lve_G726_encode(short * inp_buf, short *out_buf, long buf_sz,
	int is_alaw, HG726ENCODE hstate)
{
	int j = 0;
	int sr2 = 0;
	int dq6 = 0;
	int stemp = 0;
	int sez = 0;
	int se = 0; /* 估值信号 */
	int s = 0; /* 原始pcm信号 */
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

	int d = 0; /* 原始pcm与估值se的差值 */
	int dl = 0; /* 差值的对数 */
	int ds = 0; /* d的符号 */
	int dln = 0; /* 量化值/量化阶的对数 */

	int dq = 0; /* 反量化值 */
	int dql = 0; /* 反量化值的对数 */
	int dqs = 0; /* dq的符号 */
	int dqln = 0; /* 反量化值/量化阶的对数 */

	int ap = 0;
	int al = 0;
	int pk2 = 0;
	int ax = 0;

	/* 计算y相关的变量 */
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
		/* 输入是alaw,具体原因见alaw的编码方式 */
		for (j = 0; j < buf_sz; j++)
			inp_buf[j] ^= 85;
	}
	
	for(j = 0; j < buf_sz; j ++)
	{
		/*
		* 首先根据历史的重建信号值与反量化的信号差值(因为如此,只需要发送量化的差值信号,即可以完成解码)计算估值信号
		* se = a1 * sr1 + a2 * sr2 + b1 * dq1 + ... + b6 * dq6
		* 所有的历史值延后,然后相乘相加
		*/

		/*
		 * 重建信号延后,并与线性预测系数相乘相加
		 * 注意,应将se 与sez都计算出来,因为之后要用到sez + dq的符号来生成pk0 pk1,用于求线性预测参数估值
		 */

		/* 每个值延后一个时间单位 */
		sr2 = hstate->sr1;
		hstate->sr1 = hstate->sr0;

		a2 = hstate->a2;
		a1 = hstate->a1;

		__fmult(a1, hstate->sr1, &wa1);
		__fmult(a2, sr2, &wa2);

		/**
		 * 计算sez
		 */

		/* 每个值延后一个单位 */
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

		/* 估值信号 */
		__cal_se(wa1, wa2, wb1, wb2, wb3, wb4, wb5, wb6, &se, &sez);

		/* 将ulaw/alaw展开成pcm信号,并计算差值,注意此处要除4(就因为没有除4,老子忙活了好几天啊) */
		s = (lve_g711_ulaw2line(inp_buf[j])) / 4;//要除4,即跟711一样,忽略低两位,除4操作就让编译器来优化成>>2吧,嘻嘻

		/* 计算原始信号与估值信号的差值 */
		d = s - se;
		
		/* 计处差值的以2为底的对数值 dl,并提取出相应的符号 */
		__log(d, &dl, &ds);

		/* 量化dl,先求出量化阶y,除法转到对数域,变成相应的减法 */
		ap = hstate->apr;

		/* 求量化阶的对数y */
		__limt_al(ap, &al);

		/* 以al为比例因子混合快速慢速自适应量化因子 */
		yu = hstate->yup;
		yl = hstate->ylp;		
		__mix_scale_factor(yl, yu, al, &y);

		/* 求dl与y的差值 */
		__cal_dln(dl, y, &dln);

		/* 查表量化 */
		__quan_dln(dln, ds, &i);

		/* 将编码结果输出 */
		out_buf[j] = i;

		/* 反量化i,更新各个寄存器 */
		__anti_quan(i, &dqln, &dqs);

		/* dqln与y相加,得到dql */
		__cal_dql(dqln, y, &dql);

		/* dql做反对数运算,并结合符号位得到dq */
		__anti_log(dql, dqs, &dq);

		/* 将dq值存下来至dq0(1,4,6)格式 */
		__pack_146_float(dq, &hstate->dq0, 1);

		sr = se + dq;
		__pack_146_float(sr, &hstate->sr0, 0);

		/* 计算w(i[n]),更新yl yu */
		__function_w(i, &wi);
		__filter_yu(y, wi, &yut);
		/* yut有个限制 */
		__limit_yu(yut, &hstate->yup);

		/* 计算yl */
		__filter_yl(yl, hstate->yup, &hstate->ylp);

		/* 根据i计算 f(i[n]),更新dml dms */
		__function_f(i, &fi);
		__cal_dml(hstate->dmlp, fi, &hstate->dmlp);
		__cal_dms(hstate->dmsp, fi, &hstate->dmsp);

		/* 更新ap,根据各种条件,采用不同的表达式计算ap(长短时平均之差,是否检测到td,是否tr,以及y[n]的值) */
		/* 历史pk值延后,并计算pk0 */
		pk2 = hstate->pk1;
		hstate->pk1 = hstate->pk0;
		__cal_pk(dq, sez, &hstate->pk0);
		/* 先计算a2,进而得到td, */
		__cal_a2(hstate->pk0, hstate->pk1, pk2, a2, a1, &a2t);
		/* 对a2的限制 */
		__limit_a2(a2t, &a2);

		/* 用过去的td yl 当前的dq (看了itu的文档应该都是用当前的才对)来对tr的取值 */
		td = hstate->tdr;
		__cal_tr(td, yl, dq, &tr);

		/* 根据td新值(未被tr修改的)dms dml y计算ax,进而计算ap */
		__cal_ax(hstate->dmsp, hstate->dmlp, td, y, &ax);
		__cal_ap(ax, ap, &hstate->apr);

		/* 利用tr对ap值进行修改 */
		if(1 == tr)
			hstate->apr = 256;/* 1扩大2^8倍 */

		/* 根据a2值(未被tr修改的),计算td的新值 */
		__tone_detect(a2, &td);
		/* 根据tr值,修正td新值 */
		if(1 == tr)
			td = 0;

		/* 根据tr值,对a2的值做修正,根据itu tr=0,认为信号出现瞬变,将a2 a1 b1 ... b6全部置成0 */
		if(1 == tr)
			hstate->a2 = 0;
		else
			hstate->a2 = a2;

		/* 计算a1 */
		if(1 == tr)
		{
			/* 利用tr修改a1 */
			hstate->a1 = 0;
		}
		else
		{
			__cal_a1(a1, hstate->pk0, hstate->pk1, &hstate->a1);
			/* 根据a2限制a1 */
			__limit_a1(a1, a2, &a1);
		}

		/* b1 ... b2(利用符号梯度法来更新自适应预测系数) */
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
			/* 根据dq[n] 与 dq[n - i]来计算各个bi */
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
 * @brief 726解码,将输入4bit的压缩包转换成8bit ulaw / alaw
 * @param inp_buf:输入的4bit差分压缩码
 * @param out_buf:输出的 ulaw / alaw码
 * @param buf_sz:缓冲区大小
 * @param is_alaw:1:输出是alaw 0:ulaw
 * @param reset:是否将寄存器置成初始状态
 * @param hstate:解码状态机
 */
void lve_G726_decode(short *inp_buf, short *out_buf, long buf_sz,
	int is_alaw, HG726DECODE hstate)
{
	/* 解码算法本身已经包含在编码中了,只要把重构信号算出来即可 */
	/* 算出重构信号后,将信号转成alaw或者ulaw,再转回来,将得到的信号的估值信号相减,并重新计算i,
	根据新的i与编码的i的大小,对输出的alaw或者ulaw的值进行 +1 -1
	*/
	int j = 0;
	int sr2 = 0;
	int dq6 = 0;
	int stemp = 0;
	int sez = 0;
	int se = 0; /* 估值信号 */
	int s = 0; /* 原始pcm信号 */
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

	int d = 0; /* 原始pcm与估值se的差值 */
	int dl = 0; /* 差值的对数 */
	int ds = 0; /* d的符号 */
	int dln = 0; /* 量化值/量化阶的对数 */

	int dq = 0; /* 反量化值 */
	int dql = 0; /* 反量化值的对数 */
	int dqs = 0; /* dq的符号 */
	int dqln = 0; /* 反量化值/量化阶的对数 */

	int ap = 0;
	int al = 0;
	int pk2 = 0;
	int ax = 0;

	/* 计算y相关的变量 */
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
		/* 将差分压缩码反量化成dq */
		/* 根据dq重新计算sr */

		/* 计算估值信号 */
		/* 每个值延后一个时间单位 */
		sr2 = hstate->sr1;
		hstate->sr1 = hstate->sr0;

		a2 = hstate->a2;
		a1 = hstate->a1;

		__fmult(a1, hstate->sr1, &wa1);
		__fmult(a2, sr2, &wa2);

		/**
		* 计算sez
		*/

		/* 每个值延后一个单位 */
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

		/* 估值信号 */
		__cal_se(wa1, wa2, wb1, wb2, wb3, wb4, wb5, wb6, &se, &sez);

		/* 计算y */
		/* 量化dl,先求出量化阶y,除法转到对数域,变成相应的减法 */
		ap = hstate->apr;
		__limt_al(ap, &al);
		yu = hstate->yup;
		yl = hstate->ylp;		
		__mix_scale_factor(yl, yu, al, &y);

		/* 反量化i,得到dq */
		i = inp_buf[j];
		__anti_quan(i, &dqln, &dqs);
		__cal_dql(dqln, y, &dql);
		__anti_log(dql, dqs, &dq);

		/* 将dq值存下来至dq0(1,4,6)格式 */
		__pack_146_float(dq, &hstate->dq0, 1);

		/* 根据估值信号计算sr */
		sr = se + dq;
		__pack_146_float(sr, &hstate->sr0, 0);

		/* 将得到的信号压缩成ulaw(暂不支持alaw,因为本人还没写出alaw的编解码算法) */
		sr = sr * 4;/* 编译器会将它优化成 <<2,呵呵 */
		out_buf[j] = lve_g711_line2ulaw(sr);
		/* todo:此处还需要做同步编码调整,但看了一些其它的726编码器,有的也没做...算了,有空再做 */

		/* 更新相应的寄存器,与编码方同步更新 */
		/* 以下做的事情跟编码时的相同 */

		/* 计算w(i[n]),更新yl yu */
		__function_w(i, &wi);
		__filter_yu(y, wi, &yut);
		/* yut有个限制 */
		__limit_yu(yut, &hstate->yup);

		/* 计算yl */
		__filter_yl(yl, hstate->yup, &hstate->ylp);

		/* 根据i计算 f(i[n]),更新dml dms */
		__function_f(i, &fi);
		__cal_dml(hstate->dmlp, fi, &hstate->dmlp);
		__cal_dms(hstate->dmsp, fi, &hstate->dmsp);

		/* 更新ap,根据各种条件,采用不同的表达式计算ap(长短时平均之差,是否检测到td,是否tr,以及y[n]的值) */
		/* 历史pk值延后,并计算pk0 */
		pk2 = hstate->pk1;
		hstate->pk1 = hstate->pk0;
		__cal_pk(dq, sez, &hstate->pk0);
		/* 先计算a2,进而得到td, */
		__cal_a2(hstate->pk0, hstate->pk1, pk2, a2, a1, &a2t);
		/* 对a2的限制 */
		__limit_a2(a2t, &a2);

		/* 用过去的td yl 当前的dq (看了itu的文档应该都是用当前的才对)来对tr的取值 */
		td = hstate->tdr;
		__cal_tr(td, yl, dq, &tr);

		/* 根据td新值(未被tr修改的)dms dml y计算ax,进而计算ap */
		__cal_ax(hstate->dmsp, hstate->dmlp, td, y, &ax);
		__cal_ap(ax, ap, &hstate->apr);

		/* 利用tr对ap值进行修改 */
		if(1 == tr)
			hstate->apr = 256;/* 1扩大2^8倍 */

		/* 根据a2值(未被tr修改的),计算td的新值 */
		__tone_detect(a2, &td);
		/* 根据tr值,修正td新值 */
		if(1 == tr)
			td = 0;

		/* 根据tr值,对a2的值做修正,根据itu tr=0,认为信号出现瞬变,将a2 a1 b1 ... b6全部置成0 */
		if(1 == tr)
			hstate->a2 = 0;
		else
			hstate->a2 = a2;

		/* 计算a1 */
		if(1 == tr)
		{
			/* 利用tr修改a1 */
			hstate->a1 = 0;
		}
		else
		{
			__cal_a1(a1, hstate->pk0, hstate->pk1, &hstate->a1);
			/* 根据a2限制a1 */
			__limit_a1(a1, a2, &a1);
		}

		/* b1 ... b2(利用符号梯度法来更新自适应预测系数) */
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
			/* 根据dq[n] 与 dq[n - i]来计算各个bi */
			__cal_bi(hstate->dq0, hstate->dq1, hstate->b1, &hstate->b1);
			__cal_bi(hstate->dq0, hstate->dq2, hstate->b2, &hstate->b2);
			__cal_bi(hstate->dq0, hstate->dq3, hstate->b3, &hstate->b3);
			__cal_bi(hstate->dq0, hstate->dq4, hstate->b4, &hstate->b4);
			__cal_bi(hstate->dq0, hstate->dq5, hstate->b5, &hstate->b5);
			__cal_bi(hstate->dq0, dq6, hstate->b6, &hstate->b6);
		}
	}
}




















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
#include "mybitvector.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "myconfig.h"


typedef struct __mybitvector_t_
{
	/* 内存池句柄 */
	HMYMEMPOOL hm;

	/* 位数组 */
	unsigned char * v;

	/* 位数组v的大小(byte) */
	size_t v_byte_size;

	/* 
	* 位数组含有多少bit,
	* 初始为用户的传入值
	* 之后为mybitVectorSetbit的pos mybitVectorSetSize的vector_size的最大值
	*/
	size_t vector_bit_size;

	/* 用户指定的初始值 */
	unsigned int ini_val;
}mybitvector_t;


#define BIT_PER_BYTE 8
#define bitsize_to_bytesize(bz) ((bz + CAL_ALIGMENT(bz, BIT_PER_BYTE)) / BIT_PER_BYTE)

/**
 * @brief 重设数组大小,根据重设之后的大小,清相应的位
 */
static __INLINE__ int bitvector_resize(mybitvector_t * bv, const size_t byte_size, const size_t bit_size)
{
	assert(bv && bv->v && byte_size && bit_size && byte_size == bitsize_to_bytesize(bit_size));

	if(bit_size > bv->vector_bit_size)
		bv->vector_bit_size = bit_size;

	if(byte_size == bv->v_byte_size)
		return 0;

	if(byte_size > bv->v_byte_size)
	{
		unsigned char * v_new = MyMemPoolMalloc(bv->hm, byte_size * 2);

		if(NULL == v_new)
			return -1;

		memcpy(v_new, bv->v, bv->v_byte_size);
		memset(&v_new[bv->v_byte_size], (bv->ini_val ? 0xff : 0), byte_size * 2 - bv->v_byte_size);
		MyMemPoolFree(bv->hm, bv->v);

		bv->v_byte_size = byte_size * 2;
		bv->v = v_new;
		bv->vector_bit_size = bit_size;

		return 0;
	}

	memset(&(bv->v[byte_size]), (bv->ini_val ? 0xff : 0), bv->v_byte_size - byte_size);

	if(bv->ini_val)
		bv->v[byte_size] |= 0xff >> bit_size % BIT_PER_BYTE;
	else
		bv->v[byte_size] &= 0xff << CAL_ALIGMENT(bit_size, BIT_PER_BYTE);

	return 0;
}

/**
 * @brief 销毁位数组
 */
static void bitvector_destroy(mybitvector_t * bv)
{
	assert(bv);

	if(bv->v)
		MyMemPoolFree(bv->hm, bv->v);

	MyMemPoolFree(bv->hm, bv);
}

/**
 * @brief 创建一个位数组
 * @param hm:内存池句柄
 * @param vector_size:数组的初始大小,表示有多少bit
 * @param ini_val:位的初始值定义(0:0, 非零:1)
 */
HMYBITVECTOR mybitVectorConstruct(HMYMEMPOOL hm, const size_t vector_size, const unsigned int ini_val)
{
	mybitvector_t * bv = MyMemPoolMalloc(hm, sizeof(*bv));

	if(NULL == bv)
		return NULL;

	bv->hm = hm;

	bv->vector_bit_size = vector_size;
	bv->v_byte_size = bitsize_to_bytesize(vector_size);
	bv->ini_val = ini_val;

	bv->v = MyMemPoolMalloc(hm, bv->v_byte_size);
	if(NULL == bv->v)
		goto mybitVectorConstruct_err_;

	memset(bv->v, (ini_val ? 0xff : 0), bv->v_byte_size);

	return bv;

mybitVectorConstruct_err_:

	if(bv)
		bitvector_destroy(bv);

	return NULL;
}

/**
 * @brief 销毁位数组
 */
void mybitVectorDestruct(HMYBITVECTOR hbv)
{
	if(hbv)
		bitvector_destroy(hbv);
}

/**
 * @brief 设置某一个pos的值
 * @param val:位的值 0:0 非零:1
 * @param pos:位置,表示位偏移
 */
int mybitVectorSetbit(HMYBITVECTOR hbv, const unsigned int val, const size_t pos)
{
	size_t byte_offset = bitsize_to_bytesize(pos) - 1;

	if(NULL == hbv)
		return -1;

	if(byte_offset >= hbv->v_byte_size || pos >= hbv->vector_bit_size)
	{
		if(0 != bitvector_resize(hbv, byte_offset + 1, pos + 1))
			return -1;
	}

	if(val)
		hbv->v[byte_offset] |= 1 << CAL_ALIGMENT(pos + 1, BIT_PER_BYTE);
	else
		hbv->v[byte_offset] &= 0xff & ~(1 << CAL_ALIGMENT(pos + 1, BIT_PER_BYTE));

	return 0;
}

/**
 * @brief 获取某一个pos的值
 * @param pos:位置,表示位偏移
 * @return 返指定位的值,如果指定位不存在,则返回初始值定义,即用户在创建时传入的ini_val的值(0:0, 非零:1)
 */
int mybitVectorGetbit(HMYBITVECTOR hbv, const size_t pos)
{
	size_t byte_offset = bitsize_to_bytesize(pos) - 1;

	if(NULL == hbv)
		return -1;

	if(pos >= hbv->vector_bit_size)
		return hbv->ini_val ? 1 : 0;

	return (hbv->v[byte_offset] >> (CAL_ALIGMENT(pos, BIT_PER_BYTE) - 1)) & 0x01;
}

/**
 * @brief 获取某一个pos的值,并判断是否越界访问(并不一定超过初始位数组大小就越界,mybitVectorSetbit的最大pos为标准)
 * @param pos:位置,表示位偏移
 * @return 返指定位的值(0:0, 非零:1, 越界:-1)
 */
int mybitVectorGetbitAndJudgeOver(HMYBITVECTOR hbv, const size_t pos)
{
	size_t byte_offset = bitsize_to_bytesize(pos) - 1;

	if(NULL == hbv)
		return -1;

	if(pos >= hbv->vector_bit_size)
		return -1;

	return hbv->v[byte_offset] | 1 << CAL_ALIGMENT(pos + 1, BIT_PER_BYTE);
}

/**
 * @brief 重设位数组的大小,
 * @param vector_size:数组,表示有多少bit,vector_size比缓冲区小,多余的bit位信息丢失(变为ini_val)
 */
int mybitVectorSetSize(HMYBITVECTOR hbv, const size_t vector_size)
{
	if(NULL ==hbv)
		return -1;

	return bitvector_resize(hbv, bitsize_to_bytesize(vector_size), vector_size);
}

/**
 * @brief 打印出数组
 */
int mybitVectorPrint(HMYBITVECTOR hbv)
{
	size_t i = 0;

	if(NULL == hbv)
		return -1;

	for(i = 0; i < hbv->v_byte_size; i ++)
	{
		if(i != 0 && (i % 8) == 0)
			printf("\r\n");
		printf("%x ", hbv->v[i]);
	}

	return 0;
}




















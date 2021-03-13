/**
 * @file mybitvector.h λ���� 2008-2-11 21:34
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
	/* �ڴ�ؾ�� */
	HMYMEMPOOL hm;

	/* λ���� */
	unsigned char * v;

	/* λ����v�Ĵ�С(byte) */
	size_t v_byte_size;

	/* 
	* λ���麬�ж���bit,
	* ��ʼΪ�û��Ĵ���ֵ
	* ֮��ΪmybitVectorSetbit��pos mybitVectorSetSize��vector_size�����ֵ
	*/
	size_t vector_bit_size;

	/* �û�ָ���ĳ�ʼֵ */
	unsigned int ini_val;
}mybitvector_t;


#define BIT_PER_BYTE 8
#define bitsize_to_bytesize(bz) ((bz + CAL_ALIGMENT(bz, BIT_PER_BYTE)) / BIT_PER_BYTE)

/**
 * @brief ���������С,��������֮��Ĵ�С,����Ӧ��λ
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
 * @brief ����λ����
 */
static void bitvector_destroy(mybitvector_t * bv)
{
	assert(bv);

	if(bv->v)
		MyMemPoolFree(bv->hm, bv->v);

	MyMemPoolFree(bv->hm, bv);
}

/**
 * @brief ����һ��λ����
 * @param hm:�ڴ�ؾ��
 * @param vector_size:����ĳ�ʼ��С,��ʾ�ж���bit
 * @param ini_val:λ�ĳ�ʼֵ����(0:0, ����:1)
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
 * @brief ����λ����
 */
void mybitVectorDestruct(HMYBITVECTOR hbv)
{
	if(hbv)
		bitvector_destroy(hbv);
}

/**
 * @brief ����ĳһ��pos��ֵ
 * @param val:λ��ֵ 0:0 ����:1
 * @param pos:λ��,��ʾλƫ��
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
 * @brief ��ȡĳһ��pos��ֵ
 * @param pos:λ��,��ʾλƫ��
 * @return ��ָ��λ��ֵ,���ָ��λ������,�򷵻س�ʼֵ����,���û��ڴ���ʱ�����ini_val��ֵ(0:0, ����:1)
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
 * @brief ��ȡĳһ��pos��ֵ,���ж��Ƿ�Խ�����(����һ��������ʼλ�����С��Խ��,mybitVectorSetbit�����posΪ��׼)
 * @param pos:λ��,��ʾλƫ��
 * @return ��ָ��λ��ֵ(0:0, ����:1, Խ��:-1)
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
 * @brief ����λ����Ĵ�С,
 * @param vector_size:����,��ʾ�ж���bit,vector_size�Ȼ�����С,�����bitλ��Ϣ��ʧ(��Ϊini_val)
 */
int mybitVectorSetSize(HMYBITVECTOR hbv, const size_t vector_size)
{
	if(NULL ==hbv)
		return -1;

	return bitvector_resize(hbv, bitsize_to_bytesize(vector_size), vector_size);
}

/**
 * @brief ��ӡ������
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




















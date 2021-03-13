#ifndef __MYUTILITY_H__
#define __MYUTILITY_H__


#include <assert.h>

#include "myerr_def.h"


/**
 * ��ʹ�õĲ���
 */
#define MY_UNUSED_ARG(x) do{}while(&x == 0)

/**
 * @brief ��������i��ָ��ֵ,xΪ��ʼ,sΪָ�����ߵĿ��
 */
#define GET_INDEX_PTR(x, i, s) (/*assert(0 == (s % 4)),*/ ((unsigned char *)(x) + (i) * (s)))

/**
 * @brief �������
 */
#define CAL_ALIGMENT(x, ali) ((ali) - (x) % (ali))

/**
 * @brief unsgined intת�ɴ���,����һ������
 */
#define uint_to_big_endian(_uint_val, _dst_array, _dst_array_size) \
	do{\
		assert((_dst_array_size) >= 4);\
		(_dst_array)[0] = ((_uint_val)>>24) & 0xff;\
		(_dst_array)[1] = ((_uint_val)>>16) & 0xff;\
		(_dst_array)[2] = ((_uint_val)>>8) & 0xff;\
		(_dst_array)[3] = (_uint_val) & 0xff;\
	}while(0)

/**
 * @brief ��һ��������ʽ������޷���������ת�浽һ��unsigned int ������
 */
#define array_to_uint_as_big_endian(_src_array, _src_array_size, _uint_val) \
	do{\
		assert((_src_array_size) >= 4);\
		_uint_val = ((_src_array)[0]<<24) | ((_src_array)[1]<<16) | ((_src_array)[2]<<8) | (_src_array)[3];\
	}while(0)

/**
 * @brief ��һ��2�ֽڵ�ֵ��������
 */
#define put_2byte(_val, _dst_array, _dst_array_sz) \
	do{\
		assert((_dst_array_sz) >= 2);\
		(_dst_array)[0] = (_val)>>8; \
		(_dst_array)[1] = (_val); \
	}while(0)

/**
 * @brief ��һ��������ȡ��һ��2�ֽڵ�ֵ
 */
#define get_2byte(_src_array, _src_array_sz, _val) \
	do{\
		(_val) = ((_src_array)[0]<<8) | (_src_array)[1];\
	}while(0)


#endif












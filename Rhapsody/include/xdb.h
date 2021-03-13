/**
 * @file xdb.h ��btree�Ļ����Ϲ���xdb���ݿ�ӿں��� 2008-03-03 23:26
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
#ifndef __XDB_H__
#define __XDB_H__


#include "mymempool.h"


/* ���ݿ������� */
struct __xdb_t_;
typedef struct __xdb_t_ * HXDB;

struct __xdb_result_t_;
typedef struct __xdb_result_t_ * HXDBRESULT;


typedef enum __E_XDB_TYPE
{
	E_XDB_BYTE1,
	E_XDB_BYTE2,
	E_XDB_BYTE4,
	E_XDB_TEXT,
	E_XDB_BLOB,
}E_XDB_TYPE;

typedef struct __xdb_field_info_t_
{
	E_XDB_TYPE type;
	const char * field_name;
}xdb_field_info;

typedef struct __xdb_record_t_
{
	const char * field_name;
	union _r
	{
		unsigned char byte1;
		unsigned short byte2;
		unsigned int byte2;

		char * text;
		
		struct _blob
		{
			void * buf;
			unsigned int buf_sz;
		}blob;
	}r;
}xdb_record;

typedef enum __E_XDB_KEY_TYPE
{
	E_DEL_EXACTLY,
	E_DEL_GREATER,
	E_DEL_LESS,
}E_XDB_KEY_TYPE;

typedef struct __xdb_key_info_t_
{
	E_XDB_KEY_TYPE type;
	xdb_record value;
}xdb_key_info;


/**
 * @brief �����ݿ��ļ�
 * @param file_name:���ݿ��ļ���
 * @param max_cache_pages:���ݿ�ҳ�������
 */
extern HXDB xdb_open(HMYMEMPOOL hm, const char * file_name, const unsigned int max_cache_pages);

/**
 * @brief �ر����ݿ�
 */
extern int xdb_close(HXDB hdb);

/**
 * @brief ��������
 */
extern int xdb_create_table(HXDB hdb,
							const char * tlb_name,
							xdb_field_info * fld_info,
							unsigned int fld_info_count);

/**
 * @brief ɾ����
 */
extern int xdb_drop_table(HXDB hdb, const char * tlb_name);

/**
 * @brief ��Ӽ�¼
 */
extern int xdb_insert(HXDB hdb, const char * tlb_name, xdb_record * r, unsigned int r_count);

/**
 * @brief ɾ����¼
 */
extern int xdb_del(HXDB hdb, xdb_key_info * key, unsigned int key_count);

/**
 * @brief ���¼�¼
 */
extern int xdb_update(HXDB hdb, xdb_key_info * key, unsigned int key_count);

/**
 * @brief ��ѯ��¼,���ؽ����
 */
extern int xdb_query(HXDB hdb, xdb_key_info * key, unsigned int key_count, HXDBRESULT * result);


#endif



















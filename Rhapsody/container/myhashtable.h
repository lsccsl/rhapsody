/*
*
* myhashtable.h ��ϣ�� 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/


#ifndef __MYHASHTABLE_H__
#define __MYHASHTABLE_H__


#include "mymempool.h"


struct __myhashtable_t_;
typedef struct __myhashtable_t_ * HMYHASHTABLE;

struct __myhash_node_t_;
typedef struct __myhash_node_t_ * HMYHASHTABLE_ITER;


/*
*
*��ϣ����
*
*/
typedef size_t (*MYHASH_FUN)(const void * key);

/*
*
*�Ƚϼ�ֵ�Ƿ����
*1:��� 0:�����
*
*/
typedef int(*MYHASH_KEYEQUAL_FUN)(const void * key1, const void * key2);


/*
*
*��ϣ����
*
*/
extern HMYHASHTABLE MyHashTableConstruct(HMYMEMPOOL hm, MYHASH_FUN hash_fun, MYHASH_KEYEQUAL_FUN keyequal_fun, size_t hash_size);

/*
*
*��ϣ������
*
*/
extern void MyHashTableDestruct(HMYHASHTABLE hht);

/*
*
*��ϣ�����һ����¼,�����ظ�
*
*/
extern HMYHASHTABLE_ITER MyHashTableInsertEqual(HMYHASHTABLE hht, const void * key, const void * data);

/*
*
*����һ����¼,�������ظ�
*
*/
extern HMYHASHTABLE_ITER MyHashTableInsertUnique(HMYHASHTABLE hht, const void * key, const void * data);

/*
*
*ɾ��һ����¼,����key,
*�ɹ�ɾ������0, ���򷵻�-1
*
*/
extern int MyHashTableDelKey(HMYHASHTABLE hht, const void * key, void ** key_ret, void ** data);

/*
*
*ɾ��һ����¼,���ݵ�����
*�ɹ�ɾ������0, ���򷵻�-1
*
*/
extern int MyHashTableDelIter(HMYHASHTABLE hht, HMYHASHTABLE_ITER iter, void ** key, void ** data);

/*
*
*����һ����¼
*
*/
extern HMYHASHTABLE_ITER MyHashTableSearch(const HMYHASHTABLE hht, const void * key);

/*
*
*��ȡ��������ֵ��
*
*/
extern void * MyHashTableGetIterData(const HMYHASHTABLE_ITER it);

/*
*
*���õ�������ֵ��
*
*/
extern void MyHashTableSetIterData(const HMYHASHTABLE_ITER it, const void * data);

/*
*
*��ȡ�������ļ�ֵ
*
*/
extern const void * MyHashTableGetIterKey(HMYHASHTABLE_ITER it);

/*
*
*��ȡԪ�ظ���
*
*/
extern size_t MyHashTableGetElementCount(const HMYHASHTABLE hht);

/*
*
*��ȡ�������ļ�ֵ
*
*/
extern HMYHASHTABLE_ITER MyHashTableBegin(HMYHASHTABLE hht);

/*
*
*��ȡһ�µ�����
*
*/
extern HMYHASHTABLE_ITER MyHashTableGetNext(HMYHASHTABLE hht, HMYHASHTABLE_ITER it);

/*
*
*��ȡһ�µ�����
*
*/
extern void MyHashTableClear(HMYHASHTABLE hht);

/*
*
*��ӡ��ϣ��
*
*/
extern void MyHashTablePrint(const HMYHASHTABLE hht);


#endif





























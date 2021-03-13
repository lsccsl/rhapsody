/*
*
* myhashmap.h hashӳ�� 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/

#ifndef __MYHASHMAP_H__
#define __MYHASHMAP_H__


#include "myhashtable.h"
#include "myobj.h"


/*
*��ϣӳ����
*/
typedef struct __myhashmap_handle_
{int unused;}*HMYHASHMAP;

/*
*���������
*/
typedef struct __myhashamp_iter__
{int unused;}*HMYHASHMAP_ITER;


/*����һ����¼*/
#define MyHashMapSearch(hhm, key) (HMYHASHMAP_ITER)MyHashTableSearch((HMYHASHTABLE)hhm, key)

/*��ȡ��*/
#define MyHashMapGetIterKey(it) MyHashTableGetIterKey((HMYHASHTABLE_ITER)it)

/*��ȡ��������ֵ��*/
#define MyHashMapGetIterData(it) MyHashTableGetIterData((HMYHASHTABLE_ITER)it)

/*��ȡԪ�ظ���*/
#define MyHashMapGetElementCount(hhm) MyHashTableGetElementCount((HMYHASHTABLE)hhm)

/*��ӡ��ϣ��*/
#define MyHashMapPrint(hhm) MyHashTablePrint((HMYHASHTABLE)hhm)

/*��ȡ��һ���ڵ�*/
#define MyHashMapBegin(hhm) (HMYHASHMAP_ITER)MyHashTableBegin((HMYHASHTABLE)hhm)

/*ȡ��һ���ڵ�*/
#define MyHashMapGetNext(hhm, it) (HMYHASHMAP_ITER)MyHashTableGetNext((HMYHASHTABLE)hhm, (HMYHASHTABLE_ITER)it)


/*
*
*����ӳ��
*
*/
extern HMYHASHMAP MyHashMapConstruct(HMYMEMPOOL hm, MYHASH_FUN hash_fun, MYHASH_KEYEQUAL_FUN keyequal_fun, size_t hash_size,
								myobj_ops * key_ops,
								myobj_ops * data_ops);

/*
*
*����ӳ��
*
*/
extern void MyHashMapDestruct(HMYHASHMAP hhashmap);

/*
*
*����Ԫ��
*
*/
extern HMYHASHMAP_ITER MyHashMapInsertUnique(HMYHASHMAP hhm, const void * key, size_t key_size, const void * data, size_t data_size);

/*
*
*ɾ��һ����¼,����key,
*�ɹ�ɾ������0, ���򷵻�-1
*
*/
extern int MyHashMapDelKey(HMYHASHMAP hhm, const void * key);

/*
*
*ɾ��һ����¼,���ݵ�����
*�ɹ�ɾ������0, ���򷵻�-1
*
*/
extern int MyHashMapDelIter(HMYHASHMAP hhm, HMYHASHMAP_ITER iter);


#endif



















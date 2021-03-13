/*
*
* myhashmap.h hash映射 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/

#ifndef __MYHASHMAP_H__
#define __MYHASHMAP_H__


#include "myhashtable.h"
#include "myobj.h"


/*
*哈希映射句柄
*/
typedef struct __myhashmap_handle_
{int unused;}*HMYHASHMAP;

/*
*迭代器句柄
*/
typedef struct __myhashamp_iter__
{int unused;}*HMYHASHMAP_ITER;


/*查找一条记录*/
#define MyHashMapSearch(hhm, key) (HMYHASHMAP_ITER)MyHashTableSearch((HMYHASHTABLE)hhm, key)

/*获取键*/
#define MyHashMapGetIterKey(it) MyHashTableGetIterKey((HMYHASHTABLE_ITER)it)

/*获取迭代器的值域*/
#define MyHashMapGetIterData(it) MyHashTableGetIterData((HMYHASHTABLE_ITER)it)

/*获取元素个数*/
#define MyHashMapGetElementCount(hhm) MyHashTableGetElementCount((HMYHASHTABLE)hhm)

/*打印哈希表*/
#define MyHashMapPrint(hhm) MyHashTablePrint((HMYHASHTABLE)hhm)

/*获取第一个节点*/
#define MyHashMapBegin(hhm) (HMYHASHMAP_ITER)MyHashTableBegin((HMYHASHTABLE)hhm)

/*取下一个节点*/
#define MyHashMapGetNext(hhm, it) (HMYHASHMAP_ITER)MyHashTableGetNext((HMYHASHTABLE)hhm, (HMYHASHTABLE_ITER)it)


/*
*
*构造映射
*
*/
extern HMYHASHMAP MyHashMapConstruct(HMYMEMPOOL hm, MYHASH_FUN hash_fun, MYHASH_KEYEQUAL_FUN keyequal_fun, size_t hash_size,
								myobj_ops * key_ops,
								myobj_ops * data_ops);

/*
*
*析构映射
*
*/
extern void MyHashMapDestruct(HMYHASHMAP hhashmap);

/*
*
*插入元素
*
*/
extern HMYHASHMAP_ITER MyHashMapInsertUnique(HMYHASHMAP hhm, const void * key, size_t key_size, const void * data, size_t data_size);

/*
*
*删除一条记录,根据key,
*成功删除返回0, 否则返回-1
*
*/
extern int MyHashMapDelKey(HMYHASHMAP hhm, const void * key);

/*
*
*删除一条记录,根据迭代器
*成功删除返回0, 否则返回-1
*
*/
extern int MyHashMapDelIter(HMYHASHMAP hhm, HMYHASHMAP_ITER iter);


#endif



















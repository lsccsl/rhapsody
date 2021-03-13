/*
*
* myhashtable.h 哈希表 
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
*哈希函数
*
*/
typedef size_t (*MYHASH_FUN)(const void * key);

/*
*
*比较键值是否相等
*1:相等 0:不相等
*
*/
typedef int(*MYHASH_KEYEQUAL_FUN)(const void * key1, const void * key2);


/*
*
*哈希表构造
*
*/
extern HMYHASHTABLE MyHashTableConstruct(HMYMEMPOOL hm, MYHASH_FUN hash_fun, MYHASH_KEYEQUAL_FUN keyequal_fun, size_t hash_size);

/*
*
*哈希表析构
*
*/
extern void MyHashTableDestruct(HMYHASHTABLE hht);

/*
*
*哈希表插入一条记录,允许重复
*
*/
extern HMYHASHTABLE_ITER MyHashTableInsertEqual(HMYHASHTABLE hht, const void * key, const void * data);

/*
*
*插入一条记录,不允许重复
*
*/
extern HMYHASHTABLE_ITER MyHashTableInsertUnique(HMYHASHTABLE hht, const void * key, const void * data);

/*
*
*删除一条记录,根据key,
*成功删除返回0, 否则返回-1
*
*/
extern int MyHashTableDelKey(HMYHASHTABLE hht, const void * key, void ** key_ret, void ** data);

/*
*
*删除一条记录,根据迭代器
*成功删除返回0, 否则返回-1
*
*/
extern int MyHashTableDelIter(HMYHASHTABLE hht, HMYHASHTABLE_ITER iter, void ** key, void ** data);

/*
*
*查找一条记录
*
*/
extern HMYHASHTABLE_ITER MyHashTableSearch(const HMYHASHTABLE hht, const void * key);

/*
*
*获取迭代器的值域
*
*/
extern void * MyHashTableGetIterData(const HMYHASHTABLE_ITER it);

/*
*
*设置迭代器的值域
*
*/
extern void MyHashTableSetIterData(const HMYHASHTABLE_ITER it, const void * data);

/*
*
*获取迭代器的键值
*
*/
extern const void * MyHashTableGetIterKey(HMYHASHTABLE_ITER it);

/*
*
*获取元素个数
*
*/
extern size_t MyHashTableGetElementCount(const HMYHASHTABLE hht);

/*
*
*获取迭代器的键值
*
*/
extern HMYHASHTABLE_ITER MyHashTableBegin(HMYHASHTABLE hht);

/*
*
*获取一下迭代器
*
*/
extern HMYHASHTABLE_ITER MyHashTableGetNext(HMYHASHTABLE hht, HMYHASHTABLE_ITER it);

/*
*
*获取一下迭代器
*
*/
extern void MyHashTableClear(HMYHASHTABLE hht);

/*
*
*打印哈希表
*
*/
extern void MyHashTablePrint(const HMYHASHTABLE hht);


#endif





























/*
*
* 引用计数，动态增长的缓冲区，多线程不安全 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/

#ifndef __MYBUFFER_H__
#define __MYBUFFER_H__


#include "mymempool.h"


struct __mybuffer_t_;
typedef struct __mybuffer_t_* HMYBUFFER;


/*
*
*构造
*
*/
extern HMYBUFFER MyBufferConstruct(HMYMEMPOOL hm, size_t s);

/*
*
*析构
*
*/
extern void MyBufferDestruct(HMYBUFFER hb);

/*
*
*引用
*
*/
extern void MyBufferRef(HMYBUFFER hb_src, HMYBUFFER hb_dst);

/*
*
*深拷贝
*
*/
extern void MyBufferDeepCopy(HMYBUFFER hb_src, HMYBUFFER hb_dst);

/*
*
*获取缓冲区
*
*/
extern void * MyBufferGet(HMYBUFFER hb, size_t * len);

/*
*
*设置缓冲区
*
*/
extern int MyBufferSet(HMYBUFFER hb, const void * b, size_t len);

/**
 * @brief 将缓冲区中的内容置空,实际是将内容长度置零
 */
extern int MyBufferClear(HMYBUFFER hb);

/*
*
*拼接缓冲区
*
*/
extern int MyBufferAppend(HMYBUFFER hb, const void * b, size_t len);

/*
*
*两个缓冲区对象拼接
*
*/
extern size_t MyBufferCat(HMYBUFFER hb_src, HMYBUFFER hb_dst);

/*
*
*获取缓冲区的容量
*
*/
extern size_t MyBufferGetCapability(HMYBUFFER hb);

/*
*
*获取缓冲区的引用计数
*
*/
extern int MyBufferGetRefCount(HMYBUFFER hb);

/*
*
*查看buffer
*
*/
extern void MyBufferLook(HMYBUFFER hb);

#endif














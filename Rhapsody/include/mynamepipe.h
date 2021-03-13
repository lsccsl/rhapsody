/**
 *
 * @file mynamepipe.c 命名管道 2007-8-23 23:41
 *
 * @author diablo 
 *
 */
#ifndef __MYNAMEPIPE_H__
#define __MYNAMEPIPE_H__


#include "mymempool.h"

 
typedef struct __mynamepipe_handle_
{int unused;}*HMYNAMEPIPE;
 
 
/**
 * @brief 创建命名管道
 */
extern HMYNAMEPIPE MyNamePipeConstruct(HMYMEMPOOL hm, const char * name, int bcreate);
 
/**
 * @brief 销毁命名管道
 */
extern void MyNamePipeDestruct(HMYNAMEPIPE hnp);

/**
 * @brief 获取管道的fd
 */
extern int MyNamePipeGetFd(HMYNAMEPIPE hnp);

/**
 * @brief 写
 */
extern int MyNamePipeWrite(HMYNAMEPIPE hnp, const void * data, size_t data_len);

/**
 * @brief 读
 */
extern int MyNamePipeRead(HMYNAMEPIPE hnp, void * data, size_t data_len);


#endif
 
 
 
 
 
 
 




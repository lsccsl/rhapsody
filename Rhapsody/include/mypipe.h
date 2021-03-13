/*
*
*mypipe.h 读写管道 lin shao chuan
*
*/
#ifndef __MYPIPE_H__
#define __MYPIPE_H__


#include "mymempool.h"


typedef struct __mypipe_handle_
{int unused;}*HMYPIPE;


/*
*
*创建管道
*
*/
extern HMYPIPE MyPipeConstruct(HMYMEMPOOL hp);

/*
*
*销毁管道
*
*/
extern void MyPipeDestruct(HMYPIPE hp);

/**
 * @brief 设成非阻塞
 */
extern void MyPipeNoBlock(HMYPIPE hp);

/*
*
*读管道
*
*/
extern int MyPipeRead(HMYPIPE hp, void * data, size_t data_len);

/**
 * @brief 管道读
 */
extern int MyPipeSelectRead(HMYPIPE hp, void * data, size_t data_len, int time_out);

/*
*
*写管道
*
*/
extern int MyPipeWrite(HMYPIPE hp, void * data, size_t data_len);

/**
 * @brief 管道写
 */
extern int MyPipeSelectWrite(HMYPIPE hp, void * data, size_t data_len, int time_out);

/*
*
*获取读fd
*
*/
extern int MyPipeGetReadFD(HMYPIPE hp);

/*
*
*获取写fd
*
*/
extern int MyPipeGetWriteFD(HMYPIPE hp);


#endif


















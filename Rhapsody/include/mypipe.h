/*
*
*mypipe.h ��д�ܵ� lin shao chuan
*
*/
#ifndef __MYPIPE_H__
#define __MYPIPE_H__


#include "mymempool.h"


typedef struct __mypipe_handle_
{int unused;}*HMYPIPE;


/*
*
*�����ܵ�
*
*/
extern HMYPIPE MyPipeConstruct(HMYMEMPOOL hp);

/*
*
*���ٹܵ�
*
*/
extern void MyPipeDestruct(HMYPIPE hp);

/**
 * @brief ��ɷ�����
 */
extern void MyPipeNoBlock(HMYPIPE hp);

/*
*
*���ܵ�
*
*/
extern int MyPipeRead(HMYPIPE hp, void * data, size_t data_len);

/**
 * @brief �ܵ���
 */
extern int MyPipeSelectRead(HMYPIPE hp, void * data, size_t data_len, int time_out);

/*
*
*д�ܵ�
*
*/
extern int MyPipeWrite(HMYPIPE hp, void * data, size_t data_len);

/**
 * @brief �ܵ�д
 */
extern int MyPipeSelectWrite(HMYPIPE hp, void * data, size_t data_len, int time_out);

/*
*
*��ȡ��fd
*
*/
extern int MyPipeGetReadFD(HMYPIPE hp);

/*
*
*��ȡдfd
*
*/
extern int MyPipeGetWriteFD(HMYPIPE hp);


#endif


















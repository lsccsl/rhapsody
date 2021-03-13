/**
 *
 * @file mynamepipe.c �����ܵ� 2007-8-23 23:41
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
 * @brief ���������ܵ�
 */
extern HMYNAMEPIPE MyNamePipeConstruct(HMYMEMPOOL hm, const char * name, int bcreate);
 
/**
 * @brief ���������ܵ�
 */
extern void MyNamePipeDestruct(HMYNAMEPIPE hnp);

/**
 * @brief ��ȡ�ܵ���fd
 */
extern int MyNamePipeGetFd(HMYNAMEPIPE hnp);

/**
 * @brief д
 */
extern int MyNamePipeWrite(HMYNAMEPIPE hnp, const void * data, size_t data_len);

/**
 * @brief ��
 */
extern int MyNamePipeRead(HMYNAMEPIPE hnp, void * data, size_t data_len);


#endif
 
 
 
 
 
 
 




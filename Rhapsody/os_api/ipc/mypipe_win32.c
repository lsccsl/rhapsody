/**
* @file mypipe_win32.c ����win32�����ܵ� 2008-03-25 23:25
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

#include "mypipe.h"

#include <windows.h>
#include <assert.h>

#include "mylog.h"
#include "os_def.h"


typedef struct __mypipe_t_
{
	HANDLE read_fd;
	HANDLE write_fd;

	HMYMEMPOOL hm;
}mypipe_t;


/**
 * @brief �����ܵ�
 */
HMYPIPE MyPipeConstruct(HMYMEMPOOL hm)
{
	mypipe_t * p = NULL;
	p = MyMemPoolMalloc(hm, sizeof(*p));
	assert(p);

	memset(p, 0, sizeof(*p));

	p->hm = hm;

	if(!CreatePipe(&p->read_fd, &p->write_fd, NULL, 0))
		LOG_WARN(("fuck windows!"));

	return (HMYPIPE)p;
}

/**
 * @brief ���ٹܵ�
 */
void MyPipeDestruct(HMYPIPE hp)
{
	mypipe_t * p = (mypipe_t *)hp;
	if(NULL == p)
		return;

	CloseHandle(p->read_fd);
	CloseHandle(p->write_fd);

	MyMemPoolFree(p->hm, p);
}

/**
 * @brief ���ܵ�
 */
int MyPipeRead(HMYPIPE hp, void * data, size_t data_len)
{
	unsigned long got;
	mypipe_t * p = (mypipe_t *)hp;
	if(NULL == p ||	NULL == data || 0 == data_len)
		return 0;

	ReadFile(p->read_fd, data, (unsigned long)data_len, &got, NULL);

	return got;
}

/**
 * @brief д�ܵ�
 */
int MyPipeWrite(HMYPIPE hp, void * data, size_t data_len)
{
	unsigned long wrote = 0;
	mypipe_t * p = (mypipe_t *)hp;
	if(NULL == p ||	NULL == data || 0 == data_len)
		return 0;

	WriteFile(p->write_fd, data, (unsigned long)data_len, &wrote, NULL);

	return wrote;
}

/**
 * @brief ��ȡ��fd
 */
int MyPipeGetReadFD(HMYPIPE hp)
{
#pragma warning(disable:4311)
	mypipe_t * p = (mypipe_t *)hp;	
	if(NULL == p)
		return (int)INVALID_FD;

	return (int)p->read_fd;
}

/**
 * @brief ��ȡдfd
 */
int MyPipeGetWriteFD(HMYPIPE hp)
{
#pragma warning(disable:4311)
	mypipe_t * p = (mypipe_t *)hp;	
	if(NULL == p)
		return (int)INVALID_FD;

	return (int)p->write_fd;
}









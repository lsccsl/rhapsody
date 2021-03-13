/**
* @file mynamepipe_win32.c ����win32�������ܵ� 2008-03-25 23:25
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
#include "mynamepipe.h"

#include <windows.h>
#include <assert.h>

#include "os_def.h"
#include "mybuffer.h"


typedef struct __mynamepipe_t_
{
	/* ��Źܵ����ֵĻ����� */
	HMYBUFFER hbuf_name;
	
	/* ��¼�Ƿ��ǵ�ǰ���󴴽�������ܵ�,���ڶ�������ʱ�ж��Ƿ�unlink */
	int bcreate;
	
	/* �ܵ���fd */
	HANDLE hpipe;
	///* �ܵ��ļ��ľ����fd */
	//HANDLE hpipefile;
	
	/* �ڴ�ر�ʶ */
	HMYMEMPOOL hm;
}mynamepipe_t;


/**
 * @brief ���ٹܵ�
 */
static int namepipe_destroy(mynamepipe_t * p)
{
	assert(p);

	MyBufferDestruct(p->hbuf_name);

	//if(p->bcreate)
	//{
	//	assert(p->hpipe != (HANDLE)INVALID_FD);
	//	CloseHandle(p->hpipe);
	//}

	CloseHandle(p->hpipe);

	MyMemPoolFree(p->hm, p);

	return 0;
}

/**
 * @brief ���������ܵ�
 */
HMYNAMEPIPE MyNamePipeConstruct(HMYMEMPOOL hm, const char * name, int bcreate)
{
#define DEFAULT_BUFFER_LEN 4096
#define SYS_PIPE_PATH "\\\\.\\pipe\\"

	mynamepipe_t * p = NULL;
	if(NULL == name)
		return NULL;

	p = MyMemPoolMalloc(hm, sizeof(*p));
	assert(p);

	memset(p, 0, sizeof(*p));
	p->hm = hm;

	p->hbuf_name = MyBufferConstruct(hm, 0);
	assert(p->hbuf_name);

	MyBufferSet(p->hbuf_name, SYS_PIPE_PATH, strlen(SYS_PIPE_PATH));
	MyBufferAppend(p->hbuf_name, name, strlen(name));
	MyBufferAppend(p->hbuf_name, "\0", 1);

	if(bcreate)
	{
		p->hpipe = CreateNamedPipe(MyBufferGet(p->hbuf_name, NULL), PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			DEFAULT_BUFFER_LEN,
			DEFAULT_BUFFER_LEN,
			NMPWAIT_USE_DEFAULT_WAIT,
			NULL);

		if(INVALID_FD == p->hpipe)
			goto MyNamePipeConstruct_end_;

		p->bcreate = 1;
	}
	else
	{
		p->hpipe = CreateFile(MyBufferGet(p->hbuf_name, NULL), GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL);
	}

	if(INVALID_FD == p->hpipe)
		goto MyNamePipeConstruct_end_;

	return (HMYNAMEPIPE)p;

MyNamePipeConstruct_end_:

	if(p)
		namepipe_destroy(p);

	return NULL;
}
 
/**
 * @brief ���������ܵ�
 */
void MyNamePipeDestruct(HMYNAMEPIPE hnp)
{
	mynamepipe_t * p = (mynamepipe_t *)hnp;
	if(NULL == p)
		return;

	namepipe_destroy(p);
}
 
/**
 * @brief д
 */
int MyNamePipeWrite(HMYNAMEPIPE hnp, const void * data, size_t data_len)
{
	unsigned long wrote = 0;
	mynamepipe_t * p = (mynamepipe_t *)hnp;		
	if(NULL == p ||	NULL == data || 0 == data_len)
		return 0;

	WriteFile(p->hpipe, data, (unsigned long)data_len, &wrote, NULL);

	return wrote;
}
 
/**
 * @brief ��
 */
int MyNamePipeRead(HMYNAMEPIPE hnp, void * data, size_t data_len)
{
	unsigned long got;
	mynamepipe_t * p = (mynamepipe_t *)hnp;
	if(NULL == p ||	NULL == data || 0 == data_len)
		return 0;

	ReadFile(p->hpipe, data, (unsigned long)data_len, &got, NULL);

	return got;
}

/**
 * @brief ��ȡ�ܵ���fd
 */
int MyNamePipeGetFd(HMYNAMEPIPE hnp)
{
#pragma warning(disable:4311)

	mynamepipe_t * p = (mynamepipe_t *)hnp;
	if(NULL == p)
		return (int)INVALID_FD;
		
	return (int)p->hpipe;
}
















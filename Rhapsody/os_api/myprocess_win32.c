/**
* @file myprocess_win32.c 描述如何起进程 2008-03-25 23:25
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
#include "myprocess.h"

#include <windows.h>
#include <assert.h>

#include "mybuffer.h"


static int create_win_process(const char * path, const char * appname, char * param[], PROCESS_INFORMATION * pi)
{
	int ret = 0;
	HMYBUFFER cmd_line = NULL;
	STARTUPINFO si;

	assert(pi);

	ZeroMemory(pi, sizeof(*pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	cmd_line = MyBufferConstruct(NULL, 128);
	if(param)
	{
		int i = 0;

		MyBufferSet(cmd_line, appname, strlen(appname));

		while(param[i])
		{
			MyBufferAppend(cmd_line, " ", 1);
			MyBufferAppend(cmd_line, param[i], strlen(param[i]));

			i ++;
		}

		MyBufferAppend(cmd_line, "\0", 1);
	}
	else
	{
		MyBufferSet(cmd_line, appname, strlen(appname));
		MyBufferAppend(cmd_line, "\0", 1);
	}

	appname = MyBufferGet(cmd_line, NULL);

	ret = CreateProcess(NULL, (char *)appname, NULL, NULL, TRUE/*FALSE*/, 0, NULL, path, &si, pi);

	if(cmd_line)
		MyBufferDestruct(cmd_line);

	return ret ? 0 : 1;
}

/**
 * @brief 创建进程
 */
int UOS_CreateProcess(const char * path, const char * appname, char * param[])
{
	PROCESS_INFORMATION pi;
	return create_win_process(path, appname, param, &pi);
}

/**
 * @brief 创建进程并等待
 */
int UOS_CreateProcessAndWait(const char * path, const char * appname, char * param[])
{
	PROCESS_INFORMATION pi;

	if(0 != create_win_process(path, appname, param, &pi))
		return -1;

	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 0;
}












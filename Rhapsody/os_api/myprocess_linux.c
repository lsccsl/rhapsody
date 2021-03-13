/**
* @file myprocess_linux.c 描述如何起进程 2008-03-25 23:25
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

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>

#include "mylog.h"


static pid_t create_process(const char * path, const char * appname, char * param[])
{
	pid_t pid = 0;

	LOG_DEBUG(("create_process"));

	pid = vfork();	

	LOG_DEBUG(("vfork over"));

	if(pid < 0)
	{
		LOG_WARN(("fork err"));
		return -1;
	}

	if(pid == 0)
	{
		LOG_DEBUG(("execvp"));
		if(execvp(appname, param) == -1)
		{
			LOG_WARN(("execvp err"));

		}
		else
		{
			LOG_DEBUG(("execvp ok"));
		}

		exit(0);
	}
	else
	{
		return pid;
	}
}

/**
 * @brief 创建进程
 */
int UOS_CreateProcess(const char * path, const char * appname, char * param[])
{
	LOG_DEBUG(("UOS_CreateProcess"));

	if(create_process(path, appname, param) < 0)
		return  -1;

	return 0;
}

/**
 * @brief 创建进程并等待
 */
int UOS_CreateProcessAndWait(const char * path, const char * appname, char * param[])
{
	pid_t pid = 0;

	LOG_DEBUG(("UOS_CreateProcessAndWait"));

	pid = create_process(path, appname, param);

	if(pid < 0)
		return -1;

	waitpid(pid, NULL, 0);

	return 0;
}











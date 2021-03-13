/**
* @file myprocess.c 描述如何起进程 2008-03-25 23:25
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
#ifndef __MYPROCESS_H__
#define __MYPROCESS_H__


/**
 * @brief 创建进程
 */
extern int UOS_CreateProcess(const char * path, const char * appname, char * param[]);

/**
 * @brief 创建进程并等待
 */
extern int UOS_CreateProcessAndWait(const char * path, const char * appname, char * param[]);


#endif













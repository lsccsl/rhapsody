/**
* @file myprocess.c ������������ 2008-03-25 23:25
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
 * @brief ��������
 */
extern int UOS_CreateProcess(const char * path, const char * appname, char * param[]);

/**
 * @brief �������̲��ȴ�
 */
extern int UOS_CreateProcessAndWait(const char * path, const char * appname, char * param[]);


#endif













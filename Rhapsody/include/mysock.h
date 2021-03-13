/**
* @file mysock.h 描述sock相关的封装,目前主要提供udp与tcp 2008-03-25 23:25
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

#include "mymempool.h"


struct __uos_tcp_t_;
typedef struct __uos_tcp_t_ * HUOSTCP;

struct __uos_udp_t_;
typedef struct __uos_udp_t_ * HUOSUDP;


/**
 * @brief 创建udp socket
 */
extern HUOSUDP UOS_UDPConstruct(HMYMEMPOOL hm, const char * ip, unsigned short * port);

/**
 * @brief 销毁
 */
extern int UOS_UDPDestruct(HUOSUDP hudp);

/**
 * @brief 写udp
 */
extern int UOS_UDPWriteTo(HUOSUDP hudp, const void * buf, size_t buf_sz,
						const char * ip, const unsigned short port);

/**
 * @brief 读udp
 */
extern int UOS_UDPReadFrom(HUOSUDP hudp, void * buf, size_t buf_sz, 
					   char * ip, size_t ip_sz, unsigned short * port);

/**
 * @brief 获取udp的fd
 */
extern int UOS_UDPGetFd(HUOSUDP hudp);


/**
 * @brief 创建tcp socket
 */
extern HUOSTCP UOS_TCPConstruct(HMYMEMPOOL hm, const char * ip, unsigned short * port);

/**
 * @brief tcp监听远端连接请求
 */
extern int UOS_TCPListerner(HUOSTCP htcp);

/**
 * @brief 接受对端的tcp连接请求
 */
extern HUOSTCP UOS_TCPAccept(HUOSTCP htcp, char * ip, size_t ip_sz, unsigned short * port);

/**
 * @brief 销毁一个tcp对象
 */
extern void UOS_TCPDestruct(HUOSTCP htcp);

/**
 * @brief 连接到对端
 */
extern int UOS_TCPConnect(HUOSTCP htcp, char * ip, unsigned short port);

/**
 * @brief 写tcp
 */
extern int UOS_TCPWrite(HUOSTCP htcp, const void * buf, size_t buf_sz);

/**
 * @brief 写tcp,如果未写足,在指定的次数内重试
 */
extern int UOS_TCPCompleteWrite(HUOSTCP htcp, const void * buf, size_t buf_sz, int retry_count);

/**
 * @brief 读tcp
 */
extern int UOS_TCPRead(HUOSTCP htcp, void * buf, size_t buf_sz);

/**
 * @brief 读tcp,如果未读足,在指定的次数内重试
 */
extern int UOS_TCPCompleteRead(HUOSTCP htcp, const void * buf, size_t buf_sz, int retry_count);

/**
 * @brief 获取tcp的fd
 */
extern int UOS_TCPGetFd(HUOSTCP htcp);













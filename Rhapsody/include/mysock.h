/**
* @file mysock.h ����sock��صķ�װ,Ŀǰ��Ҫ�ṩudp��tcp 2008-03-25 23:25
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
 * @brief ����udp socket
 */
extern HUOSUDP UOS_UDPConstruct(HMYMEMPOOL hm, const char * ip, unsigned short * port);

/**
 * @brief ����
 */
extern int UOS_UDPDestruct(HUOSUDP hudp);

/**
 * @brief дudp
 */
extern int UOS_UDPWriteTo(HUOSUDP hudp, const void * buf, size_t buf_sz,
						const char * ip, const unsigned short port);

/**
 * @brief ��udp
 */
extern int UOS_UDPReadFrom(HUOSUDP hudp, void * buf, size_t buf_sz, 
					   char * ip, size_t ip_sz, unsigned short * port);

/**
 * @brief ��ȡudp��fd
 */
extern int UOS_UDPGetFd(HUOSUDP hudp);


/**
 * @brief ����tcp socket
 */
extern HUOSTCP UOS_TCPConstruct(HMYMEMPOOL hm, const char * ip, unsigned short * port);

/**
 * @brief tcp����Զ����������
 */
extern int UOS_TCPListerner(HUOSTCP htcp);

/**
 * @brief ���ܶԶ˵�tcp��������
 */
extern HUOSTCP UOS_TCPAccept(HUOSTCP htcp, char * ip, size_t ip_sz, unsigned short * port);

/**
 * @brief ����һ��tcp����
 */
extern void UOS_TCPDestruct(HUOSTCP htcp);

/**
 * @brief ���ӵ��Զ�
 */
extern int UOS_TCPConnect(HUOSTCP htcp, char * ip, unsigned short port);

/**
 * @brief дtcp
 */
extern int UOS_TCPWrite(HUOSTCP htcp, const void * buf, size_t buf_sz);

/**
 * @brief дtcp,���δд��,��ָ���Ĵ���������
 */
extern int UOS_TCPCompleteWrite(HUOSTCP htcp, const void * buf, size_t buf_sz, int retry_count);

/**
 * @brief ��tcp
 */
extern int UOS_TCPRead(HUOSTCP htcp, void * buf, size_t buf_sz);

/**
 * @brief ��tcp,���δ����,��ָ���Ĵ���������
 */
extern int UOS_TCPCompleteRead(HUOSTCP htcp, const void * buf, size_t buf_sz, int retry_count);

/**
 * @brief ��ȡtcp��fd
 */
extern int UOS_TCPGetFd(HUOSTCP htcp);













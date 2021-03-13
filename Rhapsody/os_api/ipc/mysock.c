/**
* @file mysock.c 描述sock相关的封装,目前主要提供udp与tcp 2008-03-25 23:25
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
#include "mysock.h"

#ifdef WIN32
	#include <winsock2.h>
	#include <io.h>
#else
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <string.h>
	#include <unistd.h>
#endif

#include <assert.h>

#include "mybuffer.h"
#include "os_def.h"


typedef struct sockaddr SA;


typedef struct __uos_udp_t_
{
	unsigned short port;
	HMYBUFFER hip;

	int fd;

	HMYMEMPOOL hm;
}uos_udp_t;


#ifdef WIN32
	#pragma warning(disable:4311)
	static int ini_winsock()
	{
		int err = -1;
		WSADATA wsadata;

		static int ini_flag = 0;
		if(ini_flag)
			return 0;

		WSAStartup(0x0202, &wsadata);
		err = WSAGetLastError();
		if(err != 0)
			return -1;

		ini_flag = 1;
		return 0;
	}
#else
	#define ini_winsock() ((void)0)
#endif


/**
 * @brief 创建命名管道
 */
static int socket_bind(int fd, const char * ip, unsigned short * port)
{
#define MAX_BIND_RETRY 10

	int i;
	struct sockaddr_in stSockAddr;

	assert(ip && port);

	memset(&stSockAddr, 0, sizeof(stSockAddr));
	for(i = 0; i < MAX_BIND_RETRY; i ++)
	{
		int ret = 0;

		stSockAddr.sin_family = AF_INET;
		stSockAddr.sin_addr.s_addr = inet_addr(ip);
		stSockAddr.sin_port = htons(*port);

		ret = bind(fd, (SA *)&stSockAddr, sizeof(stSockAddr));
		if(0 == ret)
			return 0;

		(*port) ++;
	}

	return -1;

#undef MAX_BIND_RETRY
}


/**
 * @brief 创建命名管道
 */
static int udp_destroy(uos_udp_t * udp)
{
	assert(udp);

	if(udp->hip)
		MyBufferDestruct(udp->hip);

#ifdef WIN32
	closesocket(udp->fd);
#else
	close(udp->fd);
#endif

	MyMemPoolFree(udp->hm, udp);

	return 0;
}


/**
 * @brief 创建命名管道
 */
HUOSUDP UOS_UDPConstruct(HMYMEMPOOL hm, const char * ip, unsigned short * port)
{
	uos_udp_t * udp = NULL;

	assert(port && ip);

#ifdef WIN32
	if(0 != ini_winsock())
		return NULL;
#endif

	udp = MyMemPoolMalloc(hm, sizeof(*udp));
	if(NULL == udp)
		return NULL;

	udp->hm = hm;
	udp->port = *port;

	udp->hip = MyBufferConstruct(hm, strlen(ip) + 1);
	if(NULL == udp->hip)
		goto UOS_UDPConstruct_err_;

	MyBufferSet(udp->hip, ip, strlen(ip));
	MyBufferAppend(udp->hip, "\0", 1);

	udp->fd = (int)socket(AF_INET, SOCK_DGRAM,0);
	if((int)INVALID_FD == udp->fd)
		goto UOS_UDPConstruct_err_;

	if(0 != socket_bind(udp->fd, ip, &udp->port))
		goto UOS_UDPConstruct_err_;

	*port = udp->port;

	return udp;

UOS_UDPConstruct_err_:

	if(udp)
		udp_destroy(udp);

	return NULL;
}

/**
 * @brief 创建命名管道
 */
int UOS_UDPDestruct(HUOSUDP hudp)
{
	if(NULL == hudp)
		return -1;

	return udp_destroy(hudp);
}

/**
 * @brief 写udp
 */
int UOS_UDPWriteTo(HUOSUDP hudp, const void * buf, size_t buf_sz,
				   const char * ip, const unsigned short port)
{
	struct sockaddr_in saddr;

	if(NULL == hudp || NULL == buf || 0 == buf_sz)
		return -1;

	if((int)INVALID_FD == hudp->fd)
		return -1;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ip);
	saddr.sin_port = htons(port);

	return sendto(hudp->fd, buf, (int)buf_sz, 0, (SA *)&saddr, sizeof(saddr));
}

/**
 * @brief 读udp
 */
int UOS_UDPReadFrom(HUOSUDP hudp, void * buf, size_t buf_sz,
				char * ip, size_t ip_sz, unsigned short * port)
{
	int ret = 0;
	struct sockaddr_in saddr;
	int sadd_sz = sizeof(saddr);

	if(NULL == hudp || NULL == buf || 0 == buf_sz)
		return -1;

	ret = recvfrom(hudp->fd, buf, (int)buf_sz, 0, (SA *)&saddr, (int*)&sadd_sz);
	if(ret > 0)
	{
		if(port)
			*port = ntohs(saddr.sin_port);
		if(ip && ip_sz)
			strncpy(ip, inet_ntoa(saddr.sin_addr), ip_sz);
	}

	return ret;
}

/**
 * @brief 获取udp的fd
 */
int UOS_UDPGetFd(HUOSUDP hudp)
{
	if(NULL == hudp)
		return (int)INVALID_FD;

	return hudp->fd;
}


typedef struct __uos_tcp_t_
{
	unsigned short port;
	HMYBUFFER hip;

	int fd;

	HMYMEMPOOL hm;
}uos_tcp_t;


/**
 * @brief 创建命名管道
 */
static void tcp_destroy(uos_tcp_t * tcp)
{
	assert(tcp);

	if(tcp->hip)
		MyBufferDestruct(tcp->hip);

#ifdef WIN32
	closesocket(tcp->fd);
#else
	close(tcp->fd);
#endif

	MyMemPoolFree(tcp->hm, tcp);

	return;
}

/**
 * @brief 创建命名管道
 */
HUOSTCP UOS_TCPConstruct(HMYMEMPOOL hm, const char * ip, unsigned short * port)
{
	uos_tcp_t * tcp = NULL;

	assert(port && ip);

#ifdef WIN32
	if(0 != ini_winsock())
		return NULL;
#endif

	tcp = MyMemPoolMalloc(hm, sizeof(*tcp));
	if(NULL == tcp)
		return NULL;

	tcp->hm = hm;
	tcp->port = *port;

	tcp->hip = MyBufferConstruct(hm, strlen(ip) + 1);
	if(NULL == tcp->hip)
		goto UOS_TCPConstruct_err_;

	MyBufferSet(tcp->hip, ip, strlen(ip));
	MyBufferAppend(tcp->hip, "\0", 1);

	tcp->fd = (int)socket(AF_INET, SOCK_STREAM,0);
	if((int)INVALID_FD == tcp->fd)
		goto UOS_TCPConstruct_err_;

	if(0 != socket_bind(tcp->fd, ip, &tcp->port))
		goto UOS_TCPConstruct_err_;

	*port = tcp->port;
	
	return tcp;

UOS_TCPConstruct_err_:

	if(tcp)
		tcp_destroy(tcp);

	return NULL;
}

/**
 * @brief tcp监听远端连接请求
 */
int UOS_TCPListerner(HUOSTCP htcp)
{
	if(NULL == htcp)
		return -1;

	if((int)INVALID_FD == htcp->fd)
		return -1;

	if(0 != listen(htcp->fd, SOMAXCONN))
		return -1;

	return 0;
}

/**
 * @brief 接受对端的tcp连接请求
 */
HUOSTCP UOS_TCPAccept(HUOSTCP htcp, char * ip, size_t ip_sz, unsigned short * port)
{
	uos_tcp_t * accept_tcp = NULL;
	struct sockaddr_in saddr;
	int sadd_sz = sizeof(saddr);

	assert(ip && ip_sz && port);

	accept_tcp = MyMemPoolMalloc(htcp->hm, sizeof(*accept_tcp));
	memset(accept_tcp, 0, sizeof(*accept_tcp));

	accept_tcp->hm = htcp->hm;

	accept_tcp->fd = (int)accept(htcp->fd, (struct sockaddr *)&saddr, &sadd_sz);
	if((int)INVALID_FD == accept_tcp->fd)
		goto UOS_TCPAccept_err_;

	if(inet_ntoa(saddr.sin_addr) && ip && ip_sz)
		strncpy(ip, inet_ntoa(saddr.sin_addr), ip_sz);

	if(port)
		*port = ntohs(saddr.sin_port);

	return accept_tcp;

UOS_TCPAccept_err_:

	tcp_destroy(accept_tcp);

	return NULL;
}

/**
 * @brief 销毁一个tcp对象
 */
void UOS_TCPDestruct(HUOSTCP htcp)
{
	if(NULL == htcp)
		return;

	tcp_destroy(htcp);
}

/**
 * @brief 连接到对端
 */
int UOS_TCPConnect(HUOSTCP htcp, char * ip, unsigned short port)
{
	struct sockaddr_in saddr;

	if(NULL == htcp || NULL == ip || (int)INVALID_FD == htcp->fd)
		return -1;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ip);
	saddr.sin_port = htons(port);

	if(0 != connect(htcp->fd, (SA *)&saddr, sizeof(saddr)))
		return -1;

	return 0;
}

/**
 * @brief 写tcp
 */
int UOS_TCPWrite(HUOSTCP htcp, const void * buf, size_t buf_sz)
{
	if(NULL == htcp || NULL == buf || 0 == buf_sz || (int)INVALID_FD == htcp->fd)
		return -1;

	return send(htcp->fd, buf, (int)buf_sz, 0);
}

/**
 * @brief 写tcp,如果未写足,在指定的次数内重试
 */
int UOS_TCPCompleteWrite(HUOSTCP htcp, const void * buf, size_t buf_sz, int retry_count)
{
	size_t write_sz = 0;

	if(NULL == htcp || NULL == buf || 0 == buf_sz || (int)INVALID_FD == htcp->fd)
		return -1;

	while(write_sz < buf_sz)
	{
		int ret = send(htcp->fd, (unsigned char *)buf + write_sz, (int)(buf_sz - write_sz), 0);
		if(-1 != ret)
			write_sz += ret;

		if(retry_count <= 0)
			break;

		retry_count --;
	}

	return (write_sz == buf_sz) ? 0 : -1;
}

/**
 * @brief 读tcp
 */
int UOS_TCPRead(HUOSTCP htcp, void * buf, size_t buf_sz)
{
	if(NULL == htcp || NULL == buf || 0 == buf_sz || (int)INVALID_FD == htcp->fd)
		return -1;

	return recv(htcp->fd, buf, (int)buf_sz, 0);
}

/**
 * @brief 读tcp,如果未读足,在指定的次数内重试
 */
int UOS_TCPCompleteRead(HUOSTCP htcp, const void * buf, size_t buf_sz, int retry_count)
{
	size_t write_sz = 0;

	if(NULL == htcp || NULL == buf || 0 == buf_sz || (int)INVALID_FD == htcp->fd)
		return -1;

	while(write_sz < buf_sz)
	{
		int ret = recv(htcp->fd, (unsigned char *)buf + write_sz, (int)(buf_sz - write_sz), 0);
		if(-1 != ret)
			write_sz += ret;

		if(retry_count <= 0)
			break;

		retry_count --;
	}

	return (write_sz == buf_sz) ? 0 : -1;
}

/**
 * @brief 获取tcp的fd
 */
int UOS_TCPGetFd(HUOSTCP htcp)
{
	if(NULL == htcp || (int)INVALID_FD == htcp->fd)
		return (int)INVALID_FD;

	return htcp->fd;
}










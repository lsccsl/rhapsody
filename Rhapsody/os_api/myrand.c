/**
 * @file myrand.c 随机数发生器 2008-02-18 21:59
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
#include "myrand.h"

#include <string.h>
#include <time.h>

#ifdef WIN32
	#include <windows.h>
#else
	#include <stdio.h>
	#include <fcntl.h>
	#include <termio.h>
	#include <unistd.h>
#endif

#include "./sync/mymutex.h"


typedef struct __myrand_t_
{
	/* State variables */
    unsigned char i, j;

	/* State variables */
    unsigned char s[256];

	/* 保护锁 */
	HMYMUTEX protector;

	/* 内存池 */
	HMYMEMPOOL hm;
}myrand_t;


/**
 * @brief 销毁随机数发生器
 */
static void rand_destroy(myrand_t * r)
{
	HMYMUTEX hmtx = NULL;

	if(NULL == r)
		return;

	hmtx = r->protector;

	if(hmtx)
		MyMutexLock(hmtx);

	MyMemPoolFree(r->hm, r);

	if(hmtx)
		MyMutexUnLock(hmtx);

	if(hmtx)
		MyMutexDestruct(hmtx);
}

/**
 * @brief 创建随机数发生器
 * @param bshare:是否多线程共享
 */
HMYRAND myrandConstruct(HMYMEMPOOL hm, void * rand_seed, size_t rand_seed_len, int bshare)
{
	int i;
	unsigned char t = 0;
	char * k = NULL;
	myrand_t * r = NULL;

	k = MyMemPoolMalloc(hm, sizeof( ((myrand_t *)(0)) -> s));
	if(NULL == k)
		goto myrandConstruct_err_;
	memset(k, 0, sizeof( ((myrand_t *)(0)) -> s));

    r = MyMemPoolMalloc(hm, sizeof(*r));
	if(NULL == r)
		goto myrandConstruct_err_;

	r->hm = hm;

    if(bshare)
	{
		r->protector = MyMutexConstruct(hm);
		if(NULL == r->protector)
			goto myrandConstruct_err_;
	}
	else
		r->protector = NULL;

	if(rand_seed && rand_seed_len)
		memcpy(k, rand_seed, sizeof( ((myrand_t *)(0)) -> s ) > rand_seed_len ? rand_seed_len : sizeof( ((myrand_t *)(0)) -> s ));

	r->j = 0;
	r->i = 0;
	for(i = 0; i < sizeof(r->s); i ++)
		r->s[i] = i;

	for(i = 0; i < sizeof(r->s); i ++)
	{
		r->j += r->s[i] + k[i];
		t = r->s[r->j];
		r->s[r->j] = r->s[i];
		r->s[i] = t;
	}

	if(k)
		MyMemPoolFree(hm, k);

	return r;

myrandConstruct_err_:

	if(k)
		MyMemPoolFree(hm, k);

	if(r)
		rand_destroy(r);

	return NULL;
}

/**
 * @brief 销毁随机数发生器
 */
void myrandDestruct(HMYRAND hr)
{
	if(hr)
		rand_destroy(hr);
}

/**
 * @brief 获取一个8bit的随机数
 */
unsigned char myrandGetByte(HMYRAND hr)
{
	unsigned char ret = 0;
	unsigned char t = 0;

	if(NULL == hr)
		return 0;

	if(hr->protector)
		if(0 != MyMutexLock(hr->protector))
			return 0;

	hr->i++;
	t = hr->s[hr->i];
	hr->j += t;
	hr->s[hr->i] = hr->s[hr->j];
	hr->s[hr->j] = t;
	t += hr->s[hr->i];
	ret = hr->s[t];

	if(hr->protector)
		MyMutexUnLock(hr->protector);

	return ret;
}

/**
 * @brief 获取初始化随机种子
 */
int myrandSeed(void * rand_seed, size_t rand_seed_size)
{
#ifdef WIN32
	SYSTEMTIME systime;

	if(NULL == rand_seed || 0 == rand_seed_size)
		return -1;

	memset(rand_seed, 0, rand_seed_size);

	GetSystemTime(&systime);

	memcpy(rand_seed, &systime, rand_seed_size > sizeof(systime) ? sizeof(systime) : rand_seed_size);

	return 0;
#else
	int pid, fd;
	fd = open("/dev/urandom", O_RDONLY);
	if( fd<0 )
	{
		time_t t;
		time(&t);
		memcpy(rand_seed, &t, rand_seed_size > sizeof(t) ? sizeof(t) : rand_seed_size);
		pid = getpid();
		memcpy((char *)rand_seed + sizeof(t), &pid, sizeof(pid));
	}
	else
	{
		read(fd, rand_seed, rand_seed_size);
		close(fd);
	}

	return 0;
#endif
}













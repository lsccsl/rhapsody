/**
 * @file lcx_record2file.h wrapper os audio input read 2006-12-27 23:26
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
#include "lcx_record2file.h"

#include <windows.h>

#include "myOsFile.h"


typedef struct __wave_file_t_
{
	HMYOSFILE hfile;

	DWORD wv_sz;
}wave_file_t;


/* riff头定义 */
struct RIFF_HEADER
{
	char szRiffID[4];   // 'R','I','F','F'
	DWORD dwRiffSize;
	char szRiffFormat[4]; // 'W','A','V','E'
	char fmt[4];
	DWORD sz;
};

/* 波形相关的属性 */
struct WAVE_FORMAT
{
	WORD wFormatTag;
	WORD wChannels;
	DWORD dwSamplesPerSec;
	DWORD dwAvgBytesPerSec;
	WORD wBlockAlign;
	WORD wBitsPerSample;
};

/* 数据域 */
struct DATA_BLOCK
{
	char szDataID[4]; // 'd','a','t','a'
	DWORD dwDataSize;
};


/**
 * @brief 打开文件
 */
HWAVEFILE lcx_wave_file_open(const char * filename, unsigned int sampleRate, unsigned int bitWidth, unsigned int channels)
{
	/* 写wave文件头 */
	/* 先写RIFF */
	/* 预留file size */
	/* 再写WAVE与'fmt ' */

	wave_file_t * wvf = NULL;

	struct RIFF_HEADER rifffmt = {
		"RIFF", 0, "WAVE", "fmt ", 16,
	};

	struct WAVE_FORMAT wvfmt = {
		1,
	};

	struct DATA_BLOCK data_blk = {
		"data",
	};

	wvfmt.wChannels = channels;
	wvfmt.dwSamplesPerSec = sampleRate;
	wvfmt.dwAvgBytesPerSec = sampleRate * (((bitWidth + 7) / 8) * channels);
	wvfmt.wBlockAlign = ((bitWidth + 7) / 8) * channels;
	wvfmt.wBitsPerSample = bitWidth;

	wvf = MyMemPoolMalloc(NULL, sizeof(*wvf));
	if(NULL == wvf)
		return NULL;

	wvf->hfile = myOsFileOpenReadWrite(filename, NULL);
	if(NULL == wvf->hfile)
		goto lcx_wave_file_open_err_;

	wvf->wv_sz = 0;

	myOsFileWrite(wvf->hfile, &rifffmt, sizeof(rifffmt), NULL);
	myOsFileWrite(wvf->hfile, &wvfmt, sizeof(wvfmt), NULL);
	myOsFileWrite(wvf->hfile, &data_blk, sizeof(data_blk), NULL);

	return wvf;

lcx_wave_file_open_err_:

	if(wvf && wvf->hfile)
		myOsFileClose(wvf->hfile);

	if(wvf)
		MyMemPoolFree(NULL, wvf);

	return NULL;
}

/**
 * @brief 关闭文件
 */
void lcx_wave_file_close(HWAVEFILE hwf)
{
	DWORD riff_sz = 0;

	if(NULL == hwf || NULL == hwf->hfile)
		return;

	/* 写入wave data 大小 */
	myOsFileSeek(hwf->hfile, sizeof(struct RIFF_HEADER) + sizeof(struct WAVE_FORMAT) + 
		(int64)&(((struct DATA_BLOCK *)NULL)->dwDataSize));
	myOsFileWrite(hwf->hfile, &hwf->wv_sz, sizeof(hwf->wv_sz), NULL);

	/* 写入整个文件大小 */
	riff_sz = hwf->wv_sz + 36;
	myOsFileSeek(hwf->hfile, (int64)&(((struct RIFF_HEADER *)NULL)->dwRiffSize));
	myOsFileWrite(hwf->hfile, &riff_sz, sizeof(riff_sz), NULL);

	myOsFileSyn(hwf->hfile);

	myOsFileClose(hwf->hfile);

	MyMemPoolFree(NULL,  hwf);
}

/**
 * @brief 关闭文件
 */
int lcx_wave_file_append_data(HWAVEFILE hwf, const void * data, unsigned int data_sz)
{
	DWORD riff_sz = 0;
	int64 file_sz;

	if(NULL == hwf || NULL == data || 0 == data_sz || NULL == hwf->hfile)
		return -1;

	myOsFileSize(hwf->hfile, &file_sz);
	myOsFileSeek(hwf->hfile, file_sz);

	myOsFileWrite(hwf->hfile, data, data_sz, NULL);

	hwf->wv_sz += data_sz;

	/* 写入wave data 大小 */
	myOsFileSeek(hwf->hfile, sizeof(struct RIFF_HEADER) + sizeof(struct WAVE_FORMAT) + 
		(int64)&(((struct DATA_BLOCK *)NULL)->dwDataSize));
	myOsFileWrite(hwf->hfile, &hwf->wv_sz, sizeof(hwf->wv_sz), NULL);

	/* 写入整个文件大小 */
	riff_sz = hwf->wv_sz + 36;
	myOsFileSeek(hwf->hfile, (int64)&(((struct RIFF_HEADER *)NULL)->dwRiffSize));
	myOsFileWrite(hwf->hfile, &riff_sz, sizeof(riff_sz), NULL);

	myOsFileSyn(hwf->hfile);

	return 0;
}




















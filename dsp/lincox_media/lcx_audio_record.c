/**
 * @file lcx_audio_record.c wrapper os audio input read 2006-12-27 23:26
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
#include "lcx_audio_record.h"

/* for waveXXX function extern */
#include <windows.h>
#include <Mmsystem.h>


typedef struct __lcx_audio_record_t_
{
	/* the handle for mic device, the src of voice data input */
	HWAVEIN hWaveIn;
	/* the buffer for voice data input */
	WAVEHDR * voice_data_in;
	/* the size of voice_data_in array */
	size_t voice_data_in_sz;
	/* current read index */
	size_t read_idx;

	/* wave format */
	WAVEFORMATEX wvfmt;

	/* call back event handle */
	HANDLE hevent_cb;
}lcx_audio_record_t;


/**
 * @brief audio input object construct
 */
HLCXAUDIO_RECORD lcx_audio_record_construct()
{
	lcx_audio_record_t * a = malloc(sizeof(*a));
	if(NULL == a)
		return NULL;

	memset(a, 0 ,sizeof(*a));

	return a;
}

/**
 * @brief audio input object destruct
 */
void lcx_audio_record_destruct(HLCXAUDIO_RECORD hlcxAudio)
{
	//todo free all mem here.
}

/**
 * @brief set audio input param,such as sample rate, frame size,and bit width, and device name(no support this version).
 * @param sampleRate:sample rate, if 0 the default value is 8000
 * @param bitWidth:if 0 the default value is 16,
 * @param bufSize:the size for buffer for read and write
 * @param bufCount:the buffer count. 
 *				the windows waveXXX api is asyn, we prepare a buf queue for the read and write, 
 *				to avoid the read and write delay.
 *				the default value is 30
 * @param device_name:the sound card device name, no support this version
 */
int lcx_audio_record_init(HLCXAUDIO_RECORD hlcxAudio,
						  unsigned int sampleRate,
						  unsigned int bitWidth,
						  unsigned int bufSize,
						  unsigned int bufCount,
						  const char * device_name)
{
	MMRESULT ret = MMSYSERR_NOERROR;

	if(NULL == hlcxAudio)
		return -1;
	
	/* has inited */
	if(hlcxAudio->hWaveIn)
		return -1;

	hlcxAudio->wvfmt.wFormatTag = WAVE_FORMAT_PCM;
	hlcxAudio->wvfmt.nChannels = 1;
	hlcxAudio->wvfmt.wBitsPerSample = bitWidth;
	hlcxAudio->wvfmt.nSamplesPerSec = sampleRate;
	hlcxAudio->wvfmt.nBlockAlign = (WORD)(hlcxAudio->wvfmt.nChannels *(bitWidth + 7) / 8);
	hlcxAudio->wvfmt.nAvgBytesPerSec = hlcxAudio->wvfmt.nSamplesPerSec * hlcxAudio->wvfmt.nBlockAlign;
	hlcxAudio->wvfmt.cbSize = 0;

	hlcxAudio->hevent_cb = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(INVALID_HANDLE_VALUE == hlcxAudio->hevent_cb)
		return -1;

	ret = waveInOpen(&hlcxAudio->hWaveIn, WAVE_MAPPER, &hlcxAudio->wvfmt, (DWORD)hlcxAudio->hevent_cb, 0, CALLBACK_EVENT);
	if(MMSYSERR_NOERROR != ret)
		goto lcx_audio_record_init_err_;

	/* malloc buffer for recording */
	hlcxAudio->voice_data_in_sz = bufCount;
	hlcxAudio->voice_data_in = malloc(sizeof(hlcxAudio->voice_data_in[0]) * bufCount);
	if(NULL == hlcxAudio->voice_data_in)
		goto lcx_audio_record_init_err_;

	memset(hlcxAudio->voice_data_in, 0, sizeof(hlcxAudio->voice_data_in[0]) * bufCount);

	{
		unsigned int i = 0;
		for(; i < bufCount; i ++)
		{
			hlcxAudio->voice_data_in[i].lpData = malloc(bufSize);
			if(NULL == hlcxAudio->voice_data_in[i].lpData)
				goto lcx_audio_record_init_err_;

			hlcxAudio->voice_data_in[i].dwBufferLength = bufSize;
			hlcxAudio->voice_data_in[i].dwFlags = WHDR_DONE;
		}
	}

	/* start recording */
	ret = waveInStart(hlcxAudio->hWaveIn);
	if(MMSYSERR_NOERROR != ret)
		goto lcx_audio_record_init_err_;

	{
		/* begin record,add all buffer to recording queue */
		unsigned int i = 0;
		for(; i < bufCount; i ++)
		{
			ret = waveInPrepareHeader(hlcxAudio->hWaveIn,
				&hlcxAudio->voice_data_in[i],
				sizeof(hlcxAudio->voice_data_in[i]));
			if(MMSYSERR_NOERROR != ret)
				goto lcx_audio_record_init_err_;

			ret = waveInAddBuffer(hlcxAudio->hWaveIn,
				&hlcxAudio->voice_data_in[i],
				sizeof(hlcxAudio->voice_data_in[i]));
			if(MMSYSERR_NOERROR != ret)
				goto lcx_audio_record_init_err_;
		}

		hlcxAudio->read_idx = 0;
	}

	return 0;

lcx_audio_record_init_err_:

	/* free all mem here */
	//todo ...

	return -1;
}

/**
 * @brief read from audio input
 * @retval >0:bytes count success readed,  -1:fail
 */
int lcx_audio_record_read(HLCXAUDIO_RECORD hlcxAudio, void * buf, size_t buf_sz)
{
	if(NULL == hlcxAudio || NULL == buf || 0 == buf_sz)
		return -1;

	/* if no record data is ok, block here */
	while(!(hlcxAudio->voice_data_in[hlcxAudio->read_idx].dwFlags & WHDR_DONE) ||
		!(hlcxAudio->voice_data_in[hlcxAudio->read_idx].dwBytesRecorded))
	{
		if(WaitForSingleObject(hlcxAudio->hevent_cb, INFINITE) != WAIT_OBJECT_0)
			return -1;
	}

	/* get data form record queue */
	memcpy(buf, hlcxAudio->voice_data_in[hlcxAudio->read_idx].lpData, buf_sz);

	/* 
	* add buf to recording queue again 
	* what about if we fail here?
	*/
	waveInUnprepareHeader(hlcxAudio->hWaveIn,
		&hlcxAudio->voice_data_in[hlcxAudio->read_idx],
		sizeof(hlcxAudio->voice_data_in[hlcxAudio->read_idx]));
	waveInPrepareHeader(hlcxAudio->hWaveIn,
		&hlcxAudio->voice_data_in[hlcxAudio->read_idx],
		sizeof(hlcxAudio->voice_data_in[hlcxAudio->read_idx]));
	waveInAddBuffer(hlcxAudio->hWaveIn,
		&hlcxAudio->voice_data_in[hlcxAudio->read_idx],
		sizeof(hlcxAudio->voice_data_in[hlcxAudio->read_idx]));

	/* goto next read buf index */
	hlcxAudio->read_idx = (hlcxAudio->read_idx + 1) % hlcxAudio->voice_data_in_sz;

	return (hlcxAudio->voice_data_in[hlcxAudio->read_idx].dwBytesRecorded < buf_sz) ?
		hlcxAudio->voice_data_in[hlcxAudio->read_idx].dwBytesRecorded :
		buf_sz;
}















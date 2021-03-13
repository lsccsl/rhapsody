/**
 * @file lcx_audio_play.c wrapper os audio output write 2006-12-27 23:26
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
#include "lcx_audio_play.h"

/* for waveXXX function extern */
#include <windows.h>
#include <Mmsystem.h>


typedef struct __lcx_audio_play_t_
{
	/* the handle for mic device, the src of voice data input */
	HWAVEOUT hWaveOut;
	/* the buffer for voice data input */
	WAVEHDR * voice_data_out;
	/* the size of voice_data_in array */
	size_t voice_data_out_sz;
	/* the lpdata's size */
	size_t voice_data_frame_sz;
	/* current read index */
	size_t write_idx;

	/* wave format */
	WAVEFORMATEX wvfmt;

	/* call back event handle */
	HANDLE hevent_cb;
}lcx_audio_play_t;


/**
 * @brief audio output object construct
 */
HLCXAUDIO_PLAY lcx_audio_play_construct()
{
	lcx_audio_play_t * a = malloc(sizeof(*a));
	if(NULL == a)
		return NULL;

	memset(a, 0, sizeof(*a));

	return a;
}

/**
 * @brief audio output object destruct
 */
void DLLEXPORT lcx_audio_play_destruct(HLCXAUDIO_PLAY hlcxAudio)
{
	//todo free all mem here
}

/**
 * @brief set audio output param,such as sample rate, frame size,and bit width, and device name(no support this version).
 * @param sampleRate:sample rate, if 0 the default value is 8000
 * @param bitWidth:if 0 the default value is 16,
 * @param bufSize:the size for buffer for read and write
 * @param bufCount:the buffer count. 
 *				the windows waveXXX api is asyn, we prepare a buf queue for the read and write, 
 *				to avoid the read and write delay.
 *				the default value is 30
 * @param device_name:the sound card device name, no support this version
 */
int lcx_audio_play_init(HLCXAUDIO_PLAY hlcxAudio,
						unsigned int sampleRate,
						unsigned int bitWidth,
						unsigned int bufSize,
						unsigned int bufCount,
						const char * device_name)
{
	MMRESULT ret = MMSYSERR_NOERROR;

	if(NULL == hlcxAudio)
		return -1;

	if(hlcxAudio->hWaveOut)
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

	/* open wave out handle */
	ret = waveOutOpen(&hlcxAudio->hWaveOut, WAVE_MAPPER, &hlcxAudio->wvfmt, (DWORD)hlcxAudio->hevent_cb, 0, CALLBACK_EVENT);
	if(MMSYSERR_NOERROR != ret)
		goto lcx_audio_play_init_err_;

	/* alloc write buf */
	hlcxAudio->voice_data_out_sz = bufCount;
	hlcxAudio->voice_data_out = malloc(sizeof(hlcxAudio->voice_data_out[0]) * bufCount);
	if(NULL == hlcxAudio->voice_data_out)
		goto lcx_audio_play_init_err_;

	memset(hlcxAudio->voice_data_out, 0, sizeof(hlcxAudio->voice_data_out[0]) * bufCount);

	{
		unsigned int i = 0;

		hlcxAudio->voice_data_frame_sz = bufSize;
		
		for(; i < bufCount; i ++)
		{
			hlcxAudio->voice_data_out[i].lpData = malloc(bufSize);
			hlcxAudio->voice_data_out[i].dwFlags = WHDR_DONE;
			if(NULL == hlcxAudio->voice_data_out[i].lpData)
				goto lcx_audio_play_init_err_;

			hlcxAudio->voice_data_out[i].dwBufferLength = bufSize;
		}
	}

	hlcxAudio->write_idx = 0;

	return 0;

lcx_audio_play_init_err_:

	/* free all mem here */
	//todo ...

	return -1;
}

/**
 * @brief write to audio output
 * @retval >0:bytes count success readed,  -1:fail
 */
int lcx_audio_play_write(HLCXAUDIO_PLAY hlcxAudio, void * buf, size_t buf_sz)
{
	if(NULL == hlcxAudio || NULL == buf || 0 == buf_sz)
		return -1;

	/* if all buf is in the write queue, block here */
	while(!(hlcxAudio->voice_data_out[hlcxAudio->write_idx].dwFlags & WHDR_DONE))
	{
		if(WaitForSingleObject(hlcxAudio->hevent_cb, INFINITE) != WAIT_OBJECT_0)
			return -1;
	}

	/* copy user data,and put the buf into write queue */
	waveOutUnprepareHeader(hlcxAudio->hWaveOut,
		&hlcxAudio->voice_data_out[hlcxAudio->write_idx],
		sizeof(hlcxAudio->voice_data_out[hlcxAudio->write_idx]));
	hlcxAudio->voice_data_out[hlcxAudio->write_idx].dwBufferLength = 
		(hlcxAudio->voice_data_frame_sz > buf_sz) ? buf_sz : hlcxAudio->voice_data_frame_sz;
	waveOutPrepareHeader(hlcxAudio->hWaveOut,
		&hlcxAudio->voice_data_out[hlcxAudio->write_idx],
		sizeof(hlcxAudio->voice_data_out[hlcxAudio->write_idx]));
	memcpy(hlcxAudio->voice_data_out[hlcxAudio->write_idx].lpData,
		buf,
		hlcxAudio->voice_data_out[hlcxAudio->write_idx].dwBufferLength);
	waveOutWrite(hlcxAudio->hWaveOut,
		&hlcxAudio->voice_data_out[hlcxAudio->write_idx],
		sizeof(hlcxAudio->voice_data_out[hlcxAudio->write_idx]));

	/* goto next buf */
	hlcxAudio->write_idx = (hlcxAudio->write_idx + 1) % hlcxAudio->voice_data_out_sz;

	return hlcxAudio->voice_data_out[hlcxAudio->write_idx].dwBufferLength;
}



















/**
 * @file lcx_audio_play.h wrapper os audio output write 2006-12-27 23:26
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
#ifndef __LCX_AUDIO_PLAY_H__
#define __LCX_AUDIO_PLAY_H__

#include <stdlib.h>

/* for global marco define */
#include "lin_config.h"


/* audio output handle define */
struct __lcx_audio_play_t_;
typedef struct __lcx_audio_play_t_ * HLCXAUDIO_PLAY;

/**
 * @brief audio output object construct
 */
extern HLCXAUDIO_PLAY DLLEXPORT lcx_audio_play_construct();

/**
 * @brief audio output object destruct
 */
extern void DLLEXPORT lcx_audio_play_destruct(HLCXAUDIO_PLAY hlcxAudio);

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
extern int DLLEXPORT lcx_audio_play_init(HLCXAUDIO_PLAY hlcxAudio,
										 unsigned int sampleRate,
										 unsigned int bitWidth,
										 unsigned int bufSize,
										 unsigned int bufCount,
										 const char * device_name);

/**
 * @brief write to audio output
 * @retval >0:bytes count success readed,  -1:fail
 */
extern int DLLEXPORT lcx_audio_play_write(HLCXAUDIO_PLAY hlcxAudio, void * buf, size_t buf_sz);


#endif




















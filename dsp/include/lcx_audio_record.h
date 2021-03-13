/**
 * @file lcx_audio_record.h wrapper os audio input read 2006-12-27 23:26
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
#ifndef __LCX_AUDIO_RECORD_H__
#define __LCX_AUDIO_RECORD_H__

#include <stdlib.h>

/* for global marco define */
#include "lin_config.h"


/* audio input handle define */
struct __lcx_audio_record_t_;
typedef struct __lcx_audio_record_t_ * HLCXAUDIO_RECORD;

/**
 * @brief audio input object construct
 */
extern HLCXAUDIO_RECORD DLLEXPORT lcx_audio_record_construct();

/**
 * @brief audio input object destruct
 */
extern void DLLEXPORT lcx_audio_record_destruct(HLCXAUDIO_RECORD hlcxAudio);

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
extern int DLLEXPORT lcx_audio_record_init(HLCXAUDIO_RECORD hlcxAudio,
										   unsigned int sampleRate,
										   unsigned int bitWidth,
										   unsigned int bufSize,
										   unsigned int bufCount,
										   const char * device_name);

/**
 * @brief read from audio input
 * @retval >0:bytes count success readed,  -1:fail
 */
extern int DLLEXPORT lcx_audio_record_read(HLCXAUDIO_RECORD hlcxAudio, void * buf, size_t buf_sz);


#endif











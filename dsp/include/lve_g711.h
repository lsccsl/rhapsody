/**
 * @file lve_g711.h g711 code/decode 2006-12-27 23:26
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
#ifndef __LINVE_G711_H__
#define __LINVE_G711_H__

/* for global marco define */
#include "lin_config.h"


/**
 * @brief g711 encode
 */
extern DLLEXPORT int lve_g711_line2ulaw(int frame);

/**
 * @brief g711 decode
 */
extern DLLEXPORT int lve_g711_ulaw2line(int frame);


#endif


















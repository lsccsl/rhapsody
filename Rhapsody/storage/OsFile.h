/**
 * @file myOsFile.h 封装不同系统的文件操作接口 2008-1-30 00:43
 *
 * @author lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
 *
 * @brief if it works, it was written by lin shao chuan, if not, i don't know who wrote it.
 *        封装不同系统的文件操作接口
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
#ifndef __OSFILE_H__
#define __OSFILE_H__


#ifdef __USER_STDIO__

	#include <stdio.h>

	typedef FILE * HOSFILE;

	#define OsFileOpenReadOnly(file_name, hm) fopen(file_name, "r")
	#define OsFileOpenReadWrite(file_name, hm) fopen(file_name, "ab")
	#define OsFileOpenExclusive(file_name, hm) do{LOG_WARN(("no exclusive for stdio"))}while(0), OsFileOpenReadWrite(file_name, ex)
	#define OsFileClose(hf) fclose(hf)
	#define OsFileSyn(hf) fflush(hf) 
	#define OsFileWrite(hf, data, data_size, write_return) ((*write_return = fwrite(data, 1, data_size, hf)) == data_size ? 0 : -1)
	#define OsFileRead(hf, data, data_size, read_return) ((*read_return = fread(data, 1, data_size, hf)) == data_size ? 0 : -1)
	#define OsFileSeek(hf, off_set) fseek(hf, off_set, SEEK_SET)
	#define OsFileDel(file_name) fopen(file_name, "w"),fclose(file_name);

#else
	
	#include "myOsFile.h"

	typedef HMYOSFILE HOSFILE;

	#define OsFileOpenReadOnly(file_name, hm) myOsFileOpenReadOnly(file_name, hm)
	#define OsFileOpenReadWrite(file_name, hm) myOsFileOpenReadWrite(file_name, hm)
	#define OsFileOpenExclusive(file_name, hm) myOsFileOpenExclusive(file_name, hm)
	#define OsFileClose(hf) myOsFileClose(hf)
	#define OsFileSyn(hf) myOsFileSyn(hf) 
	#define OsFileWrite(hf, data, data_size, write_return) myOsFileWrite(hf, data, data_size, write_return)
	#define OsFileRead(hf, data, data_size, read_return) myOsFileRead(hf, data, data_size, read_return)
	#define OsFileSeek(hf, off_set) myOsFileSeek(hf, off_set)
	#define OsFileDel(file_name) myOsFileDel(file_name)
	#define OsFileSize(hf, psize) myOsFileSize(hf, psize)
	#define OsFileExists(_file_name) myOsFileExists(_file_name)
	#define OsFileTruncate(hf, nByte) myOsFileTruncate(hf, nByte)

#endif

#endif















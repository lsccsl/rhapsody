/*
*
*myini.h 解析ini文件 lin shao chuan
*
*/


#ifndef __MYINI_H__
#define __MYINI_H__


#include "mylist.h"


typedef struct __myini_handle_
{	int unused;	} * HMYINI;

typedef struct __myini_key_handle_
{	int unused;	} * HMYINI_KEY;

typedef struct __myini_sec_handle_
{	int unused;	} * HMYINI_SEC;


/*
*
*从字符串中导入ini文件
*
*/
extern HMYINI MyIniConstructFromChunk(const char * content, size_t content_len);

/*
*
*从文件中导入
*
*/
extern HMYINI MyIniConstructFromFile(const char * filepath);

/*
*
*销毁ini
*
*/
extern void MyIniDestruct(HMYINI hini);

/*
*
*获取所有的sec
*
*/
extern HMYLIST MyIniGetSecList(HMYINI hini);

/*
*
*获取某个sec
*
*/
extern HMYINI_SEC MyIniGetSec(HMYINI hini, const char * secname);

/*
*
*获取sec的名字 
*
*/
extern size_t MyIniGetSecName(HMYINI_SEC hsec, char * name, size_t name_len);

/*
*
*获取某个sec的所有key子节点
*
*/
extern HMYLIST MyIniGetKeyList(HMYINI_SEC hsec);

/*
*
*获取某个sec的某个key子节点
*
*/
extern HMYINI_KEY MyIniGetKey(HMYINI_SEC hsec, const char * keyname);

/*
*
*获取key的val
*
*/
extern size_t MyIniGetKeyValString(HMYINI_KEY hkey, char * val, size_t val_len);

/*
*
*获取key的name
*
*/
extern size_t MyIniGetKeyName(HMYINI_KEY hkey, char * name, size_t name_len);

/*
*
*读取某个键
*
*/
extern int MyIniReadKeyString(HMYINI hini, const char * sec, const char * key, char * val, size_t * val_size);

/*
*
*写某个键
*
*/
extern int MyIniWriteKeyString(HMYINI hini, const char * sec, const char * key, const char * val,size_t val_size);

/*
*
*删除某个sec
*
*/
extern int MyIniDelSec(HMYINI hini, const char * sec);

/*
*
*删除某个key
*
*/
extern int MyIniDelKey(HMYINI hini, const char * sec, const char * key);

/*
*
*写回外存
*
*/
extern int MyIniWriteBack(HMYINI hini);


#endif




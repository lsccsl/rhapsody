/*
*
*myini.h ����ini�ļ� lin shao chuan
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
*���ַ����е���ini�ļ�
*
*/
extern HMYINI MyIniConstructFromChunk(const char * content, size_t content_len);

/*
*
*���ļ��е���
*
*/
extern HMYINI MyIniConstructFromFile(const char * filepath);

/*
*
*����ini
*
*/
extern void MyIniDestruct(HMYINI hini);

/*
*
*��ȡ���е�sec
*
*/
extern HMYLIST MyIniGetSecList(HMYINI hini);

/*
*
*��ȡĳ��sec
*
*/
extern HMYINI_SEC MyIniGetSec(HMYINI hini, const char * secname);

/*
*
*��ȡsec������ 
*
*/
extern size_t MyIniGetSecName(HMYINI_SEC hsec, char * name, size_t name_len);

/*
*
*��ȡĳ��sec������key�ӽڵ�
*
*/
extern HMYLIST MyIniGetKeyList(HMYINI_SEC hsec);

/*
*
*��ȡĳ��sec��ĳ��key�ӽڵ�
*
*/
extern HMYINI_KEY MyIniGetKey(HMYINI_SEC hsec, const char * keyname);

/*
*
*��ȡkey��val
*
*/
extern size_t MyIniGetKeyValString(HMYINI_KEY hkey, char * val, size_t val_len);

/*
*
*��ȡkey��name
*
*/
extern size_t MyIniGetKeyName(HMYINI_KEY hkey, char * name, size_t name_len);

/*
*
*��ȡĳ����
*
*/
extern int MyIniReadKeyString(HMYINI hini, const char * sec, const char * key, char * val, size_t * val_size);

/*
*
*дĳ����
*
*/
extern int MyIniWriteKeyString(HMYINI hini, const char * sec, const char * key, const char * val,size_t val_size);

/*
*
*ɾ��ĳ��sec
*
*/
extern int MyIniDelSec(HMYINI hini, const char * sec);

/*
*
*ɾ��ĳ��key
*
*/
extern int MyIniDelKey(HMYINI hini, const char * sec, const char * key);

/*
*
*д�����
*
*/
extern int MyIniWriteBack(HMYINI hini);


#endif




/*
*
*myini.c 解析ini文件 lin shao chuan
*
*/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "myini.h"


typedef struct __mykey_t_
{
	char * name;
	size_t name_len;

	void * val;
	size_t val_len;
}mykey_t;

typedef struct __mysec_t_
{
	char * name;
	size_t name_len;

	HMYLIST keys;
}mysec_t;

typedef struct __myini_t_
{
	HMYLIST secs;
}myini_t;


/*
*
*字符串右截空函数
*
*/
char * StrRTrim(char *_pcStr)
{
	if(_pcStr != NULL)
	{
		size_t iLen = strlen(_pcStr);

		while(iLen != 0)
		{
			if ((' ' == _pcStr[iLen-1]) || ('\t' == _pcStr[iLen-1])||
				('\n' == _pcStr[iLen-1])|| ('\r' == _pcStr[iLen-1]))
			{
				_pcStr[iLen-1]='\0';
				iLen--;
			}
			else 
				break;
		}
	}

	return _pcStr;
}

/*
*
*字符串左截空函数
*
*/
char * StrLTrim(char *_pcStr)
{
	if(_pcStr != NULL)
	{
		size_t iLen = strlen(_pcStr);
		
		while(iLen != 0)
		{
			if((' ' == _pcStr[0]) || ('\t'== _pcStr[0]))
			{	
				memmove(_pcStr, _pcStr + 1, strlen(_pcStr +1) + 1);
				//strcpy(_pcStr, _pcStr + 1);
				iLen--;
			}
			else
				break;
		}
	}
	return _pcStr;
}

/*
*
*字符串左右截空函数
*
*/
char * StrLRTrim(char *_pcStr)
{
	StrRTrim(_pcStr);
	return StrLTrim(_pcStr);
}

/*
*
*销毁key
*
*/
static __INLINE__ void destroyKey(mykey_t * key)
{
	if(NULL == key)
		return;

	if(key->name)
		MyMemPoolFree(NULL, key->name);
	key->name = NULL;
	key->name_len = 0;

	if(key->val)
		MyMemPoolFree(NULL, key->val);
	key->val = NULL;
	key->val_len = 0;

	MyMemPoolFree(NULL, key);
}

/*
*
*创建key
*
*/
static __INLINE__ mykey_t * createKey(const char * name, size_t name_len, void * val, size_t val_len)
{
	mykey_t * key = NULL;

	assert(name && val);

	key = (mykey_t *)MyMemPoolMalloc(NULL, sizeof(*key));
	if(NULL == key)
		goto createKey_err_;
	memset(key, 0, sizeof(*key));

	key->name = (char *)MyMemPoolMalloc(NULL, name_len + 1);
	if(NULL == key->name)
		goto createKey_err_;
	strncpy(key->name, name, name_len);
	key->name_len = name_len;
	key->name[key->name_len] = 0;

	key->val = MyMemPoolMalloc(NULL, val_len + 1);
	if(NULL == key->val)
		goto createKey_err_;
	memset(key->val, 0, val_len + 1);
	memcpy(key->val, val, val_len);
	key->val_len = val_len;

	return key;

createKey_err_:

	destroyKey(key);

	return NULL;
}

/*
*
*销毁sec
*
*/
static __INLINE__ void destroySec(mysec_t * sec)
{
	if(NULL == sec)
		return;

	if(sec->keys)
	{
		HMYLIST_ITER iter = MyListGetHead(sec->keys);
		HMYLIST_ITER end = MyListGetTail(sec->keys);

		for(; iter != end; iter = MyListGetNext(sec->keys, iter))
			destroyKey((mykey_t *)MyListGetIterData(iter));

		MyListDestruct(sec->keys);
	}
	sec->keys = NULL;

	if(sec->name)
		MyMemPoolFree(NULL, sec->name);
	sec->name = NULL;
	
	sec->name_len = 0;

	MyMemPoolFree(NULL, sec);
}

/*
*
*创建sec
*
*/
static __INLINE__ mysec_t * createSec(const char * sec_name, size_t name_len)
{
	mysec_t * sec = NULL;

	assert(sec_name);

	sec = (mysec_t *)MyMemPoolMalloc(NULL, sizeof(*sec));
	if(NULL == sec)
		return NULL;
	memset(sec, 0, sizeof(*sec));

	sec->keys = MyListConstruct(NULL);
	if(NULL == sec->keys)
		goto createSec_err_;

	sec->name = (char *)MyMemPoolMalloc(NULL, name_len + 1);
	if(NULL == sec->name)
		goto createSec_err_;

	sec->name_len = name_len;
	strncpy(sec->name, sec_name, name_len);
	sec->name[name_len] = 0;

	return sec;

createSec_err_:
	
	destroySec(sec);

	return NULL;
}

static __INLINE__ void destroyIni(myini_t * ini)
{
	HMYLIST_ITER iter = NULL;
	HMYLIST_ITER end = NULL;

	if(NULL == ini)
		return;

	if(NULL == ini->secs)
	{
		MyMemPoolFree(NULL, ini);
		return;
	}

	for(iter = MyListGetHead(ini->secs); iter != MyListGetTail(ini->secs); iter = MyListGetNext(ini->secs, iter))
		destroySec((mysec_t *)MyListGetIterData(iter));

	MyListDestruct(ini->secs);
	ini->secs = NULL;

	MyMemPoolFree(NULL, ini);
}

/*
*
*创建ini
*
*/
static __INLINE__ myini_t * CreateIni()
{
	myini_t * ini = (myini_t *)MyMemPoolMalloc(NULL, sizeof(myini_t));
	if(NULL == ini)
		goto iniCreate_err_;

	ini->secs = MyListConstruct(NULL);
	if(NULL == ini->secs)
		goto iniCreate_err_;

	return ini;

iniCreate_err_:

	destroyIni(ini);

	return NULL;
}

/*
*
*添加一个sec至ini
*
*/
static __INLINE__ mysec_t * IniAddSec(myini_t * ini, const char * sec_name, size_t sec_name_len)
{
	mysec_t * sec = NULL;

	assert(ini && sec_name);

	sec = createSec(sec_name, sec_name_len);
	if(NULL == sec)
		return NULL;

	MyListAddTail(ini->secs, sec);

	return sec;
}

/*
*
*添加一个key至sec
*
*/
static __INLINE__ mykey_t * SecAddKey(mysec_t * sec, const char * name, size_t name_len, void * val, size_t val_len)
{
	mykey_t * key = NULL;

	assert(sec && sec->keys);

	key = createKey(name, name_len, val, val_len);
	if(NULL == key)
		return NULL;

	MyListAddTail(sec->keys, key);

	return key;
}

/*
*
*导入ini文件
*
*/
static __INLINE__ myini_t * IniConstruct(const char * content, size_t content_len)
{
	mysec_t * sec = NULL;
	mykey_t * key = NULL;
	myini_t * ini =	CreateIni();

	char * pcbegin = (char *)content;
	char * pcEnd   = (char *)content + content_len - 1;
	char * pcline  = NULL;
	//char * pcComment = NULL;

	printf("[%s:%d] IniConstruct Begin\n", __FILE__, __LINE__);

	assert(content && content_len);

	while(pcbegin < pcEnd)
	{
		pcline = strchr(pcbegin, '\n');
		if(pcline)
		{
			*pcline = 0;
		}
		
		if( (*pcbegin) == '[')/// 如果是sect
		{
			char * pcSecEnd = strchr(pcbegin, ']');

			if(pcSecEnd)
			{
				*pcSecEnd = 0;

				sec = IniAddSec(ini, pcbegin + 1, strlen(pcbegin + 1));

				*pcSecEnd = ']';

				if(NULL == sec)
					goto MyIniConstruct_err_;                
			}
		}
		else/// 如果是item
		{
			char * p_item_name_end = strchr(pcbegin, '=');
			if(p_item_name_end)
			{
				if(NULL == sec)
				{
					sec = IniAddSec(ini, "", 0);

					if(NULL == sec)
						goto MyIniConstruct_err_;
				}

				*p_item_name_end = 0;

				key = SecAddKey(sec, pcbegin, strlen(pcbegin), p_item_name_end + 1, strlen(p_item_name_end + 1));

				*p_item_name_end = '=';

				if(NULL == key)
					goto MyIniConstruct_err_;
			}
		}

		if(pcline)
		{
			*pcline = '\n';
			pcbegin = pcline + 1;
		}
		else
		{
			pcbegin = pcEnd;
		}
	}

	printf("[%s:%d] IniConstruct end\n", __FILE__, __LINE__);

	return ini;

MyIniConstruct_err_:

	destroyIni(ini);

	return NULL;
}

/*
*
*销毁ini
*
*/
extern void MyIniDestruct(HMYINI hini);

/*
*
*从字符串中导入ini文件
*
*/
HMYINI MyIniConstructFromChunk(const char * content, size_t content_len)
{
	if(content && content_len)
		return (HMYINI)IniConstruct(content, content_len);

	return NULL;
}

/*
*
*从文件中导入
*
*/
HMYINI MyIniConstructFromFile(const char * filepath)
{
#define CONTENT_LEN 512
#define LINE_LEN 128

	char * content = NULL;
	char * pcbuf = NULL;
	size_t content_len = CONTENT_LEN;
	FILE * p = NULL;
	HMYINI hini = NULL;

	if(NULL == filepath)
		return NULL;

	p = fopen(filepath, "rt");
	if(NULL == p)
		return NULL;

	printf("[%s:%d] MyIniConstructFromFile Begin\n", __FILE__, __LINE__);

	content = (char *)MyMemPoolMalloc(NULL, content_len);
	if(NULL == content)
		goto MyIniConstructFromFile_end_;
	memset(content, 0, content_len);//


	pcbuf = NULL;

	do
	{
		char buf[LINE_LEN] = {0};
		char * pcPosEnd = NULL;

		pcbuf = fgets(buf, sizeof(buf), p);

		pcPosEnd=strstr(buf,"##");
		if (NULL!=pcPosEnd)
			*pcPosEnd=0;

		StrLRTrim(buf);

		buf[strlen(buf)] = '\n';

		if(content_len - strlen(content) <= strlen(buf))
		{
			char * ptemp = (char *)MyMemPoolMalloc(NULL, content_len * 2);
			if(NULL == ptemp)
				goto MyIniConstructFromFile_end_;
			memset(ptemp, 0, content_len);//

			memcpy(ptemp, content, strlen(content));

			MyMemPoolFree(NULL, content);
			content_len = content_len * 2;
			content = ptemp;
		}

		strcpy(content + strlen(content), buf);
	}while(pcbuf);

	hini = (HMYINI)IniConstruct(content, content_len);

MyIniConstructFromFile_end_:

	if(p)
		fclose(p);

	if(content)
		MyMemPoolFree(NULL, content);

	printf("[%s:%d] MyIniConstructFromFile end\n", __FILE__, __LINE__);

	return hini;
}

/*
*
*销毁ini
*
*/
void MyIniDestruct(HMYINI hini)
{
	if(NULL == hini)
		return;

	destroyIni((myini_t *)hini);
}

/*
*
*获取所有的sec
*
*/
HMYLIST MyIniGetSecList(HMYINI hini)
{
	myini_t * ini = (myini_t *)hini;

	if(NULL == ini)
		return NULL;

	return ini->secs;
}

/*
*
*获取某个sec
*
*/
HMYINI_SEC MyIniGetSec(HMYINI hini, const char * secname)
{
	HMYLIST_ITER it = NULL;
	HMYLIST_ITER end = NULL;

	myini_t * ini = (myini_t *)hini;
	if(NULL == ini || NULL == secname)
		return NULL;

	end = MyListGetTail(ini->secs);

	for(it = MyListGetHead(ini->secs); it != end; it = MyListGetNext(ini->secs, it))
	{
		mysec_t * sec = (mysec_t *)MyListGetIterData(it);
		if(NULL == sec)
			continue;

		if(strcmp(secname, sec->name) == 0)
			return (HMYINI_SEC)sec;
	}

	return NULL;
}

/*
*
*获取sec的名字 
*
*/
size_t MyIniGetSecName(HMYINI_SEC hsec, char * name, size_t name_len)
{
	mysec_t * sec = (mysec_t *)hsec;
	if(NULL == sec || NULL == name || 0 == name_len)
		return 0;

	strncpy(name, sec->name, name_len);
	return sec->name_len;
}

/*
*
*获取某个sec的所有key子节点
*
*/
HMYLIST MyIniGetKeyList(HMYINI_SEC hsec)
{
	mysec_t * sec = (mysec_t *)hsec;
	if(NULL == sec)
		return NULL;

	return sec->keys;
}

/*
*
*获取某个sec的某个key子节点
*
*/
HMYINI_KEY MyIniGetKey(HMYINI_SEC hsec, const char * keyname)
{
	HMYLIST_ITER it = NULL;
	HMYLIST_ITER end = NULL;

	mysec_t * sec = (mysec_t *)hsec;
	if(NULL == sec || NULL == keyname)
		return NULL;

	end = MyListGetTail(sec->keys);

	for(it = MyListGetHead(sec->keys); it != end; it = MyListGetNext(sec->keys, it))
	{
		mykey_t * key = (mykey_t *)MyListGetIterData(it);
		if(NULL == key)
			continue;

		if(strcmp(keyname, key->name) == 0)
			return (HMYINI_KEY)key;
	}

	return NULL;
}

/*
*
*获取key的val
*
*/
size_t MyIniGetKeyValString(HMYINI_KEY hkey, char * val, size_t val_len)
{
	mykey_t * key = (mykey_t *)hkey;
	if(NULL == key || NULL == val || 0 == val_len)
		return 0;

	strncpy(val, (char *)key->val, val_len);
	return key->val_len;
}

/*
*
*获取key的name
*
*/
size_t MyIniGetKeyName(HMYINI_KEY hkey, char * name, size_t name_len)
{
	mykey_t * key = (mykey_t *)hkey;
	if(NULL == key || NULL == name || 0 == name_len)
		return 0;

	strncpy(name, key->name, name_len);
	return key->name_len;
}

/*
*
*读取某个键
*
*/
int MyIniReadKeyString(HMYINI hini, const char * sec, const char * key, char * val, size_t * val_size)
{
	HMYINI_KEY hkey = NULL;
	HMYINI_SEC hsec = NULL;

	if(NULL == hini || NULL == sec|| NULL == key || NULL == val || NULL == val_size)
		return -1;

	hsec = MyIniGetSec(hini, sec);
	if(NULL == hsec)
		return -1;

	hkey = MyIniGetKey(hsec, key);
	if(NULL == hkey)
		return -1;

	*val_size = MyIniGetKeyValString(hkey, val, *val_size);

	return 0;
}

/*
*
*写某个键
*
*/
int MyIniWriteKeyString(HMYINI hini, const char * sec, const char * key, const char * val,size_t val_size);
//{
//	//HMYINI_KEY hkey = NULL;
//	//HMYINI_SEC hsec = NULL;
//
//	//if(NULL == hini || NULL == sec|| NULL == key || NULL == val || NULL == val_size)
//	//	return -1;
//
//	//hsec = MyIniGetSec(hini, sec);
//	//if(NULL == hsec)
//		return -1;
//
//	//hkey = MyIniGetKey(hsec, key);
//	//if(NULL == hkey)
//	//	return -1;
//
//	return -1;
//}





























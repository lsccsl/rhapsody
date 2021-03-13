/*
*
* myobj.h 对象 
*
* author:lin shao chuan (email:lsccsl@tom.com, msn:lsccsl@163.net)
*
*/

#ifndef __MYOBJ_H__
#define __MYOBJ_H__


#include <stdlib.h>


/*
*
*描述对象如何被构造
*
*/
typedef void (*MYOBJ_CONSTRUCT)(void * obj, size_t obj_size, void * param, size_t param_size);

/*
*
*描述对象如何被析构
*
*/
typedef void (*MYOBJ_DESTRUCT)(void * obj, size_t obj_size);

/*
*
*描述对象如何被拷贝
*
*/
typedef void (*MYOBJ_COPY)(void * dst, size_t dst_len, const void * src, size_t src_len);


typedef struct __myobj_ops_
{
	//struct __myobj_ops_ * base;

	MYOBJ_CONSTRUCT construct;
	MYOBJ_DESTRUCT destruct;
	MYOBJ_COPY copy;
}myobj_ops;

typedef struct __myobj_id_
{
	char class_id[16];
	myobj_ops ops;
}myobj_id;

typedef struct __myobj_handle_
{int unused;}*HMYOBJ;


extern HMYOBJ myobj_new(myobj_ops * op, void * param, size_t param_size);

extern void myobj_delete(HMYOBJ ho);

extern int myobj_is_kind_of(HMYOBJ ho, const char * class_id);


#endif















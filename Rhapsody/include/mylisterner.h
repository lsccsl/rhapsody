/*
*
*mylisterner.h ��ʱ���߳� lin shao chuan
*
* ����ACE��Select_Reactorʵ�����¹���:
*  �����豸���
*  ������Ϣ����(���ȼ��������ȼ�)
*  �ṩ��ʱ���ӿ�
*
* ��δʵ�ֵĽӿ�:
*	�ṩsysv��Ϣ���м���
*	����ACE�ṩsuspend resume fd���ƵĽӿ�
*	ɾ������fd�Ľӿ�
*
*/
#ifndef __MYLISTERNER_H__
#define __MYLISTERNER_H__

#include "mymempool.h"
#include "mytimerheap.h"
#include "myhandleSet.h"


struct __mylisterner_t_;
typedef struct __mylisterner_t_ * HMYLISTERNER;


/*�����������¼��Ļص�����*/
typedef int (*LISTERNER_HANDLE_INPUT)(unsigned long context_data, int fd);

/*����������¼��Ļص�����*/
typedef int (*LISTERNER_HANDLE_OUTPUT)(unsigned long context_data, int fd);

/*�������쳣�¼��Ļص�����*/
typedef int (*LISTERNER_HANDLE_EXCEPTION)(unsigned long context_data, int fd);

/*������Ϣ���������Ϣ*/
typedef int (*CB_LISTERNER_HANDLE_MSG)(unsigned long context_data, void * msg);

typedef struct __event_handle_t_
{
	LISTERNER_HANDLE_INPUT input;
	LISTERNER_HANDLE_OUTPUT output;
	LISTERNER_HANDLE_EXCEPTION exception;

	/*�û������¼�ʱ������������*/
	unsigned long context_data;
}event_handle_t;


/*
*
* ��������߳�
*
*/
extern HMYLISTERNER MyListernerConstruct(HMYMEMPOOL hm, size_t max_msg_count);

/*
*
* ���������߳�
*
*/
extern void MyListernerDestruct(HMYLISTERNER hlisterner);

/*
*
* ���м����߳�
*
*/
extern void MyListernerRun(HMYLISTERNER hlisterner);

/*
*
* �ȴ�listern�߳��˳�
*
*/
extern void MyListernerWait(HMYLISTERNER hlisterner);

/*
*
* ��Ӷ�ʱ��
*
*/
extern HTIMERID MyListernerAddTimer(HMYLISTERNER hlisterner, mytimer_node_t * node);

/*
*
* ɾ����ʱ��
*
*/
extern int MyListernerDelTimer(HMYLISTERNER hlisterner, HTIMERID timer_id);

/*
*
* ���ö�ʱ��
*
*/
extern HTIMERID MyListernerResetTimer(HMYLISTERNER hlisterner, HTIMERID timer_id, mytimer_node_t * node);

/*
*
* ����ļ�ɨ���
*
*/
extern int MyListernerAddFD(HMYLISTERNER hlisterner, int fd, enum E_HANDLE_SET_MASK mask, event_handle_t * evt_handle);

/*
*
* ɾ���ļ�ɨ���
*
*/
extern int MyListernerDelFD(HMYLISTERNER hlisterner, int fd);

/*
*
* ���һ����Ϣ 
*
*/
extern int MyListernerAddMsg(HMYLISTERNER hlisterner, 
							 const void * user_msg, 
							 unsigned long context_data,
							 CB_LISTERNER_HANDLE_MSG handle);

/*
*
* ���һ�������ȼ�����Ϣ
*
*/
extern int MyListernerAddProrityMsg(HMYLISTERNER hlisterner, 
									int prority, 
									const void * user_msg, 
									unsigned long context_data,
									CB_LISTERNER_HANDLE_MSG handle);

/*
*
* ��ȡ��ʱ������
*
*/
int MyListernerPrint(HMYLISTERNER hlisterner, char * pctemp, size_t sz);


#endif
















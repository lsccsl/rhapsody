#ifndef __MYLOG_H__
#define __MYLOG_H__

extern void log_err(char * fmt, ...);
extern void log_warn(char * fmt, ...);
extern void log_info(char * fmt, ...);
extern void log_debug(char * fmt, ...);

#ifdef HAVE_LOG
	#define LOG_ERR(x)		do{log_err("[%s:%d]", __FILE__, __LINE__);		log_err x;		log_err("\n");}while(0)
	#define LOG_WARN(x)		do{log_warn("[%s:%d]", __FILE__, __LINE__);		log_warn x;		log_warn("\n");}while(0)
	#define LOG_INFO(x)		do{log_info("[%s:%d]", __FILE__, __LINE__);		log_info x;		log_info("\n");}while(0)
	#define LOG_DEBUG(x)	do{log_debug("[%s:%d]", __FILE__, __LINE__);	log_debug x;	log_debug("\n");}while(0)
#else
	#define LOG_ERR(x)
	#define LOG_WARN(x)
	#define LOG_INFO(x)
	#define LOG_DEBUG(x)
#endif


#endif

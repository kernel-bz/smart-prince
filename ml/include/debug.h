/**
    file name: debug.h
    author: Jung-JaeJoon(rgbi3307@nate.com) on the www.kernel.bz
*/

#ifndef __DEBUG_H__

#define DEBUG_INFO_ML
///#define DEBUG_TRACE_ML
#define DEBUG_WARN_ML

#ifdef DEBUG_INFO_ML
#define pr_debug(format, ...) fprintf (stdout, format, ## __VA_ARGS__)
#define pr_info(format, ...)  fprintf (stdout, format, ## __VA_ARGS__)
#else
#define pr_debug(format, ...)   do {} while (0)
#define pr_info(format, ...)    do {} while (0)
#endif

#ifdef DEBUG_TRACE_ML
#define pr_func_s   pr_debug("\t__start__:%s:%d\n", __FUNCTION__, __LINE__)
#define pr_func_e   pr_debug("\t__end__:%s:%d\n", __FUNCTION__, __LINE__)
#else
#define pr_func_s   do {} while (0)
#define pr_func_e   do {} while (0)
#endif

#ifdef DEBUG_WARN_ML
#define pr_warn(format, ...) fprintf (stderr, format, ## __VA_ARGS__)
#define pr_err(format, ...)  fprintf (stderr, format, ## __VA_ARGS__)
#else
#define pr_warn(format, ...) do {} while (0)
#define pr_err(format, ...)  do {} while (0)
#endif


#endif

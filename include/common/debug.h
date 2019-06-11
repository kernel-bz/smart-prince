/**
 *	file name:  include/debug.h
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   Debug Module
 */

#ifndef __DEBUG_H
#define __DEBUG_H

#define DEBUG_INFO
#define DEBUG_WARN

unsigned short int debug_level;

void debug_log(int level, int errno_flag, const char *fmt, ...);

#ifdef DEBUG_INFO
#define pr_debug_msg(format, ...)   fprintf (stdout, format, ## __VA_ARGS__)
#define pr_info_msg(format, ...)    fprintf (stdout, format, ## __VA_ARGS__)
#define pr_debug_func_start   pr_debug_msg("\t__start__:%s:%d\n", __FUNCTION__, __LINE__)
#define pr_debug_func_end     pr_debug_msg("\t__end__:%s:%d\n", __FUNCTION__, __LINE__)
#else
#define pr_debug_msg(format, ...) do {} while (0)
#define pr_info_msg(format, ...) do {} while (0)
#endif

#ifdef DEBUG_WARN
#define pr_warn_msg(format, ...)    fprintf (stdout, "WARN:%s:%d: ", __FUNCTION__, __LINE__); \
                                    fprintf (stderr, format, ## __VA_ARGS__)
#define pr_err_msg(format, ...)     fprintf (stdout, "ERROR:%s:%d: ", __FUNCTION__, __LINE__); \
                                    fprintf (stderr, format, ## __VA_ARGS__)
#else
#define pr_warn_msg(format, ...) do {} while (0)
#define pr_err_msg(format, ...) do {} while (0)
#endif


#endif

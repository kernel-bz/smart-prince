/**
 *	file name:  include/common/memory.h
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   Memory Module
 */

#ifndef __MEMORY_H
#define __MEMORY_H

#include <stddef.h>
#include <syslog.h>
#include <stdlib.h>

#include "debug.h"
#include "common/debug.h"

static inline void *mymalloc(size_t nbytes)
{
    void *dummy = malloc(nbytes);
    if (!dummy) {
        debug_log(LOG_EMERG, 1, "Could not allocate %llu bytes of memory!", (unsigned long long)nbytes);
        ///motion_remove_pid();
        exit(1);
    }

    return dummy;
}

static inline void *myrealloc(void *ptr, size_t size, const char *desc)
{
    void *dummy = NULL;

    if (size == 0) {
        free(ptr);
        debug_log(LOG_WARNING, 0,
                   "Warning! Function %s tries to resize memoryblock at %p to 0 bytes!",
                   desc, ptr);
    } else {
        dummy = realloc(ptr, size);
        if (!dummy) {
            debug_log(LOG_EMERG, 0,
                       "Could not resize memory-block at offset %p to %llu bytes (function %s)!",
                       ptr, (unsigned long long)size, desc);
            ///motion_remove_pid();
            exit(1);
        }
    }

    return dummy;
}

#endif

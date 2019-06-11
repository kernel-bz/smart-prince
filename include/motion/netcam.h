/**
 *	file name:  include/motion/netcam.h
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   Network Camera Module
 */
#ifndef __NETCAM_H
#define __NETCAM_H

#include <time.h>
#include <sys/time.h>

typedef struct netcam_image_buff {
    char *ptr;
    int content_length;
    size_t size;                                //total allocated size
    size_t used;                                //bytes already used
    struct timeval image_time;     //time this image was received
} netcam_buff;
typedef netcam_buff *netcam_buff_ptr;

#endif

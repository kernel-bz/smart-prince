/*    video.h
 *
 *    Include file for video.c
 *      Copyright 2000 by Jeroen Vreeken (pe1rxq@amsat.org)
 *      This software is distributed under the GNU public license version 2
 *      See also the file 'COPYING'.
 *
 */

#ifndef __VIDEO_H
#define __VIDEO_H

#include <pthread.h>
#include "common/types.h"
#include "video.h"
#include "motion.h"

#define _LINUX_TIME_H   1


#ifndef WITHOUT_V4L
#include <linux/videodev.h>
#include <sys/mman.h>
///#include "pwc-ioctl.h"
#endif

/* video4linux stuff */
#define NORM_DEFAULT            0
#define NORM_PAL                0
#define NORM_NTSC               1
#define NORM_SECAM              2
#define NORM_PAL_NC             3
#define IN_DEFAULT              8
#define IN_TV                   0
#define IN_COMPOSITE            1
#define IN_COMPOSITE2           2
#define IN_SVIDEO               3

/* video4linux error codes */
#define V4L_GENERAL_ERROR    0x01    /* binary 000001 */
#define V4L_BTTVLOST_ERROR   0x05    /* binary 000101 */
#define V4L_FATAL_ERROR        -1

#define VIDEO_DEVICE            "/dev/video0"
///#define VIDEO_WIDTH             320
///#define VIDEO_HEIGHT            320
#define VIDEO_WIDTH             480
#define VIDEO_HEIGHT            480
///#define VIDEO_SIZE           (VIDEO_WIDTH * VIDEO_HEIGHT * 3) / 2

struct video_dev {
    struct video_dev *next;
    int usage_count;
    int fd;
    const char *video_device;
    int input;
    int width;
    int height;
    int brightness;
    int contrast;
    int saturation;
    int hue;
    unsigned long freq;
    int tuner_number;
    int fps;

    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
    int owner;
    int frames;

    /* Device type specific stuff: */
#ifndef WITHOUT_V4L
    /* v4l */
    int v4l2;
    void *v4l2_private;

    int size_map;
    int v4l_fmt;
    unsigned char *v4l_buffers[2];
    int v4l_curbuffer;
    int v4l_maxbuffer;
    int v4l_bufsize;
#endif
};

int video_v4l2_start(struct context *tsk, struct video_dev *dev);
int video_v4l2_next(struct context *tsk, struct video_dev *dev, unsigned char *map);
void video_v4l2_close(struct video_dev *viddev);

#endif // __MOTION_VIDEO_H

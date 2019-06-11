/**
 *	file name:  include/motion/video_conv.h
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   Video Convert Module
 */

#ifndef __VIDEO_CONV_H
#define __VIDEO_CONV_H

#include "common/types.h"
#include "netcam.h"

#define CLAMP(x)  ((x) < 0 ? 0 : ((x) > 255) ? 255 : (x))

#define MAX2(x, y) ((x) > (y) ? (x) : (y))
#define MIN2(x, y) ((x) < (y) ? (x) : (y))

#define AUTOBRIGHT_HYSTERESIS 10
#define AUTOBRIGHT_DAMPER      5
#define AUTOBRIGHT_MAX       255
#define AUTOBRIGHT_MIN         0

typedef struct {
    int is_abs;
    int len;
    int val;
} code_table_t;


void conv_yuv422to420p(uint8_t *map, uint8_t *cap_map, int width, int height);
void conv_uyvyto420p(uint8_t *map, uint8_t *cap_map, uint32_t width, uint32_t height);
///int conv_jpeg2yuv420(struct context *cnt, uint8_t *dst, netcam_buff * buff, int width, int height);
int sonix_decompress(uint8_t *outp, uint8_t *inp,int width, int height);
void bayer2rgb24(uint8_t *dst, uint8_t *src, long int width, long int height);
///int vid_do_autobright(struct context *cnt, struct video_dev *viddev);
int mjpegtoyuv420p(uint8_t *map, uint8_t *cap_map, int width, int height, uint32_t size);

///void conv_yuv420p_to_rgb24(uint8_t *yuv_buffer, uint8_t *rgb_buffer, uint32_t width, uint32_t height);
void conv_yuv420p_to_gbr24(uint8_t *yuv_buffer, uint8_t *rgb_buffer, uint32_t width, uint32_t height);
void conv_yuv420p_to_gray(uint8_t *yuv_buffer, uint8_t *rgb_buffer, uint32_t width, uint32_t height);
void conv_rgb24toyuv420p(uint8_t *map, uint8_t *cap_map, int width, int height);

#endif

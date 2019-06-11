/**
 *  file name:  img_resize.c
 *  function:   images read for Machine Learning
 *  author:     JungJaeJoon(rgbi3307@nate.com) on the www.kernel.bz
 *  Copyright:  2016 www.kernel.bz
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>

#include "common/types.h"
#include "common/debug.h"
#include "haar/image.h"


static inline float _get_pixel(ImageFloat m, int x, int y, int c)
{
    assert(x < m.w && y < m.h && c < m.c);
    return m.data[c*m.h*m.w + y*m.w + x];
}

static inline float _get_pixel_extend(ImageFloat m, int x, int y, int c)
{
    if(x < 0 || x >= m.w || y < 0 || y >= m.h || c < 0 || c >= m.c) return 0;
    return _get_pixel(m, x, y, c);
}

static inline void _set_pixel(ImageFloat m, int x, int y, int c, float val)
{
    assert(x < m.w && y < m.h && c < m.c);
    m.data[c*m.h*m.w + y*m.w + x] = val;
}

static inline void _add_pixel(ImageFloat m, int x, int y, int c, float val)
{
    assert(x < m.w && y < m.h && c < m.c);
    m.data[c*m.h*m.w + y*m.w + x] += val;
}

static ImageFloat _img_stb_alloc_empty_image(int w, int h, int c)
{
    ImageFloat out;
    out.data = 0;
    out.h = h;
    out.w = w;
    out.c = c;
    return out;
}

static ImageFloat _img_stb_alloc_image(int w, int h, int c)
{
    ImageFloat out = _img_stb_alloc_empty_image(w,h,c);
    out.data = calloc(h*w*c, sizeof(float));
    return out;
}

static ImageFloat _img_stb_resize(ImageFloat im, int w, int h)
{
    ImageFloat resized = _img_stb_alloc_image(w, h, im.c);
    ImageFloat part = _img_stb_alloc_image(w, im.h, im.c);
    int r, c, k;
    float w_scale = (float)(im.w - 1) / (w - 1);
    float h_scale = (float)(im.h - 1) / (h - 1);
    for(k = 0; k < im.c; ++k){
        for(r = 0; r < im.h; ++r){
            for(c = 0; c < w; ++c){
                float val = 0;
                if(c == w-1 || im.w == 1){
                    val = _get_pixel(im, im.w-1, r, k);
                } else {
                    float sx = c*w_scale;
                    int ix = (int) sx;
                    float dx = sx - ix;
                    val = (1 - dx) * _get_pixel(im, ix, r, k) + dx * _get_pixel(im, ix+1, r, k);
                }
                _set_pixel(part, c, r, k, val);
            }
        }
    }
    for(k = 0; k < im.c; ++k){
        for(r = 0; r < h; ++r){
            float sy = r*h_scale;
            int iy = (int) sy;
            float dy = sy - iy;
            for(c = 0; c < w; ++c){
                float val = (1-dy) * _get_pixel(part, c, iy, k);
                _set_pixel(resized, c, r, k, val);
            }
            if(r == h-1 || im.h == 1) continue;
            for(c = 0; c < w; ++c){
                float val = dy * _get_pixel(part, c, iy+1, k);
                _add_pixel(resized, c, r, k, val);
            }
        }
    }

    free(part.data);
    return resized;
}

static ImageFloat _img_pooling(ImageFloat m, int w, int h)
{
    ImageFloat pooling = _img_stb_alloc_image(w, h, m.c);

    int w_filter = m.w  / w;
    int h_filter = m.h  / h;

    u32  iw, ih, idx, idx2=0;
    int i, j, k;
    f32 d, d2=FLT_MAX;
    for(i = 0 ; i < m.c; ++i) {
        for(j = 0 ; j < m.h;  j+= h_filter) {
            for(k = 0; k < m.w;  k+=w_filter) {
                d2=FLT_MAX;
                for (ih=0; ih < h_filter; ih++) {
                    for (iw=0; iw < w_filter; iw++) {
                        idx = i*m.h*m.w + (j*m.w+ih) + (k+iw);
                        d = m.data[idx];
                        if (d < d2) d2 = d;
                    }
                }
                pooling.data[idx2] = d2;
                idx2++;
            }
        }
    }
    return pooling;
}

ImageFloat img_resize(ImageByte img, int w, int h, int c)
{
    ImageFloat in = _img_stb_alloc_image(w, h, c);
    ///in.data = (float*)img.data;
    u32 i, isize=w*h*c;
    for (i=0; i < isize; i++) {
        in.data[i] = (float)img.data[i];
    }
    in.w = img.width;
    in.h = img.height;
    in.c = c;

    ImageFloat resized = _img_stb_resize(in, w, h);
    ///ImageFloat resized = _img_pooling(in, w, h);

    pr_debug_msg("%s(): w:%d-->%d, h:%d-->%d\n", __FUNCTION__, img.width, w, img.height, h);

    free(in.data);

    return resized;
}

uint32_t img_resize_pgm_to_pgm(ImageByte pgm, ImageByte* out, int w, int h, int c)
{
    if (pgm.width == w && pgm.height == h) return 0;

    ImageFloat in = _img_stb_alloc_image(pgm.width, pgm.height, c);
    ///in.data = (float*)img.data;
    uint32_t i, isize=pgm.width * pgm.height * c;
    for (i=0; i < isize; i++) {
        in.data[i] = (float)pgm.data[i];
    }
    in.w = pgm.width;
    in.h = pgm.height;
    in.c = c;

    ImageFloat resized = _img_stb_resize(in, w, h);
    ///ImageFloat resized = _img_pooling(in, w, h);

    isize = w * h * c;
    uint8_t *in_data = calloc(isize, sizeof(uint8_t));

    for (i=0; i < isize; i++) {
        in_data[i] = (unsigned char)resized.data[i];    ///float to byte
    }
    img_to_pgm (in_data, w, h, out);

    if (in.data) free(in.data);
    if (resized.data) free(resized.data);
    if (in_data) free(in_data);
    if (pgm.data) free(pgm.data);
    pr_debug_msg("%s(): w:%d-->%d, h:%d-->%d\n", __FUNCTION__,
                                pgm.width, w, pgm.height, h);
    return isize;
}

ImageFloat img_load_from_pgm(ImageByte pgm, int w, int h, int c)
{
    ImageFloat img = _img_stb_alloc_image(w, h, c);

    if (pgm.width != w || pgm.height != h) {
        pr_info_msg("%s(): DIFF: w:%d != %d, h:%d != %d\n", __FUNCTION__
                                , pgm.width, w, pgm.height, h);

        ///w = (w > pgm.width) ?  pgm.width : w;
        ///h = (h > pgm.height) ? pgm.height : h;
        return img;
    }
    ///resized.data = (float*)img.data;
    u32 i, isize=w*h*c;
    for (i=0; i < isize; i++) {
        img.data[i] = (float)pgm.data[i] / (float)255;
    }
    img.w = w;
    img.h = h;
    img.c = c;
    ///img_resize_print_image(img);
    return img;
}

void img_resize_print_image(ImageFloat m)
{
    int i, j, k;
    f32 d;
    i32 d2;

    for(i = 0 ; i < m.c; ++i){
        for(j = 0 ; j < m.h; ++j){
            for(k = 0; k < m.w; ++k){
                d = m.data[i*m.h*m.w + j*m.w + k];
                //printf("%d ", (i32)(d*255));
                d2 = d * 255;
                if (d2 < 32) printf("@");
                else if (d2 < 64) printf("$");
                else if (d2 < 96) printf("#");
                else if (d2 < 128) printf("*");
                else if (d2 < 160) printf("+");
                else if (d2 < 192) printf("'");
                else printf(".");
            }
            printf("\n");
        }
        printf("\n\n");
    }
    printf("\n");
}


/**
 *	file name:  sp_eyes/video_conv.c
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   Video Convert Module for SmartPrince
 */

#include "common/debug.h"
#include "common/types.h"
#include "motion/netcam.h"
#include "motion/video_conv.h"
#include "motion/motion.h"
#include "motion/jpegutils.h"

/*
 *   sonix_decompress_init
 *   =====================
 *   pre-calculates a locally stored table for efficient huffman-decoding.
 *
 *   Each entry at index x in the table represents the codeword
 *   present at the MSB of byte x.
 *
 */
static void sonix_decompress_init(code_table_t * table)
{
    int i;
    int is_abs, val, len;

    for (i = 0; i < 256; i++) {
        is_abs = 0;
        val = 0;
        len = 0;
        if ((i & 0x80) == 0) {
            /* code 0 */
            val = 0;
            len = 1;
        } else if ((i & 0xE0) == 0x80) {
            /* code 100 */
            val = +4;
            len = 3;
        } else if ((i & 0xE0) == 0xA0) {
            /* code 101 */
            val = -4;
            len = 3;
        } else if ((i & 0xF0) == 0xD0) {
            /* code 1101 */
            val = +11;
            len = 4;
        } else if ((i & 0xF0) == 0xF0) {
            /* code 1111 */
            val = -11;
            len = 4;
        } else if ((i & 0xF8) == 0xC8) {
            /* code 11001 */
            val = +20;
            len = 5;
        } else if ((i & 0xFC) == 0xC0) {
            /* code 110000 */
            val = -20;
            len = 6;
        } else if ((i & 0xFC) == 0xC4) {
            /* code 110001xx: unknown */
            val = 0;
            len = 8;
        } else if ((i & 0xF0) == 0xE0) {
            /* code 1110xxxx */
            is_abs = 1;
            val = (i & 0x0F) << 4;
            len = 8;
        }
        table[i].is_abs = is_abs;
        table[i].val = val;
        table[i].len = len;
    }
}

/*
 *   sonix_decompress
 *   ================
 *   decompresses an image encoded by a SN9C101 camera controller chip.
 *
 *   IN    width
 *         height
 *         inp     pointer to compressed frame (with header already stripped)
 *   OUT   outp    pointer to decompressed frame
 *
 *         Returns 0 if the operation was successful.
 *         Returns <0 if operation failed.
 *
 */
int sonix_decompress(uint8_t *outp, uint8_t *inp, int width, int height)
{
    int row, col;
    int val;
    int bitpos;
    uint8_t code;
    uint8_t *addr;

    /* local storage */
    static code_table_t table[256];
    static int init_done = 0;

    if (!init_done) {
        init_done = 1;
        sonix_decompress_init(table);
        /* do sonix_decompress_init first! */
        //return -1; // so it has been done and now fall through
    }

    bitpos = 0;
    for (row = 0; row < height; row++) {

        col = 0;

        /* first two pixels in first two rows are stored as raw 8-bit */
        if (row < 2) {
            addr = inp + (bitpos >> 3);
            code = (addr[0] << (bitpos & 7)) | (addr[1] >> (8 - (bitpos & 7)));
            bitpos += 8;
            *outp++ = code;

            addr = inp + (bitpos >> 3);
            code = (addr[0] << (bitpos & 7)) | (addr[1] >> (8 - (bitpos & 7)));
            bitpos += 8;
            *outp++ = code;

            col += 2;
        }

        while (col < width) {
            /* get bitcode from bitstream */
            addr = inp + (bitpos >> 3);
            code = (addr[0] << (bitpos & 7)) | (addr[1] >> (8 - (bitpos & 7)));

            /* update bit position */
            bitpos += table[code].len;

            /* calculate pixel value */
            val = table[code].val;
            if (!table[code].is_abs) {
                /* value is relative to top and left pixel */
                if (col < 2) {
                    /* left column: relative to top pixel */
                    val += outp[-2 * width];
                } else if (row < 2) {
                    /* top row: relative to left pixel */
                    val += outp[-2];
                } else {
                    /* main area: average of left pixel and top pixel */
                    val += (outp[-2] + outp[-2 * width]) / 2;
                }
            }

            /* store pixel */
            *outp++ = CLAMP(val);
            col++;
        }
    }

    return 0;
}

/*
 * BAYER2RGB24 ROUTINE TAKEN FROM:
 *
 * Sonix SN9C10x based webcam basic I/F routines
 * Takafumi Mizuno <taka-qce@ls-a.jp>
 *
 */

void bayer2rgb24(uint8_t *dst, uint8_t *src, long int width, long int height)
{
    long int i;
    uint8_t *rawpt, *scanpt;
    long int size;

    rawpt = src;
    scanpt = dst;
    size = width * height;

    for (i = 0; i < size; i++) {
        if (((i / width) & 1) == 0) {    // %2 changed to & 1
            if ((i & 1) == 0) {
                /* B */
                if ((i > width) && ((i % width) > 0)) {
                    *scanpt++ = *rawpt;     /* B */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1) +
                                *(rawpt + width) + *(rawpt - width)) / 4;    /* G */
                    *scanpt++ = (*(rawpt - width - 1) + *(rawpt - width + 1) +
                                *(rawpt + width - 1) + *(rawpt + width + 1)) / 4;    /* R */
                } else {
                    /* first line or left column */
                    *scanpt++ = *rawpt;     /* B */
                    *scanpt++ = (*(rawpt + 1) + *(rawpt + width)) / 2;    /* G */
                    *scanpt++ = *(rawpt + width + 1);       /* R */
                }
            } else {
                /* (B)G */
                if ((i > width) && ((i % width) < (width - 1))) {
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1)) / 2;  /* B */
                    *scanpt++ = *rawpt;    /* G */
                    *scanpt++ = (*(rawpt + width) + *(rawpt - width)) / 2;  /* R */
                } else {
                    /* first line or right column */
                    *scanpt++ = *(rawpt - 1);       /* B */
                    *scanpt++ = *rawpt;    /* G */
                    *scanpt++ = *(rawpt + width);   /* R */
                }
            }
        } else {
            if ((i & 1) == 0) {
                /* G(R) */
                if ((i < (width * (height - 1))) && ((i % width) > 0)) {
                    *scanpt++ = (*(rawpt + width) + *(rawpt - width)) / 2;  /* B */
                    *scanpt++ = *rawpt;    /* G */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1)) / 2;  /* R */
                } else {
                    /* bottom line or left column */
                    *scanpt++ = *(rawpt - width);   /* B */
                    *scanpt++ = *rawpt;    /* G */
                    *scanpt++ = *(rawpt + 1);       /* R */
                }
            } else {
                /* R */
                if (i < (width * (height - 1)) && ((i % width) < (width - 1))) {
                    *scanpt++ = (*(rawpt - width - 1) + *(rawpt - width + 1) +
                                *(rawpt + width - 1) + *(rawpt + width + 1)) / 4;    /* B */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt + 1) + *(rawpt - width) +
                                *(rawpt + width)) / 4;    /* G */
                    *scanpt++ = *rawpt;     /* R */
                } else {
                    /* bottom line or right column */
                    *scanpt++ = *(rawpt - width - 1);       /* B */
                    *scanpt++ = (*(rawpt - 1) + *(rawpt - width)) / 2;    /* G */
                    *scanpt++ = *rawpt;     /* R */
                }
            }
        }
        rawpt++;
    }

}

void conv_yuv422to420p(uint8_t *map, uint8_t *cap_map, int width, int height)
{
    uint8_t *src, *dest, *src2, *dest2;
    int i, j;

    /* Create the Y plane */
    src = cap_map;
    dest = map;
    for (i = width * height; i > 0; i--) {
        *dest++ = *src;
        src += 2;
    }
    /* Create U and V planes */
    src = cap_map + 1;
    src2 = cap_map + width * 2 + 1;
    dest = map + width * height;
    dest2 = dest + (width * height) / 4;
    for (i = height / 2; i > 0; i--) {
        for (j = width / 2; j > 0; j--) {
            *dest = ((int) *src + (int) *src2) / 2;
            src += 2;
            src2 += 2;
            dest++;
            *dest2 = ((int) *src + (int) *src2) / 2;
            src += 2;
            src2 += 2;
            dest2++;
        }
        src += width * 2;
        src2 += width * 2;
    }
}

void conv_uyvyto420p(uint8_t *map, uint8_t *cap_map, uint32_t width, uint32_t height)
{
    uint8_t *pY = map;
    uint8_t *pU = pY + (width * height);
    uint8_t *pV = pU + (width * height) / 4;
    uint32_t uv_offset = width * 2 * sizeof(uint8_t);
    uint32_t ix, jx;

    for (ix = 0; ix < height; ix++) {
        for (jx = 0; jx < width; jx += 2) {
            uint16_t calc;
            if ((ix&1) == 0) {
                calc = *cap_map;
                calc += *(cap_map + uv_offset);
                calc /= 2;
                *pU++ = (uint8_t) calc;
            }
            cap_map++;
            *pY++ = *cap_map++;
            if ((ix&1) == 0) {
                calc = *cap_map;
                calc += *(cap_map + uv_offset);
                calc /= 2;
                *pV++ = (uint8_t) calc;
            }
            cap_map++;
            *pY++ = *cap_map++;
        }
    }
}

void conv_rgb24toyuv420p(uint8_t *map, uint8_t *cap_map, int width, int height)
{
    uint8_t *y, *u, *v;
    uint8_t *r, *g, *b;
    int i, loop;

    b = cap_map;
    g = b + 1;
    r = g + 1;
    y = map;
    u = y + width * height;
    v = u + (width * height) / 4;
    memset(u, 0, width * height / 4);
    memset(v, 0, width * height / 4);

    for (loop = 0; loop < height; loop++) {
        for (i = 0; i < width; i += 2) {
            *y++ = (9796 ** r + 19235 ** g + 3736 ** b) >> 15;
            *u += ((-4784 ** r - 9437 ** g + 14221 ** b) >> 17) + 32;
            *v += ((20218 ** r - 16941 ** g - 3277 ** b) >> 17) + 32;
            r += 3;
            g += 3;
            b += 3;
            *y++ = (9796 ** r + 19235 ** g + 3736 ** b) >> 15;
            *u += ((-4784 ** r - 9437 ** g + 14221 ** b) >> 17) + 32;
            *v += ((20218 ** r - 16941 ** g - 3277 ** b) >> 17) + 32;
            r += 3;
            g += 3;
            b += 3;
            u++;
            v++;
        }

        if ((loop & 1) == 0) {
            u -= width / 2;
            v -= width / 2;
        }
    }
}

#if 0
/**
//Converts YUV420 to RGB24(BGR888) format
//https://forum.videohelp.com/images/guides/p1800190/yuv_to_bgr.c
    For a YUV 420 data with width & height is 4x6
    Following is the pattern it was stored
    Y1  Y2  Y3  Y4  Y5  Y6
    Y7  Y8  Y9  Y10 Y11 Y12
    Y13 Y14 Y15 Y16 Y17 Y18
    Y19 Y20 Y21 Y22 Y23 Y24
    U1  U2  U3  U4  U5  U6
    V1  V2  V3  V4  V5  V6
    Please note Y1 Y2 Y7 Y8 will be sharing U1 and V1
    &
    Y3 Y4 Y9 Y10 will be sharing U2 V2
    Following is the plan
    We know that Y1 & Y2 is going to pair with the first U (say U1) & V (say V1)
    That means trigger the movement of cb & cr pointer
    only for 2 y movement
*/
void conv_yuv420p_to_rgb24(uint8_t *yuv_buffer, uint8_t *rgb_buffer, uint32_t width, uint32_t height)
{
  uint8_t  *rgb_buffer_1strow, *rgb_buffer_2ndrow;
  uint8_t y_start, u_start, v_start;

  uint8_t *yuv_buffer_1strow, *yuv_buffer_2ndrow;
  int r,g,b, r_prod, g_prod, b_prod;
  int y,u,v;
  uint32_t i_conv_ht =0,i_conv_wt =0;
  uint32_t y_size;
  uint8_t *cb_ptr;
  uint8_t *cr_ptr;

  y_size = width*height;

  //Calculate the Cb & Cr pointer
  cb_ptr = yuv_buffer + y_size;
  cr_ptr = cb_ptr + y_size/4;

  yuv_buffer_1strow = yuv_buffer;
  yuv_buffer_2ndrow = yuv_buffer+width;

  rgb_buffer_1strow = rgb_buffer ;
  rgb_buffer_2ndrow = rgb_buffer+(3*width);

  for (i_conv_ht =0; i_conv_ht<height; i_conv_ht+=2)
  {
    for (i_conv_wt =0; i_conv_wt<width; i_conv_wt+=2)
    {
       //Extract the Y value
        y_start = *yuv_buffer_1strow++;
        y = (y_start -16)*298;

        //extract  the value
        u_start = *cb_ptr++;
        v_start = *cr_ptr++;

       //precalculate it
        u = u_start - 128;
        v = v_start - 128;

        r_prod = 409*v + 128;
        g_prod = 100*u + 208*v - 128;
        b_prod = 516*u;

        ///now it is time to do the conversion
        r =(y + r_prod)>>8;
        g =(y - g_prod)>>8;
        b =(y + b_prod)>>8;

        //Now clip and store
        *rgb_buffer_1strow++ = CLAMP(r);
        *rgb_buffer_1strow++ = CLAMP(g);
        *rgb_buffer_1strow++ = CLAMP(b);

        //Extract the Y value
        y_start = *yuv_buffer_1strow++;
        y = (y_start -16)*298;

        ///now it is time to do the conversion
        r =(y + r_prod)>>8;
        g =(y - g_prod)>>8;
        b =(y + b_prod)>>8;

        ///Now clip and store
        *rgb_buffer_1strow++ = CLAMP(r);
        *rgb_buffer_1strow++ = CLAMP(g);
        *rgb_buffer_1strow++ = CLAMP(b);

        //Extract the Y value
        y_start = *yuv_buffer_2ndrow++;
        y = (y_start -16)*298;

        ///now it is time to do the conversion
        r =(y + r_prod)>>8;
        g =(y - g_prod)>>8;
        b =(y + b_prod)>>8;

        ///Now clip and store
        *rgb_buffer_2ndrow++ = CLAMP(r);
        *rgb_buffer_2ndrow++ = CLAMP(g);
        *rgb_buffer_2ndrow++ = CLAMP(b);

        //Extract the Y value
        y_start = *yuv_buffer_2ndrow++;
        y = (y_start -16)*298;

        ///now it is time to do the conversion
        r =(y + r_prod)>>8;
        g =(y - g_prod)>>8;
        b =(y + b_prod)>>8;

        ///Now clip and store
        *rgb_buffer_2ndrow++ = CLAMP(r);
        *rgb_buffer_2ndrow++ = CLAMP(g);
        *rgb_buffer_2ndrow++ = CLAMP(b);
      } //for width
      yuv_buffer_1strow += width;
      yuv_buffer_2ndrow += width;

      rgb_buffer_1strow += 3*width;
      rgb_buffer_2ndrow += 3*width;
    } //for height
}
#endif // 0

void conv_yuv420p_to_gbr24(uint8_t *yuv_buffer, uint8_t *rgb_buffer, uint32_t width, uint32_t height)
{
    int x, y;
    uint8_t *l = yuv_buffer;
    uint8_t *u = yuv_buffer + width * height;
    uint8_t *v = u + (width * height) / 4;
    int r, g, b;

     ///ppm header: "P6", width, height, maxval
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            r = 76283* (((int)*l) - 16) + 104595 * (((int)*u) - 128);
            g = 76283* (((int)*l) - 16)- 53281 * (((int)*u) - 128)-25625*(((int)*v)-128);
            b = 76283* (((int)*l) - 16) + 132252 * (((int)*v) - 128);
            r = r>>16;
            g = g>>16;
            b = b>>16;

            l++;
            if (x & 1) {
                u++;
                v++;
            }
            //ppm is rgb not bgr
            *rgb_buffer++ = CLAMP(b);
            *rgb_buffer++ = CLAMP(g);
            *rgb_buffer++ = CLAMP(r);
        } //for width

        if (y & 1) {
            u -= width / 2;
            v -= width / 2;
        }
    } //for height
}

void conv_yuv420p_to_gray(uint8_t *yuv_buffer, uint8_t *rgb_buffer, uint32_t width, uint32_t height)
{
    int x, y;
    uint8_t *l = yuv_buffer;
    uint8_t *u = yuv_buffer + width * height;
    uint8_t *v = u + (width * height) / 4;
    int r, g, b;
    uint8_t gray;

     ///ppm header: "P6", width, height, maxval
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            r = 76283* (((int)*l) - 16) + 104595 * (((int)*u) - 128);
            g = 76283* (((int)*l) - 16)- 53281 * (((int)*u) - 128)-25625*(((int)*v)-128);
            b = 76283* (((int)*l) - 16) + 132252 * (((int)*v) - 128);
            r = r>>16;
            g = g>>16;
            b = b>>16;

            l++;
            if (x & 1) {
                u++;
                v++;
            }

            b = CLAMP(b) * 100;
            g = CLAMP(g) * 700;
            r = CLAMP(r) * 100;
            gray = (b + g + r) >> 10;

            *rgb_buffer++ = gray;
        } //for width

        if (y & 1) {
            u -= width / 2;
            v -= width / 2;
        }
    } //for height
}

/**
int conv_jpeg2yuv420(struct context *cnt, uint8_t *dst, netcam_buff * buff, int width, int height)
{
    netcam_context netcam;

    if (!buff || !dst)
        return 3;

    if (!buff->ptr)
        return 2;    //Error decoding MJPEG frame

    memset(&netcam, 0, sizeof(netcam));
    netcam.imgcnt_last = 1;
    netcam.latest = buff;
    netcam.width = width;
    netcam.height = height;
    netcam.cnt = cnt;

    pthread_mutex_init(&netcam.mutex, NULL);
    pthread_cond_init(&netcam.cap_cond, NULL);
    pthread_cond_init(&netcam.pic_ready, NULL);
    pthread_cond_init(&netcam.exiting, NULL);

    if (setjmp(netcam.setjmp_buffer))
        return NETCAM_GENERAL_ERROR | NETCAM_JPEG_CONV_ERROR;


    return netcam_proc_jpeg(&netcam, dst);
}
*/


/*
 * mjpegtoyuv420p
 *
 * Return values
 *  -1 on fatal error
 *  0  on success
 *  2  if jpeg lib threw a "corrupt jpeg data" warning.
 *     in this case, "a damaged output image is likely."
 */

int mjpegtoyuv420p(uint8_t *map, uint8_t *cap_map, int width, int height, uint32_t size)
{
    uint8_t *yuv[3];
    uint8_t *y, *u, *v;
    int loop, ret;

    yuv[0] = malloc(width * height * sizeof(yuv[0][0]));
    yuv[1] = malloc(width * height / 4 * sizeof(yuv[1][0]));
    yuv[2] = malloc(width * height / 4 * sizeof(yuv[2][0]));


    ret = decode_jpeg_raw(cap_map, size, 0, 420, width, height, yuv[0], yuv[1], yuv[2]);

    if (ret == 1) {
        if (debug_level >= CAMERA_WARNINGS)
            debug_log(LOG_ERR, 0, "%s: Corrupt image ... continue", __FUNCTION__);
        ret = 2;
    }


    y = map;
    u = y + width * height;
    v = u + (width * height) / 4;
    memset(y, 0, width * height);
    memset(u, 0, width * height / 4);
    memset(v, 0, width * height / 4);

    for(loop = 0; loop < width * height; loop++)
        *map++=yuv[0][loop];

    for(loop = 0; loop < width * height / 4; loop++)
        *map++=yuv[1][loop];

    for(loop = 0; loop < width * height / 4; loop++)
        *map++=yuv[2][loop];

    free(yuv[0]);
    free(yuv[1]);
    free(yuv[2]);

    return ret;
}

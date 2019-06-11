/**
 *	file name:  sp_eyes/motion.c
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   Motion Module for SmartPrince
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
//#include <dirent.h>
#include <syslog.h>
#include <assert.h>
#include <math.h>

#include "common/config.h"
#include "common/debug.h"
#include "common/memory.h"
#include "common/file.h"

#include "haar/haar.h"
#include "haar/image.h"
#include "haar/nn_cnn.h"

#include "motion/video.h"
#include "motion/video_conv.h"
#include "motion/motion.h"
#include "motion/picture.h"
#include "motion/event.h"
#include "motion/rotate.h"

#include "devices/serial.h"
#include "devices/motor.h"
#include "devices/i2c.h"
#include "devices/intr.h"

#include "darknet.h"

extern uint32_t FaceProcessing; ///ml/examples/classifier.c

static struct context *Task;
static struct video_dev *VideoDevice;
uint8_t *ConvImg;
uint32_t Converted=0;
uint32_t DiffCaptured=0;

uint8_t static _diff_pooling_max(uint8_t *buf, uint32_t width, uint32_t wx, uint32_t wy)
{
    uint32_t x, y, offset;
    uint8_t byte, max=0;

    for (y=0; y < wy; y++) {
        offset = width * y;
        for (x=0; x < wx; x++) {
            byte =  *(buf+offset+x);
            max = (byte > max) ? byte : max;
        }   //for x
    }   //for y
    return max;
}

///finding differents of image
static uint32_t _motion_diff(struct context *tsk)
{
    uint8_t *img_buf0 = tsk->imgs.ref;
    uint8_t *img_buf1 = tsk->imgs.out;
    const uint32_t width = tsk->imgs.width;
    const uint32_t height = tsk->imgs.height;
    //const uint32_t size = tsk->imgs.motionsize;
    //const uint32_t step = tsk->imgs.motionsize / (width * height);
    const uint32_t wx=10, wy=10;    ///polling window size
    //const uint32_t mx = width / wx;    ///320/10==32(must be 32)
    uint32_t my = height / wy;    ///320/10==32
    uint32_t my_div1 = my / 3;
    uint32_t my_div2 = my_div1 * 2;
    uint32_t x, y, offset, r=0, c;
    uint32_t diff_mat[33], right=0, left=0, up=0, down=0, center=0, sum;
    uint8_t m0, m1;  ///pooling max data
    int diff;

    //memset(diff_mat, 0, my);
    for (y=0; y < my; y++) diff_mat[y] = 0;

    for (y=0; y < height; y += wy) {
        offset = width * y;
        c = 0;
        for (x=0; x < width; x += wx) {
            m0 = _diff_pooling_max(img_buf0+offset+x, width, wx, wy);
            m1 = _diff_pooling_max(img_buf1+offset+x, width, wx, wy);
            diff = m0 - m1;
            diff *= diff;
            //printf("%d\n", diff);
            if (diff > DIFF_VALUE)
                diff_mat[r] |= (1 << c);
            c++;    ///column
        }   //for x
        ///printf("[%02d]%08X\n", r, diff_mat[r]);

        diff = (diff_mat[r] & DIFF_MASK_RIGHT) >> 24;
        if (diff) right++;

        diff = (diff_mat[r] & DIFF_MASK_LEFT);
        if (diff) left++;

        diff = (diff_mat[r] & DIFF_MASK_CENTER) >> 12;
        if (diff) {
            if (r < my_div1) up++;
            else if (r > my_div2) down++;
            else center++;
        }

        r++;    ///row
    }   //for y

    sum = right + left + up + down + center;
    return (sum > 0) ? 1 : 0;

#if 0
    if (sum > 0) {
        char cmd[40] = {0,};
        uint32_t max;
        int s, d, ratio;

        max = (right > left) ? right : left;
        max = (max > up) ? max : up;
        max = (max > down) ? max : down;
        max = (max > center) ? max : center;
        ratio = max * 100 / sum;
        ///s = ratio / 10 - 2;
        s = ratio / 9 - 2;
        if (s <= 0) return 0;
        if (s > 9) s = 9;

        MotorControl.act++;
        d = max;

        if (max == up) {
            sprintf(cmd, "AT+MDTR=%d,%d,%d,%d\r\n",  MI3_UPPER, s, MD_CCW, d);   ///index, speed, dir, count
            serial_send(cmd);
            MotorControl.dir = MD_UP;
            printf("Up(s=%d, d=%d)\n", s, d);
        } else if (max == down) {
            d = d/2 + 1;
            sprintf(cmd, "AT+MDTR=%d,%d,%d,%d\r\n",  MI3_UPPER, s, MD_CW, d);
            serial_send(cmd);
            MotorControl.dir = MD_DOWN;
            printf("Down(s=%d, d=%d)\n", s, d);
        } else if (max == right) {
            sprintf(cmd, "AT+MDTR=%d,%d,%d,%d\r\n",  MI4_BOTTOM, s, MD_CCW, d);
            serial_send(cmd);
            MotorControl.dir = MD_RIGHT;
            printf("Right(s=%d, d=%d)\n", s, d);
        } else if (max == left) {
            sprintf(cmd, "AT+MDTR=%d,%d,%d,%d\r\n",  MI4_BOTTOM, s, MD_CW, d);
            serial_send(cmd);
            MotorControl.dir = MD_LEFT;
            printf("Left(s=%d, d=%d)\n", s, d);
        } else if (max == center) {
            d = d/3 + 1;
            if (MotorControl.dir & MD_RIGHT)
                sprintf(cmd, "AT+MDTR=%d,%d,%d,%d\r\n",  MI4_BOTTOM, s, MD_CW, d);   ///Left
            else if (MotorControl.dir & MD_LEFT)
                sprintf(cmd, "AT+MDTR=%d,%d,%d,%d\r\n",  MI4_BOTTOM, s, MD_CCW, d);  ///Right
            else if (MotorControl.dir & MD_UP)
                sprintf(cmd, "AT+MDTR=%d,%d,%d,%d\r\n",  MI3_UPPER, s, MD_CW, d);    ///Down
            else if (MotorControl.dir & MD_DOWN)
                sprintf(cmd, "AT+MDTR=%d,%d,%d,%d\r\n",  MI3_UPPER, s, MD_CCW, d);   ///Up
            else {
                sprintf(cmd, "AT+MSTP=%d\r\n", MI5_ALL);
                MotorControl.act = MD_STOP;
            }
            serial_send(cmd);
            MotorControl.dir = MD_STOP;
            printf("Center[%d]\n", MotorControl.dir);
            //printf("R[%03d], R=%d, L=%d, U=%d, D=%d, C=%d\n\n", ratio, right, left, up, down, center);
        }

        ///while(MotorControl.act) usleep(max);

        MotorControl.act = MD_GO;
        usleep(500);

        return 1;   //DiffCaptured

    } else {
        return 0;
    }
#endif
}

static int _motion_init(struct context *tsk, struct video_dev *dev, int color)
{
    dev->v4l_fmt = VIDEO_PALETTE_YUV420P;
    dev->v4l_bufsize = (dev->width * dev->height * 3) / 2;    ///Video Memory Copy Size(YUV)

    tsk->currenttime_tm = mymalloc(sizeof(struct tm));
    tsk->eventtime_tm = mymalloc(sizeof(struct tm));
    //Init frame time
    tsk->currenttime = time(NULL);
    localtime_r(&tsk->currenttime, tsk->currenttime_tm);

    tsk->smartmask_speed = 0;
    tsk->event_nr = 1;
    tsk->prev_event = 0;
    tsk->lightswitch_framecounter = 0;
    tsk->detecting_motion = 0;
    tsk->makemovie = 0;

    tsk->imgs.width = dev->width;
    tsk->imgs.height = dev->height;

    tsk->conf.quality = 100; //quality is the jpeg encoding quality 0-100%, put_picture()
    ///tsk->conf.autobright = 1;

    ///dev->fd
    tsk->video_dev = video_v4l2_start(tsk, dev);
    if (tsk->video_dev < 0) {
        debug_log(LOG_ERR, 0, "Could not fetch initial image from camera");
        return -1;
    }

    if (color) {
        ///tsk->conf.ppm = VIDEO_PALETTE_RGB24;         ///Save to ppm(rgb24) file
        tsk->conf.ppm = VIDEO_PALETTE_YUV420P;                          ///Save to jpg file
        tsk->imgs.motionsize = dev->width * dev->height * 3;    ///RGB24 Size
    } else {
        tsk->conf.ppm = VIDEO_PALETTE_GREY;                                 ///Save to ppm(gray) file
        tsk->imgs.motionsize = dev->width * dev->height;            ///GRAY Size
    }

    ///YUV420 size: (dev->width * dev->height * 3) / 2;
    tsk->imgs.image_virgin = mymalloc(tsk->imgs.size);
    tsk->imgs.ref = mymalloc(tsk->imgs.motionsize);
    tsk->imgs.out = mymalloc(tsk->imgs.motionsize);

#if 0
    //Create a initial precapture ring buffer with 1 frame
    ///image_ring_resize(cnt, 1);

    tsk->imgs.ref_dyn = mymalloc(tsk->imgs.motionsize * sizeof(tsk->imgs.ref_dyn));
    tsk->imgs.smartmask = mymalloc(tsk->imgs.motionsize);
    tsk->imgs.smartmask_final = mymalloc(tsk->imgs.motionsize);
    tsk->imgs.smartmask_buffer = mymalloc(tsk->imgs.motionsize * sizeof(tsk->imgs.smartmask_buffer));
    tsk->imgs.labels = mymalloc(tsk->imgs.motionsize * sizeof(tsk->imgs.labels));
    tsk->imgs.labelsize = mymalloc((tsk->imgs.motionsize/2+1) * sizeof(tsk->imgs.labelsize));

    //allocate buffer here for preview buffer
    tsk->imgs.preview_image.image = mymalloc(tsk->imgs.size);

    /* Allocate a buffer for temp. usage in some places */
    /* Only despeckle & bayer2rgb24() for now for now... */
    tsk->imgs.common_buffer = mymalloc(3 * tsk->imgs.width * tsk->imgs.height);
#endif // 0

    tsk->conf.rotate_deg = 90;
    rotate_init(tsk);

    return 0;
}

static void _motion_write_picture(uint32_t count)
{
    char fname[80];
    sprintf(fname, "%s/img_%08d.%s", PATH_DATA_ML_BAK1, count, imageext(Task));
    put_picture(Task, fname,Task->imgs.image_virgin, FTYPE_IMAGE_MOTION);
    pr_info_msg("Capture: %d: %s, %d\n", count, fname, Task->conf.quality);
}

static uint32_t _motion_capture(int ppm)
{
    uint32_t i, retry=5, count=0;
    uint32_t diff=1;

    while (count < 8)
    {
        ///memset(Task->imgs.image_virgin, 0x80, Task->imgs.size);       //initialize to grey
        for (i = 0; i < retry; i++) {
            if (video_v4l2_next(Task, VideoDevice, Task->imgs.image_virgin) == 0)
                break;    ///Video Copy OK
            usleep(10000);  ///10ms
        } //for

        count++;
        ConvImg = (count & 0) ? Task->imgs.out : Task->imgs.ref;

        if (i >= retry) {
            debug_log(LOG_ERR, 0, "Error capturing first image");
            goto _end;
        } else {
            ///if (Task->conf.ppm==VIDEO_PALETTE_GREY)
            if (ppm == VIDEO_PALETTE_GREY)
                conv_yuv420p_to_gray(Task->imgs.image_virgin, ConvImg, VideoDevice->width , VideoDevice->height);
            else
                conv_yuv420p_to_gbr24(Task->imgs.image_virgin, ConvImg, VideoDevice->width , VideoDevice->height);
        }
    } //while

_end:
    //diff = _motion_diff(Task);   ///finding differents of image
    //pr_info_msg("%s(): diff=%d.\n", __FUNCTION__, diff);
    return diff;
}


static void _motion_face_detection(void)
{
    ImageByte imageObj;
    ImageByte *image = &imageObj;
    int ret;

    ///VIDEO_PALETTE_GREY;      ///Save to ppm(gray) file
    ///conv_yuv420p_to_gray(Task->imgs.image_virgin, ConvImg, VideoDevice->width , VideoDevice->height);
    ret = img_to_pgm(ConvImg, VideoDevice->width , VideoDevice->height, image);

    ///sprintf(src, "%s/%s.pgm", PATH_DATA_ML_SAVE, entry->d_name);
    ///writePgm(src, image);

    if (ret == -1) {
        pr_err_msg("%s(): Error on the img_to_pgm()\n", __FUNCTION__);
    } else {
        ///uint32_t flag = ML_LEARN  | ML_ANSWER | ML_SAVE;
        uint32_t flag = ML_SAVE;
        pr_info_msg("%s(): Running...\n", __FUNCTION__);
        haar_run (image, PATH_DATA_ML_SAVE, "pic", flag);

        freeImage(image);
    }
}


void *motion_thread_loop(void *arg)
{
    uint64_t cnt=0;
    unsigned int run_mode = 1 << RunCfg.mode;

    while(1)
    {
        ///memset(Task->imgs.image_virgin, 0x80, Task->imgs.size);       //initialize to grey
        cnt++;
        usleep(10000);  //10ms
        if (FaceProcessing) continue;

        if (run_mode & CONFIG_MODE_CAP) {
            _motion_capture(VIDEO_PALETTE_YUV420P);
            _motion_write_picture(cnt);   //Save to File
            usleep(2000000);  //2s

        } else {
            _motion_capture(VIDEO_PALETTE_GREY);            ///Task->imgs.image_virgin --> ConvImg
            _motion_face_detection();
        }
    } //while
}

void motion_thread_start(int width, int height, char *dev_name, int color)
{
    int ret;

    Task = mymalloc(sizeof( struct context));
    memset(Task, 0, sizeof(struct context));

    Task->conf.v4l2_palette = 8;
    Task->running = 1;

    VideoDevice = mymalloc(sizeof( struct video_dev));
    memset(VideoDevice, 0, sizeof(struct video_dev));

    VideoDevice->video_device = dev_name;       ///"/dev/video0"
    ///Size must be 16times
    VideoDevice->width = width;
    VideoDevice->height = height;

    ret = _motion_init(Task, VideoDevice, color) ;
    if (ret < 0) {
        pr_err_msg("_motion_init() error!\n");
        video_v4l2_close(VideoDevice);
        free(VideoDevice);
        free(Task);
        return;
    }

    ///read classifier array from file(info.txt and class.txt)
	readTextClassifier();   ///for face detection

    pthread_t  tid;
    if (pthread_create(&tid, NULL, motion_thread_loop, NULL)) {
        pr_err_msg("can't create motion_thread_loop()\n");
        video_v4l2_close(VideoDevice);
        free(VideoDevice);
        free(Task);
    }

}

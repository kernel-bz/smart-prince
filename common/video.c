/**
 *	file name:  sp_eyes/video.c
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   Video Module for SmartPrince
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fcntl.h>
///#include <fcntl.h>
#include <syslog.h>
#include <linux/videodev2.h>

#include "common/debug.h"
#include "common/types.h"
#include "motion/netcam.h"
#include "motion/video.h"
#include "motion/conf.h"
#include "motion/motion.h"
#include "motion/video_conv.h"
#include "motion/rotate.h"

#define MMAP_BUFFERS        4
#define MIN_MMAP_BUFFERS    2

#define ZC301_V4L2_CID_DAC_MAGN         V4L2_CID_PRIVATE_BASE
#define ZC301_V4L2_CID_GREEN_BALANCE    (V4L2_CID_PRIVATE_BASE+1)

static const u32 queried_ctrls[] = {
    V4L2_CID_BRIGHTNESS,
    V4L2_CID_CONTRAST,
    V4L2_CID_SATURATION,
    V4L2_CID_HUE,

    V4L2_CID_RED_BALANCE,
    V4L2_CID_BLUE_BALANCE,
    V4L2_CID_GAMMA,
    V4L2_CID_EXPOSURE,
    V4L2_CID_AUTOGAIN,
    V4L2_CID_GAIN,

    ZC301_V4L2_CID_DAC_MAGN,
    ZC301_V4L2_CID_GREEN_BALANCE,
    0
};

typedef struct {
    int fd;
    char map;
    u32 fps;

    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;

    netcam_buff *buffers;

    s32 pframe;

    u32 ctrl_flags;
    struct v4l2_queryctrl *controls;

} src_v4l2_t;


static int xioctl(int fd, int request, void *arg)
{
    int r;

    do
        r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

/* Constants used by auto brightness feature
 * Defined as constant to make it easier for people to tweak code for a
 * difficult camera.
 * The experience gained from people could help improving the feature without
 * adding too many new options.
 * AUTOBRIGHT_HYSTERESIS sets the minimum the light intensity must change before
 * we adjust brigtness.
 * AUTOBRIGHTS_DAMPER damps the speed with which we adjust the brightness
 * When the brightness changes a lot we step in large steps and as we approach the
 * target value we slow down to avoid overshoot and oscillations. If the camera
 * adjusts too slowly decrease the DAMPER value. If the camera oscillates try
 * increasing the DAMPER value. DAMPER must be minimum 1.
 * MAX and MIN are the max and min values of brightness setting we will send to
 * the camera device.
 */
static int vid_do_autobright(struct context *tsk, struct video_dev *viddev)
{

    int brightness_window_high;
    int brightness_window_low;
    int brightness_target;
    int i, j = 0, avg = 0, step = 0;
    unsigned char *image = tsk->imgs.image_virgin; /* Or tsk->current_image ? */

    int make_change = 0;

    if (tsk->conf.brightness)
        brightness_target = tsk->conf.brightness;
    else
        brightness_target = 128;

    brightness_window_high = MIN2(brightness_target + AUTOBRIGHT_HYSTERESIS, 255);
    brightness_window_low = MAX2(brightness_target - AUTOBRIGHT_HYSTERESIS, 1);

    for (i = 0; i < tsk->imgs.motionsize; i += 101) {
        avg += image[i];
        j++;
    }
    avg = avg / j;

    /* average is above window - turn down brightness - go for the target */
    if (avg > brightness_window_high) {
        step = MIN2((avg - brightness_target) / AUTOBRIGHT_DAMPER + 1, viddev->brightness - AUTOBRIGHT_MIN);
        if (viddev->brightness > step + 1 - AUTOBRIGHT_MIN) {
            viddev->brightness -= step;
            make_change = 1;
        }
    } else if (avg < brightness_window_low) {
        /* average is below window - turn up brightness - go for the target */
        step = MIN2((brightness_target - avg) / AUTOBRIGHT_DAMPER + 1, AUTOBRIGHT_MAX - viddev->brightness);
        if (viddev->brightness < AUTOBRIGHT_MAX - step) {
            viddev->brightness += step;
            make_change = 1;
        }
    }

    return make_change;
}

static int v4l2_set_control(src_v4l2_t * s, u32 cid, int value)
{
    int i, count;

    if (!s->controls)
        return -1;

    for (i = 0, count = 0; queried_ctrls[i]; i++) {
        if (s->ctrl_flags & (1 << i)) {
            if (cid == queried_ctrls[i]) {
                struct v4l2_queryctrl *ctrl = s->controls + count;
                struct v4l2_control control;
                int ret;

                memset (&control, 0, sizeof (control));
                control.id = queried_ctrls[i];

                switch (ctrl->type) {
                case V4L2_CTRL_TYPE_INTEGER:
                    value = control.value =
                            (value * (ctrl->maximum - ctrl->minimum) / 256) + ctrl->minimum;
                    ret = xioctl(s->fd, VIDIOC_S_CTRL, &control);
                    break;

                case V4L2_CTRL_TYPE_BOOLEAN:
                    value = control.value = value ? 1 : 0;
                    ret = xioctl(s->fd, VIDIOC_S_CTRL, &control);
                    break;

                default:
                    debug_log(LOG_ERR, 0, "%s: control type not supported yet");
                    return -1;
                }

                if (debug_level > CAMERA_VIDEO)
                    debug_log(LOG_INFO, 0, "setting control \"%s\" to %d (ret %d %s) %s", ctrl->name,
                               value, ret, ret ? strerror(errno) : "",
                               ctrl->flags & V4L2_CTRL_FLAG_DISABLED ? "Control is DISABLED!" : "");

                return 0;
            }
            count++;
        }
    }

    return -1;
}


static void v4l2_picture_controls(struct context *tsk, struct video_dev *viddev)
{
    src_v4l2_t *s = (src_v4l2_t *) viddev->v4l2_private;

    if (tsk->conf.contrast && tsk->conf.contrast != viddev->contrast) {
        viddev->contrast = tsk->conf.contrast;
        v4l2_set_control(s, V4L2_CID_CONTRAST, viddev->contrast);
    }

    if (tsk->conf.saturation && tsk->conf.saturation != viddev->saturation) {
        viddev->saturation = tsk->conf.saturation;
        v4l2_set_control(s, V4L2_CID_SATURATION, viddev->saturation);
    }

    if (tsk->conf.hue && tsk->conf.hue != viddev->hue) {
        viddev->hue = tsk->conf.hue;
        v4l2_set_control(s, V4L2_CID_HUE, viddev->hue);
    }

    if (tsk->conf.autobright) {
        if (vid_do_autobright(tsk, viddev)) {
            if (v4l2_set_control(s, V4L2_CID_BRIGHTNESS, viddev->brightness))
                v4l2_set_control(s, V4L2_CID_GAIN, viddev->brightness);
        }
    } else {
        if (tsk->conf.brightness && tsk->conf.brightness != viddev->brightness) {
            viddev->brightness = tsk->conf.brightness;
            if (v4l2_set_control(s, V4L2_CID_BRIGHTNESS, viddev->brightness))
                v4l2_set_control(s, V4L2_CID_GAIN, viddev->brightness);
        }
    }

}


static int v4l2_get_capability(src_v4l2_t * s)
{
    if (xioctl(s->fd, VIDIOC_QUERYCAP, &s->cap) < 0) {
        debug_log(LOG_ERR, 0, "Not a V4L2 device?");
        return -1;
    }

    debug_log(LOG_INFO, 0, "cap.driver: \"%s\"", s->cap.driver);
    debug_log(LOG_INFO, 0, "cap.card: \"%s\"", s->cap.card);
    debug_log(LOG_INFO, 0, "cap.bus_info: \"%s\"", s->cap.bus_info);
    debug_log(LOG_INFO, 0, "cap.capabilities=0x%08X", s->cap.capabilities);

    if (s->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        debug_log(LOG_INFO, 0, "- VIDEO_CAPTURE");

    if (s->cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)
        debug_log(LOG_INFO, 0, "- VIDEO_OUTPUT");

    if (s->cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)
        debug_log(LOG_INFO, 0, "- VIDEO_OVERLAY");

    if (s->cap.capabilities & V4L2_CAP_VBI_CAPTURE)
        debug_log(LOG_INFO, 0, "- VBI_CAPTURE");

    if (s->cap.capabilities & V4L2_CAP_VBI_OUTPUT)
        debug_log(LOG_INFO, 0, "- VBI_OUTPUT");

    if (s->cap.capabilities & V4L2_CAP_RDS_CAPTURE)
        debug_log(LOG_INFO, 0, "- RDS_CAPTURE");

    if (s->cap.capabilities & V4L2_CAP_TUNER)
        debug_log(LOG_INFO, 0, "- TUNER");

    if (s->cap.capabilities & V4L2_CAP_AUDIO)
        debug_log(LOG_INFO, 0, "- AUDIO");

    if (s->cap.capabilities & V4L2_CAP_READWRITE)
        debug_log(LOG_INFO, 0, "- READWRITE");

    if (s->cap.capabilities & V4L2_CAP_ASYNCIO)
        debug_log(LOG_INFO, 0, "- ASYNCIO");

    if (s->cap.capabilities & V4L2_CAP_STREAMING)
        debug_log(LOG_INFO, 0, "- STREAMING");

    if (s->cap.capabilities & V4L2_CAP_TIMEPERFRAME)
        debug_log(LOG_INFO, 0, "- TIMEPERFRAME");

    if (!(s->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        debug_log(LOG_ERR, 0, "Device does not support capturing.");
        return -1;
    }

    return 0;
}

static int v4l2_select_input(src_v4l2_t * s, int in, int norm, unsigned long freq_, int tuner_number ATTRIBUTE_UNUSED)
{
    struct v4l2_input input;
    struct v4l2_standard standard;
    v4l2_std_id std_id;

    if (in == 8)
        in = 0;

    /* Set the input. */
    memset (&input, 0, sizeof (input));
    input.index = in;

    if (xioctl(s->fd, VIDIOC_ENUMINPUT, &input) == -1) {
        debug_log(LOG_ERR, 1, "Unable to query input %d VIDIOC_ENUMINPUT", in);
        return -1;
    }

    if (debug_level > CAMERA_VIDEO)
        debug_log(LOG_INFO, 0, "%s: name = \"%s\", type 0x%08X, status %08x", __FUNCTION__, input.name,
                   input.type, input.status);

    if ((input.type & V4L2_INPUT_TYPE_TUNER) && (debug_level > CAMERA_VIDEO))
        debug_log(LOG_INFO, 0, "- TUNER");

    if ((input.type & V4L2_INPUT_TYPE_CAMERA) && (debug_level > CAMERA_VIDEO))
        debug_log(LOG_INFO, 0, "- CAMERA");

    if (xioctl(s->fd, VIDIOC_S_INPUT, &in) == -1) {
        debug_log(LOG_ERR, 1, "Error selecting input %d VIDIOC_S_INPUT", in);
        return -1;
    }

    /* Set video standard usually webcams doesn't support the ioctl or return V4L2_STD_UNKNOWN */
    if (xioctl(s->fd, VIDIOC_G_STD, &std_id) == -1) {
        if (debug_level > CAMERA_VIDEO)
            debug_log(LOG_INFO, 0, "Device doesn't support VIDIOC_G_STD");
        std_id = 0;    // V4L2_STD_UNKNOWN = 0
    }

    if (std_id) {
        memset(&standard, 0, sizeof(standard));
        standard.index = 0;

        while (xioctl(s->fd, VIDIOC_ENUMSTD, &standard) == 0) {
            if ((standard.id & std_id)  && (debug_level > CAMERA_VIDEO))
                debug_log(LOG_INFO, 0, "- video standard %s", standard.name);

            standard.index++;
        }

        switch (norm) {
        case 1:
            std_id = V4L2_STD_NTSC;
            break;
        case 2:
            std_id = V4L2_STD_SECAM;
            break;
        default:
            std_id = V4L2_STD_PAL;
        }

        if (xioctl(s->fd, VIDIOC_S_STD, &std_id) == -1)
            debug_log(LOG_ERR, 1, "Error selecting standard method %d VIDIOC_S_STD", std_id);

    }

    /* If this input is attached to a tuner, set the frequency. */
    if (input.type & V4L2_INPUT_TYPE_TUNER) {
        struct v4l2_tuner tuner;
        struct v4l2_frequency freq;

        /* Query the tuners capabilities. */

        memset(&tuner, 0, sizeof(struct v4l2_tuner));
        tuner.index = input.tuner;

        if (xioctl(s->fd, VIDIOC_G_TUNER, &tuner) == -1) {
            debug_log(LOG_ERR, 1, "tuner %d VIDIOC_G_TUNER", tuner.index);
            return 0;
        }

        /* Set the frequency. */
        memset(&freq, 0, sizeof(struct v4l2_frequency));
        freq.tuner = input.tuner;
        freq.type = V4L2_TUNER_ANALOG_TV;
        freq.frequency = (freq_ / 1000) * 16;

        if (xioctl(s->fd, VIDIOC_S_FREQUENCY, &freq) == -1) {
            debug_log(LOG_ERR, 1, "freq %lu VIDIOC_S_FREQUENCY", freq.frequency);
            return 0;
        }
    }

    return 0;
}

/* *
 * v4l2_do_set_pix_format
 *
 *          This routine does the actual request to the driver
 *
 * Returns:  0  Ok
 *          -1  Problems setting palette or not supported
 *
 * Our algorithm for setting the picture format for the data which the
 * driver returns to us will be as follows:
 *
 * First, we request that the format be set to whatever is in the config
 * file (which is either the motion default, or a value chosen by the user).
 * If that request is successful, we are finished.
 *
 * If the driver responds that our request is not accepted, we then enumerate
 * the formats which the driver claims to be able to supply.  From this list,
 * we choose whichever format is "most efficient" for motion.  The enumerated
 * list is also printed to the motion log so that the user can consider
 * choosing a different value for the config file.
 *
 * We then request the driver to set the format we have chosen.  That request
 * should never fail, so if it does we log the fact and give up.
 */
static int v4l2_do_set_pix_format(u32 pixformat, src_v4l2_t * s,
				  int *width, int *height)
{
    memset(&s->fmt, 0, sizeof(struct v4l2_format));
    s->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    s->fmt.fmt.pix.width = *width;
    s->fmt.fmt.pix.height = *height;
    s->fmt.fmt.pix.pixelformat = pixformat;
    s->fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (xioctl(s->fd, VIDIOC_TRY_FMT, &s->fmt) != -1 &&
	       s->fmt.fmt.pix.pixelformat == pixformat) {
        debug_log(LOG_INFO, 0, "Test palette %c%c%c%c (%dx%d)",
                pixformat >> 0, pixformat >> 8, pixformat >> 16,
                pixformat >> 24, *width, *height);

        if (s->fmt.fmt.pix.width != (unsigned int) *width ||
	        s->fmt.fmt.pix.height != (unsigned int) *height) {
            debug_log(LOG_INFO, 0, "Adjusting resolution from %ix%i to %ix%i.",
                       *width, *height, s->fmt.fmt.pix.width,
		               s->fmt.fmt.pix.height);
            *width = s->fmt.fmt.pix.width;
            *height = s->fmt.fmt.pix.height;
        }

        if (xioctl(s->fd, VIDIOC_S_FMT, &s->fmt) == -1) {
            debug_log(LOG_ERR, 1, "Error setting pixel format VIDIOC_S_FMT");
            return -1;
        }

        debug_log(LOG_INFO, 0, "Using palette %c%c%c%c (%dx%d) bytesperlines "
                   "%d sizeimage %d colorspace %08x", pixformat >> 0,
                   pixformat >> 8, pixformat >> 16, pixformat >> 24,
                   *width, *height, s->fmt.fmt.pix.bytesperline,
                   s->fmt.fmt.pix.sizeimage, s->fmt.fmt.pix.colorspace);
        return 0;
    }
    return -1;
}

/* This routine is called by the startup code to do the format setting */
static int v4l2_set_pix_format(struct context *tsk, src_v4l2_t * s,
			       int *width, int *height)
{
    struct v4l2_fmtdesc fmt;
    short int v4l2_pal;

    /*
     * Note that this array MUST exactly match the config file list.
     * A higher index means better chance to be used
     */
    static const u32 supported_formats[] = {
        V4L2_PIX_FMT_SN9C10X,
        V4L2_PIX_FMT_SBGGR8,
        V4L2_PIX_FMT_MJPEG,
        V4L2_PIX_FMT_JPEG,
        V4L2_PIX_FMT_RGB24,
        V4L2_PIX_FMT_UYVY,
        V4L2_PIX_FMT_YUYV,
        V4L2_PIX_FMT_YUV422P,
        V4L2_PIX_FMT_YUV420	/* most efficient for motion */
    };

    int array_size = sizeof(supported_formats) / sizeof(supported_formats[0]);
    short int index_format = -1;	/* -1 says not yet chosen */

    /* First we try a shortcut of just setting the config file value */
    if (tsk->conf.v4l2_palette >= 0) {
        char name[5] = {supported_formats[tsk->conf.v4l2_palette] >>  0,
                        supported_formats[tsk->conf.v4l2_palette] >>  8,
                        supported_formats[tsk->conf.v4l2_palette] >>  16,
                        supported_formats[tsk->conf.v4l2_palette] >>  24, 0};

        if (v4l2_do_set_pix_format(supported_formats[tsk->conf.v4l2_palette],
            s, width, height) >= 0)
            return 0;

        debug_log(LOG_INFO, 0, "Config palette index %d (%s) doesn't work.",
		           tsk->conf.v4l2_palette, name);
    }
    /* Well, that didn't work, so we enumerate what the driver can offer */

    memset(&fmt, 0, sizeof(struct v4l2_fmtdesc));
    fmt.index = v4l2_pal = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    debug_log(LOG_INFO, 0, "Supported palettes:");

    while (xioctl(s->fd, VIDIOC_ENUM_FMT, &fmt) != -1) {
        short int i;

        debug_log(LOG_INFO, 0, "%i: %c%c%c%c (%s)", v4l2_pal,
                   fmt.pixelformat >> 0, fmt.pixelformat >> 8,
                   fmt.pixelformat >> 16, fmt.pixelformat >> 24,
                   fmt.description);

        /* adjust index_format if larger value found */
        for (i = index_format + 1; i < array_size; i++)
            if (supported_formats[i] == fmt.pixelformat)
                index_format = i;

        memset(&fmt, 0, sizeof(struct v4l2_fmtdesc));
        fmt.index = ++v4l2_pal;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    }

    if (index_format >= 0) {
        char name[5] = {supported_formats[index_format] >>  0,
                        supported_formats[index_format] >>  8,
                        supported_formats[index_format] >>  16,
                        supported_formats[index_format] >>  24, 0};

        debug_log(LOG_INFO, 0, "Selected palette %s", name);

        if (v4l2_do_set_pix_format(supported_formats[index_format],
            s, width, height) >= 0)
            return 0;
        debug_log(LOG_ERR, 1, "VIDIOC_TRY_FMT failed for format %s", name);
    }

    debug_log(LOG_ERR, 0, "Unable to find a compatible palette format.");
    return -1;
}

static int v4l2_set_mmap(src_v4l2_t * s)
{
    enum v4l2_buf_type type;
    u32 b;

    /* Does the device support streaming? */
    if (!(s->cap.capabilities & V4L2_CAP_STREAMING))
        return -1;

    memset(&s->req, 0, sizeof(struct v4l2_requestbuffers));

    s->req.count = MMAP_BUFFERS;
    s->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    s->req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(s->fd, VIDIOC_REQBUFS, &s->req) == -1) {
        debug_log(LOG_ERR, 1, "Error requesting buffers %d for memory map. VIDIOC_REQBUFS", s->req.count);
        return -1;
    }

    debug_log(LOG_DEBUG, 0, "mmap information:");
    debug_log(LOG_DEBUG, 0, "frames=%d", s->req.count);

    if (s->req.count < MIN_MMAP_BUFFERS) {
        debug_log(LOG_ERR, 0, "Insufficient buffer memory.");
        return -1;
    }

    s->buffers = calloc(s->req.count, sizeof(netcam_buff));
    if (!s->buffers) {
        debug_log(LOG_ERR, 1, "%s: Out of memory.", __FUNCTION__);
        return -1;
    }

    for (b = 0; b < s->req.count; b++) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(struct v4l2_buffer));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = b;

        if (xioctl(s->fd, VIDIOC_QUERYBUF, &buf) == -1) {
            debug_log(LOG_ERR, 0, "Error querying buffer %d VIDIOC_QUERYBUF", b);
            free(s->buffers);
            return -1;
        }

        s->buffers[b].size = buf.length;
        s->buffers[b].ptr = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, s->fd, buf.m.offset);

        if (s->buffers[b].ptr == MAP_FAILED) {
            debug_log(LOG_ERR, 1, "Error mapping buffer %i mmap", b);
            free(s->buffers);
            return -1;
        }

        debug_log(LOG_DEBUG, 0, "%i length=%d, ptr=%p", b, buf.length, s->buffers[b].ptr);
    }

    s->map = -1;

    for (b = 0; b < s->req.count; b++) {
        memset(&s->buf, 0, sizeof(struct v4l2_buffer));

        s->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        s->buf.memory = V4L2_MEMORY_MMAP;
        s->buf.index = b;

        if (xioctl(s->fd, VIDIOC_QBUF, &s->buf) == -1) {
            debug_log(LOG_ERR, 1, "buffer index %d VIDIOC_QBUF", s->buf.index);
            return -1;
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(s->fd, VIDIOC_STREAMON, &type) == -1) {
        debug_log(LOG_ERR, 1, "Error starting stream VIDIOC_STREAMON");
        return -1;
    }

    return 0;
}

static int v4l2_scan_controls(src_v4l2_t * s)
{
    int count, i;
    struct v4l2_queryctrl queryctrl;

    memset(&queryctrl, 0, sizeof(struct v4l2_queryctrl));

    for (i = 0, count = 0; queried_ctrls[i]; i++) {
        queryctrl.id = queried_ctrls[i];
        if (xioctl(s->fd, VIDIOC_QUERYCTRL, &queryctrl))
            continue;

        count++;
        s->ctrl_flags |= 1 << i;
    }

    if (count) {
        struct v4l2_queryctrl *ctrl = s->controls = calloc(count, sizeof(struct v4l2_queryctrl));

        if (!ctrl) {
            debug_log(LOG_ERR, 1, "%s: Insufficient buffer memory.", __FUNCTION__);
            return -1;
        }

        for (i = 0; queried_ctrls[i]; i++) {
            if (s->ctrl_flags & (1 << i)) {
                struct v4l2_control control;

                queryctrl.id = queried_ctrls[i];
                if (xioctl(s->fd, VIDIOC_QUERYCTRL, &queryctrl))
                    continue;

                memcpy(ctrl, &queryctrl, sizeof(struct v4l2_queryctrl));

                debug_log(LOG_INFO, 0, "found control 0x%08x, \"%s\", range %d,%d %s", ctrl->id,
                           ctrl->name, ctrl->minimum, ctrl->maximum,
                           ctrl->flags & V4L2_CTRL_FLAG_DISABLED ? "!DISABLED!" : "");

                memset (&control, 0, sizeof (control));
                control.id = queried_ctrls[i];
                xioctl(s->fd, VIDIOC_G_CTRL, &control);
                debug_log(LOG_INFO, 0, "\t\"%s\", default %d, current %d", ctrl->name,
                           ctrl->default_value, control.value);

                ctrl++;
            }
        }
    }

    return 0;
}


static u8 *v4l2_start(struct context *tsk, struct video_dev *viddev, int width, int height,
              int input, int norm, unsigned long freq, int tuner_number)
{
    src_v4l2_t *s;

    //Allocate memory for the state structure
    if (!(s = calloc(sizeof(src_v4l2_t), 1))) {
        debug_log(LOG_ERR, 1, "%s: Out of memory.", __FUNCTION__);
        goto err;
    }

    viddev->v4l2_private = s;
    s->fd = viddev->fd;
    s->fps = tsk->conf.frame_limit;
    s->pframe = -1;

    if (v4l2_get_capability(s))
        goto err;

    if (v4l2_select_input(s, input, norm, freq, tuner_number))
        goto err;

    if (v4l2_set_pix_format(tsk, s, &width, &height))
        goto err;

    if (v4l2_scan_controls(s))
        goto err;

    if (v4l2_set_mmap(s))
        goto err;

    viddev->size_map = 0;
    viddev->v4l_buffers[0] = NULL;
    viddev->v4l_maxbuffer = 1;
    viddev->v4l_curbuffer = 0;

    return (void *) 1;

err:
    if (s)
        free(s);

    viddev->v4l2_private = NULL;
    viddev->v4l2 = 0;
    return NULL;
}


static int v4l2_next(struct context *tsk, struct video_dev *viddev, unsigned char *map, int width, int height)
{
    sigset_t set, old;
    src_v4l2_t *s = (src_v4l2_t *) viddev->v4l2_private;

    if (viddev->v4l_fmt != VIDEO_PALETTE_YUV420P)
        return V4L_FATAL_ERROR;


    /* Block signals during IOCTL */
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &set, &old);

    if (s->pframe >= 0) {
        if (xioctl(s->fd, VIDIOC_QBUF, &s->buf) == -1) {
            debug_log(LOG_ERR, 1, "%s: VIDIOC_QBUF", __FUNCTION__);
            return -1;
        }
    }

    memset(&s->buf, 0, sizeof(struct v4l2_buffer));

    s->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    s->buf.memory = V4L2_MEMORY_MMAP;

    if (xioctl(s->fd, VIDIOC_DQBUF, &s->buf) == -1) {

        /* some drivers return EIO when there is no signal,
           driver might dequeue an (empty) buffer despite
           returning an error, or even stop capturing.
        */
        if (errno == EIO) {
            s->pframe++;
            if ((u32)s->pframe >= s->req.count) s->pframe = 0;
            s->buf.index = s->pframe;

            debug_log(LOG_ERR, 1, "%s: VIDIOC_DQBUF: EIO (s->pframe %d)", __FUNCTION__, s->pframe);

            return 1;
        }

        debug_log(LOG_ERR, 1, "%s: VIDIOC_DQBUF", __FUNCTION__);

        return -1;
    }

    s->pframe = s->buf.index;
    s->buffers[s->buf.index].used = s->buf.bytesused;
    s->buffers[s->buf.index].content_length = s->buf.bytesused;

    pthread_sigmask(SIG_UNBLOCK, &old, NULL);    /*undo the signal blocking */

    {
        netcam_buff *the_buffer = &s->buffers[s->buf.index];

        switch (s->fmt.fmt.pix.pixelformat) {
        case V4L2_PIX_FMT_RGB24:
            conv_rgb24toyuv420p(map, (unsigned char *) the_buffer->ptr, width, height);
            return 0;

        case V4L2_PIX_FMT_UYVY:
            conv_uyvyto420p(map, (unsigned char *) the_buffer->ptr, (unsigned)width, (unsigned)height);
            return 0;

        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_YUV422P:
            conv_yuv422to420p(map, (unsigned char *) the_buffer->ptr, width, height);
            return 0;


        case V4L2_PIX_FMT_YUV420:   ///this is running
            memcpy(map, the_buffer->ptr, viddev->v4l_bufsize);
            return 0;

        case V4L2_PIX_FMT_JPEG:
        case V4L2_PIX_FMT_MJPEG:
            return mjpegtoyuv420p(map, (unsigned char *) the_buffer->ptr, width, height,
                                   s->buffers[s->buf.index].content_length);
/*
            return 0;
        case V4L2_PIX_FMT_JPEG:
            return conv_jpeg2yuv420(tsk, map, the_buffer, width, height);
*/
        case V4L2_PIX_FMT_SBGGR8:    /* bayer */
            bayer2rgb24(tsk->imgs.common_buffer, (unsigned char *) the_buffer->ptr, width, height);
            conv_rgb24toyuv420p(map, tsk->imgs.common_buffer, width, height);
            return 0;

        case V4L2_PIX_FMT_SN9C10X:
            sonix_decompress(map, (unsigned char *) the_buffer->ptr, width, height);
            bayer2rgb24(tsk->imgs.common_buffer, map, width, height);
            conv_rgb24toyuv420p(map, tsk->imgs.common_buffer, width, height);
            return 0;
        }
    }

    return 1;
}


static void v4l2_set_input(struct context *tsk, struct video_dev *viddev, unsigned char *map, int width, int height,
            struct config *conf)
{
    int i;
    int input = conf->input;
    int norm = conf->norm;
    int skip = conf->roundrobin_skip;
    unsigned long freq = conf->frequency;
    int tuner_number = conf->tuner_number;

    if (input != viddev->input || width != viddev->width || height != viddev->height ||
        freq != viddev->freq || tuner_number != viddev->tuner_number) {

        struct timeval switchTime;

        v4l2_select_input((src_v4l2_t *) viddev->v4l2_private, input, norm, freq, tuner_number);

        gettimeofday(&switchTime, NULL);

        v4l2_picture_controls(tsk, viddev);

        viddev->input = input;
        viddev->width = width;
        viddev->height = height;
        viddev->freq = freq;
        viddev->tuner_number = tuner_number;


        /* Skip all frames captured before switchtime, capture 1 after switchtime */
        {
            src_v4l2_t *s = (src_v4l2_t *) viddev->v4l2_private;
            unsigned int counter = 0;
            if (debug_level > CAMERA_VIDEO)
                debug_log(LOG_DEBUG, 0, "set_input_skip_frame switch_time=%ld:%ld",
                           switchTime.tv_sec, switchTime.tv_usec);

            /* Avoid hang using the number of mmap buffers */
            while(counter < s->req.count) {
                counter++;
                if (v4l2_next(tsk, viddev, map, width, height))
                    break;

                if (s->buf.timestamp.tv_sec > switchTime.tv_sec ||
                   (s->buf.timestamp.tv_sec == switchTime.tv_sec && s->buf.timestamp.tv_usec >
                    switchTime.tv_usec))
                    break;

                if (debug_level > CAMERA_VIDEO)
                    debug_log(LOG_DEBUG, 0, "got frame before switch timestamp=%ld:%ld",
                               s->buf.timestamp.tv_sec, s->buf.timestamp.tv_usec);
            }
        }

        /* skip a few frames if needed */
        for (i = 1; i < skip; i++)
            v4l2_next(tsk, viddev, map, width, height);

    } else {
        /* No round robin - we only adjust picture controls */
        v4l2_picture_controls(tsk, viddev);
    }
}

int video_v4l2_start(struct context *tsk, struct video_dev *dev)
{
    int fd;
    int  input, norm, tuner_number;
    unsigned long frequency;

    input = IN_DEFAULT;
    norm = 0;
    frequency = 0;
    tuner_number = 0;

    fd = open(dev->video_device, O_RDWR);
    if (fd < 0) {
        debug_log(LOG_ERR, 1, "Failed to open video device(%s)", dev->video_device);
        free(dev);
        return -2;
    }

    dev->usage_count = 1;
    dev->fd = fd;
    dev->input = input;
    dev->freq = frequency;
    dev->tuner_number = tuner_number;

    dev->brightness = 0;
    dev->contrast = 0;
    dev->saturation = 0;
    dev->hue = 0;
    dev->owner = -1;
    dev->fps = 0;
    dev->v4l2 = 1;  ///MOTION_V4L2

    if (!v4l2_start(tsk, dev, dev->width, dev->height, input, norm, frequency, tuner_number)) {
        debug_log(LOG_ERR, 1, "Failed to run v4l2_start()");
        return -3;
    }

    if (dev->v4l2 == 0) {
        debug_log(-1, 0, "Using V4L1");
    } else {
        debug_log(-1, 0, "Using V4L2");
    }

    tsk->imgs.type = dev->v4l_fmt;  //VIDEO_PALETTE_YUV420P

    switch (tsk->imgs.type) {
        case VIDEO_PALETTE_GREY:
            tsk->imgs.size = dev->width * dev->height;
            ///tsk->imgs.motionsize = dev->width * dev->height;
            break;
        case VIDEO_PALETTE_YUYV:
        case VIDEO_PALETTE_RGB24:
        case VIDEO_PALETTE_YUV422:
            tsk->imgs.type = VIDEO_PALETTE_YUV420P;

        case VIDEO_PALETTE_YUV420P:
            tsk->imgs.size = (dev->width * dev->height * 3) / 2;
            ///tsk->imgs.motionsize = dev->width * dev->height * 3;
            break;
    }

    dev->next = NULL;
    printf("v4l2_start OK!\n");

    return fd;
}

int video_v4l2_next(struct context *tsk, struct video_dev *dev, unsigned char *map)
{
    int ret = -2;
    struct config *conf = &tsk->conf;
    int width, height;

    //NOTE: Since this is a capture, we need to use capture dimensions
    /**
    width = tsk->rotate_data.cap_width;
    height = tsk->rotate_data.cap_height;
    */
    width = tsk->imgs.width;
    height = tsk->imgs.height;

    if (dev == NULL)
        return V4L_FATAL_ERROR;

    /**
    if (dev->owner != tsk->threadnr) {
        ///pthread_mutex_lock(&dev->mutex);
        dev->owner = tsk->threadnr;
        dev->frames = conf->roundrobin_frames;
        }
    */

    v4l2_set_input(tsk, dev, map, width, height, conf);
    ret = v4l2_next(tsk, dev, map, width, height);

    ///rotate the image as specified
    if (tsk->rotate_data.degrees > 0)
        rotate_map(tsk, map);

    return ret;
}

void video_v4l2_close(struct video_dev *viddev)
{
    src_v4l2_t *s = (src_v4l2_t *) viddev->v4l2_private;
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(s->fd, VIDIOC_STREAMOFF, &type);
    close(s->fd);   //viddev->fd
    s->fd = -1;
}


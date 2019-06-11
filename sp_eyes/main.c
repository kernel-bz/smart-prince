/**
 *	file name:  sp_eyes/main.c
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   Eyes Main for SmartPrince
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>

#include "wiringPi.h"
#include "wiringPiI2C.h"

#include "common/config.h"
#include "common/debug.h"
#include "common/file.h"
#include "common/utils.h"
#include "motion/video.h"
#include "motion/video_conv.h"

#include "devices/serial.h"
#include "devices/motor.h"
#include "devices/intr.h"
#include "devices/i2c.h"

#include "haar/haar.h"
#include "haar/image.h"
#include "haar/nn_cnn.h"

#include "classifier.h"
#include "stb_image.h"

extern uint32_t FaceProcessing; ///ml/examples/classifier.c

void *main_thread_face_collector(void *arg)
{
    char src[80];
    char dst[80];
    struct dirent *entry;
    DIR *dir;
	ImageByte imagev;
	ImageByte *imgpgm = &imagev;
    int ret;
    char *type = (char*)arg;
    int w, h, c, w2, msize;

    pr_info_msg("---------------------- %s Starting -----------------------\n", __FUNCTION__);

    ///read classifier array from file(info.txt and class.txt)
	readTextClassifier();   ///for face detection

	pr_info_msg("Waiting...\n");

    while (1)
    {
        usleep(20000);  //20ms
        if (FaceProcessing) continue;

        dir = opendir (PATH_DATA_COLLECT);
        while ((entry = readdir (dir)) != NULL)
        {
            if (entry->d_type == DT_REG)
            {
                uint8_t *simg, *dimg;

                ///image file load: jpg --> rgb --> yuv --> pgm
                system("clear");
                system ("killall mplayer");

                sprintf(src, "%s/%s", PATH_DATA_COLLECT, entry->d_name);
                simg = stbi_load(src, &w, &h, &c, 3);  ///color=3

                w2 = (w & 1) ? w+1 : w;     ///odd to even
                msize = w2 *h*3 + w2*9;
                dimg = malloc(msize);
                pr_info_msg("malloc(): w=%d, h=%d, c=%d, size=%d\n", w2, h, c, msize);

                if (!simg || !dimg) {
                    pr_err_msg("malloc() error!\n\n");
                    break;
                }

                conv_rgb24toyuv420p(dimg, simg, w2, h);
                pr_debug_msg("conv_rgb24toyuv420p() finished.\n");

                conv_yuv420p_to_gray(dimg, simg, w, h);
                ret = img_to_pgm(simg, w , h, imgpgm);   ///simg --> imgpgm

                ///sprintf(dst, "%s/%s.pgm", PATH_DATA_ML_SAVE, entry->d_name);
                ///writePgm(dst, myimg);

                if (ret == -1) {
                    pr_err_msg("%s(): Error on the img_to_pgm()\n", __FUNCTION__);
                } else {
                    ///uint32_t flag = ML_LEARN  | ML_ANSWER | ML_SAVE;
                    pr_info_msg("%s(): Running...\n", __FUNCTION__);
                    haar_run (imgpgm, PATH_DATA_ML_SAVE, type, ML_SAVE);    ///bug in this function.
                    usleep(10000);  //10ms
                }

                if (RunCfg.save)
                    sprintf(dst, "mv %s %s/%s", src, PATH_DATA_ML_BAK1, entry->d_name);
                else
                    sprintf(dst, "rm %s", src);
                system(dst);
                usleep(20000);  //10ms

                freeImage(imgpgm);
                free(dimg);
                free(simg);
            } //if
        } //while

        closedir(dir);
    } //while
}

static void main_thread_face_collect_run(char *type)
{
    pthread_t  tid1;
    if (pthread_create(&tid1, NULL, main_thread_face_collector, type)) {
        pr_err_msg("can't create main_thread_face_collector()\n");
    }

	pr_info_msg("%s(): thread(%d) started.\n", __FUNCTION__, (int)tid1);
}

static void _main_dev_init (void)
{
    wiringPiSetup();

    wiringPiISR (INTR_IRA0_MAIN, INT_EDGE_FALLING, &isr_main) ;
    wiringPiISR (INTR_IRA1_WIFI, INT_EDGE_FALLING, &isr_wifi) ;
    wiringPiISR (INTR_IRA2_SENSOR, INT_EDGE_FALLING, &isr_sensor) ;
    wiringPiISR (INTR_IRA3_VOICE, INT_EDGE_FALLING, &isr_voice) ;
    wiringPiISR (INTR_IRA4_MOTOR, INT_EDGE_FALLING, &isr_motor) ;

    wiringPiISR (INTR_IRA6_CAMERA, INT_EDGE_FALLING, &isr_camera) ;
    wiringPiISR (INTR_IRA7_POWER, INT_EDGE_FALLING, &isr_power) ;

    pinMode (INTR_IRQ0_MAIN, OUTPUT);
    pinMode (INTR_IRQ1_WIFI, OUTPUT);
    pinMode (INTR_IRQ2_SENSOR, OUTPUT);
    pinMode (INTR_IRQ3_VOICE, OUTPUT);
    pinMode (INTR_IRQ4_MOTOR, OUTPUT);

    pinMode (INTR_IRQ6_CAMERA, OUTPUT);
    pinMode (INTR_IRQ7_POWER, OUTPUT);

    digitalWrite (INTR_IRQ0_MAIN, LOW);
    digitalWrite (INTR_IRQ1_WIFI, LOW);
    digitalWrite (INTR_IRQ2_SENSOR, LOW);
    digitalWrite (INTR_IRQ3_VOICE, LOW);
    digitalWrite (INTR_IRQ4_MOTOR, LOW);

    digitalWrite (INTR_IRQ6_CAMERA, LOW);
    digitalWrite (INTR_IRQ7_POWER, LOW);

    i2c_init();
    serial_open (DEV_SERIAL_NODE, 0);
}

static void _main_dev_close (void)
{
    serial_close();
    i2c_close();
}

static int _main_run(void)
{
    char datfile[40];   ///data config file name
    char cfgfile[40];   ///config file name
    char wtsfile[40];   ///weights file name
    unsigned int run_mode = 1 << RunCfg.mode;

    sprintf(datfile, "../%s/%s.dat", RunCfg.target, RunCfg.type);
    sprintf(cfgfile, "../%s/%s.cfg", RunCfg.target, RunCfg.type);
    sprintf(wtsfile, "../%s/%s.wts", RunCfg.target, RunCfg.type);

    if (run_mode & CONFIG_MODE_RUN)
    {
        #if 0
        pthread_t  tid;
        if (pthread_create(&tid, NULL, i2c_thread_loop, NULL)) {
            pr_err_msg("Can't create i2c_thread_loop()\n");
            return;
        }
        #endif

        motion_thread_start(VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_DEVICE, 0);   ///gray
        classifier_thread_run(datfile , cfgfile, wtsfile);

    } else  if ((run_mode & CONFIG_MODE_COLL) && !strcmp(RunCfg.target, "faces")) {
        main_thread_face_collect_run(RunCfg.type);
        classifier_thread_run(datfile , cfgfile, wtsfile);

    } else  if (run_mode & CONFIG_MODE_TRAIN) {
        classifier_train_run(datfile, cfgfile, wtsfile);
        return CONFIG_MODE_TRAIN;

    } else  if (run_mode & CONFIG_MODE_VALID) {
        classifier_validate_run(datfile, cfgfile, wtsfile);
        return CONFIG_MODE_VALID;

    } else  if (run_mode & CONFIG_MODE_CAP) {
        motion_thread_start(VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_DEVICE, 1);   ///color
        return 0;

    } else  if (run_mode & CONFIG_MODE_STOP) {
        pthread_t  tid;
        if (pthread_create(&tid, NULL, i2c_thread_loop, NULL)) {
            pr_err_msg("Can't create i2c_thread_loop()\n");
            return -CONFIG_MODE_STOP;
        }
        while (I2CControl.status != I2C_STATUS_CHECK) usleep(20000); ///20ms
        i2c_send_motor(MD_STOP_ALL);
        return CONFIG_MODE_STOP;

    } else  {
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (util_stack_size()) exit(-1);

    ///_main_dev_init();
    file_config_load();     ///RunCfg
    if (argc > 1) {
        if (!strcmp(argv[1], "run")) RunCfg.mode = 0;
        else if (!strcmp(argv[1], "train")) RunCfg.mode = 2;
        else if (!strcmp(argv[1], "valid")) RunCfg.mode = 3;
        else if (!strcmp(argv[1], "cap")) RunCfg.mode = 5;
        else RunCfg.mode = 1;   ///"coll"
    }
    file_config_display();

    int ret = _main_run();
    if (ret) goto _end;

    while(1) pause();

_end:
    ///_main_dev_close();
    file_config_display();
    return ret;
}

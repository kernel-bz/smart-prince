/**
 *	file name:  include/common/file.h
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   File I/O Module
 */

#ifndef __FILE_H
#define __FILE_H

#include "motion/motion.h"

#define PATH_TRAIN              "faces/train"

#define PATH_DATA_COLLECT       "/home/pi/nfs/ml/collect"

#define PATH_DATA_ML_SAVE       "/home/pi/nfs/ml/save"
///classifier_thread_predict()
///classifier_thread_predict_run()
///if (ans > 60) move to bak0 with ans_label
///else move to predict

#define PATH_DATA_ML_PREDICT    "/home/pi/nfs/ml/predict"
///_motion_rename_picture()
///rename with motor action
///move to train

#define PATH_DATA_ML_TRAIN      "/home/pi/nfs/ml/train"
///opendir (PATH_DATA_ML_TRAIN)
///classifier_thread_train_run()
///if (loss < 0.01) move to bak0
///else move to bak1

#define PATH_DATA_ML_BAK0       "/home/pi/nfs/ml/bak0" ///train OK
#define PATH_DATA_ML_BAK1       "/home/pi/nfs/ml/bak1" ///train retry

#define PATH_FACE_VOICES        "../data/voices/"

int create_path(const char *path);
FILE * myfopen(const char *path, const char *mode);
size_t mystrftime(struct context *cnt, char *s, size_t max, const char *userformat,
                  const struct tm *tm, const char *filename, int sqltype);
void file_config_load(void);
void file_config_display(void);

#endif

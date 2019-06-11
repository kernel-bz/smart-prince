/**
 *  file name:  nn_cnn.h
 *  function:   Neural Network Header
 *  author:     JungJaeJoon(rgbi3307@nate.com) on the www.kernel.bz
 */

#ifndef __NN_CNN_H
#define __NN_CNN_H

///#define CNN_COLORS        3
#define CNN_COLORS           1      //GRAY
///#define CNN_WIDTH          320
///#define CNN_HEIGHT         320
#define CNN_WIDTH         40
#define CNN_HEIGHT        40

#define NUM_INPUTS      CNN_WIDTH * CNN_HEIGHT * CNN_COLORS
#define NUM_HIDDEN      NUM_INPUTS / 20
#define NUM_OUTPUTS     50

#define ML_RATE         0.001

///learing flag
#define ML_CLEAR        0x00000000
#define ML_LOAD         0x00000001
#define ML_SAVE         0x00000002
#define ML_LEARN        0x00000004
#define ML_ANSWER       0x00000008

#define ML_WEIGHT_FILE  "nn_cnn.wb"

typedef struct _learn_data {
    int ans;            ///answer
    uint32_t count;     ///learning count
    float acp;            ///accuracy for answer
    int pid;                ///sp_server process id
} LearnParam_T;

LearnParam_T LearnParam;

///Weights
float Wxh[NUM_INPUTS+1][NUM_HIDDEN];
float Why[NUM_HIDDEN+1][NUM_OUTPUTS];

void nn_cnn_init(uint32_t flag);
float nn_cnn_running (float *xdata, int ydata, int isize, float rate);
float nn_cnn_question(float *xdata, int isize, int *ans);
void nn_cnn_write(char *fname);

#endif

//
//	file name: motor.h
//	comments: Motor Control Module
//	author: Jung,JaeJoon(rgbi3307@nate.com) at the www.kernel.bz
//

#ifndef _MOTOR_H
#define _MOTOR_H

#include "common/types.h"

///Motor Direction
#define MD_WAIT     -1
#define MD_STOP     0
#define MD_GO       1   ///0001b
#define MD_BACK     2   ///0010b
#define MD_BG       MD_BACK  | MD_GO    ///0011b (uturn)
#define MD_LEFT     4   ///0100b
#define MD_LG       MD_LEFT  | MD_GO    ///0101b
#define MD_LB       MD_LEFT  | MD_BACK  ///0110b
#define MD_RIGHT    8   ///1000b
#define MD_RG       MD_RIGHT | MD_GO    ///1001b
#define MD_RB       MD_RIGHT | MD_BACK  ///1010b

#define MD_LBG      MD_LB    | MD_GO    ///0111b
#define MD_RBG      MD_RB    | MD_GO    ///1011b
#define MD_RL       MD_RIGHT | MD_LEFT  ///1100b
#define MD_RLG      MD_RL    | MD_GO    ///1101b
#define MD_RLB      MD_RL    | MD_BACK  ///1110b
#define MD_RLGB     MD_RLG   | MD_BACK  ///1111b

#define MD_CNT      10
#define MD_UNKNOWN  20
#define MD_STOP_ALL 30

#define MD_UTURN    MD_BG
#define MD_UP       MD_BACK
#define MD_DOWN     MD_GO

#define MC_IDX_0    0 << 5  ///action
#define MC_IDX_1    1 << 5  ///camera pan/tilt
#define MC_IDX_2    2 << 5
#define MC_IDX_3    3 << 5

#define MI1_LEFT    1     //Left
#define MI2_RIGHT   2     //Right
#define MI3_UPPER   3     //Camera Upper
#define MI4_BOTTOM  4     //Camera Bottom
#define MI5_ALL     5

#define MD_CW       0
#define MD_CCW      1

#define MC_DATA_CNT 3

typedef struct  {
    int act;
    int dir;                            ///motor direction index
    u32 cnt;                            ///saved file count
    u8 dat[MC_DATA_CNT];
    u8 fname[60];                    ///saved file name
    u32 mode;                          ///operation mode
} MotorControl_T;

extern MotorControl_T MotorControl;


#endif


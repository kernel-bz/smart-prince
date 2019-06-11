/**
 *	file name:  include/common/config.h
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   Config Module
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CONFIG_ML_FACE_DETECTION

//#define CONFIG_DEVICE_MAIN
//#define CONFIG_DEVICE_WIFI
#define CONFIG_DEVICE_SENSOR
#define CONFIG_DEVICE_VOICE
#define CONFIG_DEVICE_MOTOR
//#define CONFIG_DEVICE_CM
//#define CONFIG_DEVICE_CAMERA
#define CONFIG_DEVICE_POWER

#define CONFIG_FNAME        "../data/config.txt"
typedef struct Config
{
    unsigned int    who;                    ///0:client/1:server
    char                   name[8];
    unsigned int    id;
    char                   net[16];
    char                   ip[16];
    unsigned int    port;
    unsigned int    start;                  ///0, 1
    unsigned int    step;                   ///4
    unsigned int    ncnt;                   ///< n

    unsigned int    mode;                   ///0:run/1:coll/2:train/3:valid/4:stop
    char                   target[8];           ///faces/actions/...
    char                   type[8];             ///class/emotion/person/race..
    unsigned int    opcode;             ///0:me/1:you/2:we/3:train/...
    unsigned int    mplay;
    unsigned int    save;
    unsigned int    acc;                ///accuracy:##.##
} RunConfig;

RunConfig RunCfg;
extern RunConfig RunCfg;

#define CONFIG_MODE_RUN     0x00000001
#define CONFIG_MODE_COLL    0x00000002
#define CONFIG_MODE_TRAIN   0x00000004
#define CONFIG_MODE_VALID   0x00000008
#define CONFIG_MODE_STOP    0x00000010
#define CONFIG_MODE_CAP     0x00000020  ///Capture

#define CONFIG_WHO_SHIFT    8
#define CONFIG_WHO_LOCAL    0x00000100
#define CONFIG_WHO_SERVER   0x00000200

#define CONFIG_OP_SHIFT     16
#define CONFIG_OP_ME        0x00010000
#define CONFIG_OP_YOU       0x00020000
#define CONFIG_OP_WE        CONFIG_OP_ME | CONFIG_OP_YOU
#define CONFIG_OP_TRAIN     0x00040000


#define CONFIG_TRAIN_AMOUNT         50
#define CONFIG_TRAIN_FINISH_COST    0.01


#endif

/*
 *  file name:  i2c.h
 *	comments:   I2C Module for SmartPrince
 *  author:     JungJaeJoon(rgbi3307@nate.com)
 *  Copyright (c) www.kernel.bz
 *  This is under the GPL License (see <http://www.gnu.org/licenses/>)
 */

#ifndef __I2C_H
#define __I2C_H

#include "common/types.h"

//I2C Address
#define I2C_ADDR_MAIN           0x10
#define I2C_ADDR_WIFI           0x11
#define I2C_ADDR_SENSOR         0x12
#define I2C_ADDR_VOICE          0x13
#define I2C_ADDR_MOTOR          0x14
#define I2C_ADDR_CM             0x15
#define I2C_ADDR_CAMERA         0x16
#define I2C_ADDR_POWER          0x17

#define I2C_ADDR_RAPI           0x18
#define I2C_ADDR_RAPI_CM        0x19
#define I2C_ADDR_RAPI_W         0x1a
#define I2C_ADDR_SMARTHOME      0x1b

//I2C Status
#define I2C_STATUS_CHECK        0xF8
#define I2C_STATUS_BUSY         0xF9
#define I2C_STATUS_OK           0xFA
#define I2C_STATUS_READY        0xFB
#define I2C_STATUS_CTL          0xFC
#define I2C_STATUS_NONE         0xFD
#define I2C_STATUS_ERROR        0xFE

#define I2C_CMD_MAIN            0x80    ///, 0x88
#define I2C_CMD_WIFI            0x90    ///, 0x98
#define I2C_CMD_SENSOR          0xA0    ///, 0xA8
#define I2C_CMD_VOICE           0xB0    ///, 0xB8
#define I2C_CMD_MOTOR           0xC0    ///, 0xC8
#define I2C_CMD_CM              0xD0    ///, 0xD8
#define I2C_CMD_CAMERA          0xE0    ///, 0xE8
#define I2C_CMD_POWER           0xF0    ///, 0xF8
#define I2C_CMD_MASK            0xF8

#define I2C_DAT_MOTOR_STOP      0x00
#define I2C_DAT_MOTOR_ACK       0x07

#define I2C_DAT_CNT_MOTOR       3

#define I2C_BUF_SIZE            16

typedef struct  {
    int fd_main;
    int fd_wifi;
    int fd_sensor;
    int fd_voice;
    int fd_motor;
    int fd_cm;
    int fd_camera;
    int fd_power;
    int irq[I2C_BUF_SIZE];      ///head, tail
    int rfd[I2C_BUF_SIZE];
    int ira[I2C_BUF_SIZE];      ///anx
    int fda[I2C_BUF_SIZE];
    u8 seq[I2C_BUF_SIZE];
    u8 ans[I2C_BUF_SIZE];       ///answer data
    u32 dev;                                ///command device
    u32 chk;                                ///check device
    u32 ack;
    u32 anx;
    u32 head;
    u32 tail;
    u8 status;
} I2CControl_T;

extern I2CControl_T I2CControl;

void i2c_init (void);
void i2c_close (void);
void i2c_send (int irq, int fd, u8 data);
void i2c_send_motor(int dir);

int i2c_device_check(void);

void* i2c_thread_loop(void* arg);

#endif

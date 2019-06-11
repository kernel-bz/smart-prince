/**
 *  file name:  i2c.c
 *	  comments:   i2c Module for SmartPrince
 *  author:     JungJaeJoon(rgbi3307@nate.com)
 *  Copyright (c) www.kernel.bz
 *  This is under the GPL License (see <http://www.gnu.org/licenses/>)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "wiringPi.h"
#include "wiringPiI2C.h"

#include "common/config.h"
#include "common/debug.h"
#include "common/file.h"
#include "devices/i2c.h"
#include "devices/motor.h"
#include "devices/intr.h"
#include "motion/motion.h"

I2CControl_T I2CControl = {
    .dev = 0,
    .chk = 0,
    .ack = 0,
    .anx = 0,
    .head = 0,
    .tail = 0,
    .status = I2C_STATUS_NONE
};

void i2c_init (void)
{
#ifdef CONFIG_DEVICE_POWER
    I2CControl.fd_power = wiringPiI2CSetup (I2C_ADDR_POWER);
    if (I2CControl.fd_power < 0) {
        pr_err_msg("I2C Open Error(fd_power=%d)\n", I2CControl.fd_power);
    } else {
        pr_info_msg("I2C Open Succeeded(fd_power=%d)\n", I2CControl.fd_power);
    }
#endif

#ifdef CONFIG_DEVICE_MOTOR
    I2CControl.fd_motor = wiringPiI2CSetup (I2C_ADDR_MOTOR);
    if (I2CControl.fd_motor < 0) {
        pr_err_msg("I2C Open Error(fd_motor=%d)\n", I2CControl.fd_motor);
    } else {
        pr_info_msg("I2C Open Succeeded(fd_motor=%d)\n", I2CControl.fd_motor);
    }
#endif

#ifdef CONFIG_DEVICE_SENSOR
    I2CControl.fd_sensor = wiringPiI2CSetup (I2C_ADDR_SENSOR);
    if (I2CControl.fd_sensor < 0) {
        pr_err_msg("I2C Open Error(fd_sensor=%d)\n", I2CControl.fd_sensor);
    } else {
        pr_info_msg("I2C Open Succeeded(fd_sensor=%d)\n", I2CControl.fd_sensor);
    }
#endif

#ifdef CONFIG_DEVICE_VOICE
    I2CControl.fd_voice = wiringPiI2CSetup (I2C_ADDR_VOICE);
    if (I2CControl.fd_voice < 0) {
        pr_err_msg("I2C Open Error(fd_voice=%d)\n", I2CControl.fd_voice);
    } else {
        pr_info_msg("I2C Open Succeeded(fd_voice=%d)\n", I2CControl.fd_voice);
    }
#endif
}

void i2c_close (void)
{
    close(I2CControl.fd_sensor);
    close(I2CControl.fd_voice);
    close(I2CControl.fd_motor);
    close(I2CControl.fd_power);
}

static void _i2c_irq(int irq)
{
    ///Interrupt to Slave(IRQ)
     digitalWrite (irq, HIGH);
    I2CControl.status = I2C_STATUS_READY;
    digitalWrite (irq, LOW);
    I2CControl.status = I2C_STATUS_OK;
}

void i2c_send (int irq, int fd, u8 data)
{
    int ret;

    ///Request IRQ for I2C Sending
    _i2c_irq(irq);

    usleep(20000); ///10ms (must be need)

    ret = wiringPiI2CWrite (fd, data);
    if  (ret < 0) {
        pr_debug_msg("Error: Send[%02X] fd<%d> ret<%d>\n", data, fd, ret);
        I2CControl.status = I2C_STATUS_ERROR;
    }
}

int i2c_device_check(void)
{
    int cnt=0;

    if (I2CControl.fd_power > 2 &&
            !((I2CControl.chk >> DEVICE_POWER) & 1)) {
        ///devices reset
        digitalWrite (INTR_IRQ7_POWER, HIGH);
        pr_warn_msg("I2C DEVICE_POWER Check.\n");
        usleep(1000); ///1ms
        cnt++;
        digitalWrite (INTR_IRQ7_POWER, LOW);
    }

    if (I2CControl.fd_motor > 2 &&
            !((I2CControl.chk >> DEVICE_MOTOR) & 1)) {
        ///devices reset
        digitalWrite (INTR_IRQ4_MOTOR, HIGH);
        pr_warn_msg("I2C DEVICE_MOTOR Check.\n");
        usleep(1000); ///1ms
        cnt++;
        digitalWrite (INTR_IRQ4_MOTOR, LOW);
    }

    if (I2CControl.fd_sensor > 2 &&
            !((I2CControl.chk >> DEVICE_SENSOR) & 1)) {
        ///devices reset
        digitalWrite (INTR_IRQ2_SENSOR, HIGH);
        pr_warn_msg("I2C DEVICE_SENSOR Check.\n");
        usleep(1000); ///1ms
        cnt++;
        digitalWrite (INTR_IRQ2_SENSOR, LOW);
    }

    if (I2CControl.fd_voice > 2 &&
            !((I2CControl.chk >> DEVICE_VOICE) & 1)) {
        ///devices reset
        digitalWrite (INTR_IRQ3_VOICE, HIGH);
        pr_warn_msg("I2C DEVICE_VOICE Check.\n");
        usleep(1000); ///1ms
        cnt++;
        digitalWrite (INTR_IRQ3_VOICE, LOW);
    }
    return cnt;
}

///motor: motor selection(0 to 3)
///dir: motor direction(0 to 15), action function
///speed: motor speed(0 to 7)
///duration: motor duration(interger 0 to 31)
static void _i2c_send_motor(u8 motor, u8 dir, u8 speed, u8 duration)
{
    i2c_send(INTR_IRQ4_MOTOR, I2CControl.fd_motor, I2C_CMD_MOTOR | I2C_DAT_CNT_MOTOR);

    while (!((I2CControl.ack >> DEVICE_MOTOR)&1)) usleep(10000);   ///wait for ack

    I2CControl.ack ^= (1 << DEVICE_MOTOR);  ///clear
    i2c_send(INTR_IRQ4_MOTOR, I2CControl.fd_motor, 0x00 | dir<<3 | speed);

    while (!((I2CControl.ack >> DEVICE_MOTOR)&1)) usleep(10000);   ///wait for ack

    I2CControl.ack ^= (1 << DEVICE_MOTOR);  ///clear
    i2c_send(INTR_IRQ4_MOTOR, I2CControl.fd_motor, 0x00 | motor<<5 | duration);

    while (!((I2CControl.ack >> DEVICE_MOTOR)&1)) usleep(10000);   ///wait for ack
    I2CControl.ack ^= (1 << DEVICE_MOTOR);  ///clear

    pr_debug_msg("Send: motor[%02X] dir[%02X] speed[%02X] duration[%02X]\n"
                , motor, dir, speed, duration);
}

void i2c_send_motor(int dir)
{
    u8 speed, duration;

    speed = 6;
    duration = 20;

    if (dir == MD_STOP_ALL) {
        dir = MD_STOP;
        speed = 0x07;           ///7
        duration = 0x1F;    ///31
    } else if (dir < MD_STOP || dir >= MD_CNT) {
        dir = MD_STOP;
    }

    ///pr_info_msg("%s(): dir=%d\n", __FUNCTION__, dir);

    _i2c_send_motor(MC_IDX_0, dir, speed, duration);
}


///Move from PREDICT to TRAIN
static void _i2c_data_rename_picture(u8 dir, u8 speed, u8 duration)
{
    char *maction[] = {
          "_STOP_", "_GO_", "_BACK_", "_BG_", "_LEFT_", "_LG_", "_LB_", "_RIGHT_", "_RG_", "_RB_"
        , "_" };
    char src[160], dst[160];

    sprintf(src, "%s/%s", PATH_DATA_ML_PREDICT, MotorControl.fname);
    sprintf(dst, "%s/%s%02d_%02d_%s", PATH_DATA_ML_TRAIN
        , maction[dir], speed, duration, MotorControl.fname);

    if (rename(src, dst) >= 0) {
        unlink(src);
        pr_info_msg("Rename to Train.\n");
    } else {
        pr_err_msg("Rename to Train Error.\n");
        unlink(src);
    }
}

static void _i2c_data_parse_motor(int irq, u8 data)
{
    u8 dat;
    static u8 idx=0;

    if ((data & I2C_CMD_MASK) == I2C_CMD_MOTOR) {
        idx = 0;
        dat = data & 0x07;
        ///pr_debug_msg("Recv: cmd[%02X]\n", data);
        if (dat == 0x00) {
            I2CControl.dev |= (1 << DEVICE_MOTOR);     ///command device
            pr_info_msg("Set Device Motor Stop.\n");
        } else if (dat == 0x07) {
            I2CControl.ack |= (1 << DEVICE_MOTOR);
        } else {
            MotorControl.dat[idx] = dat;    ///cnt
            _i2c_irq(irq);  ///ACK
        }
    } else {
        idx++;
        MotorControl.dat[idx] = data;
        _i2c_irq(irq);  ///ACK
    }

    ///Recv Data
    if (MotorControl.dat[0]  == idx+1) {
         u8 motor, dir, speed, duration;
         dat = MotorControl.dat[1];
         speed = dat  & 0x07;
        //dir = (dat >> 3) & 0x0F;
        dir = dat >> 3;
        dat = MotorControl.dat[2];
        duration = dat & 0x1F;
        //motor = (dat >> 5) & 0x03;
        motor = dat >> 5;
        pr_debug_msg("Recv: motor[%02X] dir[%02X] speed[%02X] duration[%02X]\n"
                , motor, dir, speed, duration);

        if (dir >= MD_STOP &&  dir <= MD_CNT) {
            _i2c_data_rename_picture(dir, speed, duration); ///Move from PREDICT to TRAIN
            ///MotorControl.mode = CONFIG_RUN_MODE;
        }
    }
}

static void _i2c_data_parsing(u32 idx, u8 data)
{
    u8 cmd = data & I2C_CMD_MASK;

    if (cmd == I2C_STATUS_CHECK) {
        I2CControl.anx++;
        if (I2CControl.anx >= I2C_BUF_SIZE) {
            pr_err_msg("Exceed I2C_BUF_SIZE, anx<%d>\n", I2CControl.anx);
            I2CControl.anx = I2C_BUF_SIZE;
        }
        I2CControl.ira[I2CControl.anx] = I2CControl.irq[idx];
        I2CControl.fda[I2CControl.anx] = I2CControl.rfd[idx];
        I2CControl.ans[I2CControl.anx] = I2C_STATUS_OK;

        I2CControl.chk |= (1 << I2CControl.seq[idx]);

    } else if (I2CControl.irq[idx] == INTR_IRQ4_MOTOR) {
        _i2c_data_parse_motor(I2CControl.irq[idx] , data);

    } else {
        _i2c_irq(I2CControl.irq[idx]);  ///ACK
    }

}

void* i2c_thread_loop(void* arg)
{
    u8 data;
    u32 i, idx, cnt=0;

     while(1)
    {
        if (I2CControl.head !=  I2CControl.tail) {
            I2CControl.tail++;
            I2CControl.tail %= I2C_BUF_SIZE;
            idx = I2CControl.tail;

            usleep(10000); ///10ms (must be need)
            data = wiringPiI2CRead(I2CControl.rfd[idx]);

             //pr_debug_msg("Recv:tail<%d> fd<%d> recv[%02X] diff<%d>\n"
             //       , idx, I2CControl.rfd[idx], data,  I2CControl.head - idx);

            _i2c_data_parsing(idx, data);

        } else {
            ///Answer
            for (i=1; i<=I2CControl.anx; i++) {
                pr_debug_msg("Answer: anx<%d> fd<%d> send[%02X]\n", i, I2CControl.fda[i], I2CControl.ans[i]);
                i2c_send(I2CControl.ira[i], I2CControl.fda[i], I2CControl.ans[i]);
            }
            I2CControl.anx = 0;

            cnt++;
            if (cnt > 500) {    //5s
                idx = i2c_device_check();
                if (idx==0) I2CControl.status = I2C_STATUS_CHECK;
                cnt = 0;
            }
        }

        usleep(10000); ///10ms
    } //while
}

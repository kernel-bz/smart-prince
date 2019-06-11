/**
 *  file name:  intr.c
 *	  comments:   Interrupt Module for SmartPrince
 *  author:     JungJaeJoon(rgbi3307@nate.com)
 *  Copyright (c) www.kernel.bz
 *  This is under the GPL License (see <http://www.gnu.org/licenses/>)
 */

#include <stdio.h>
#include "common/debug.h"
#include "devices/intr.h"
#include "devices/i2c.h"
#include "devices/motor.h"

///Interrupt Service Routine
void isr_main(void)
{
    u32 idx;
    I2CControl.head++;
    idx = I2CControl.head % I2C_BUF_SIZE;
    if (idx != I2CControl.tail ) {
        I2CControl.rfd[idx] = I2CControl.fd_main;
        I2CControl.irq[idx] = INTR_IRQ0_MAIN;
        I2CControl.seq[idx] = DEVICE_MAIN;
        I2CControl.head = idx;
        //pr_debug_msg("MAIN:head<%d> fd<%d> irq<%d>\n", I2CControl.head, I2CControl.fd_main, INTR_IRQ0_MAIN);
    } else {
        I2CControl.head--;
        pr_debug_msg("I2C_STATUS_BUSY.");
        I2CControl.rfd[I2CControl.tail] = I2CControl.fd_main;
        I2CControl.irq[I2CControl.tail] = INTR_IRQ0_MAIN;
    }
}

void isr_wifi(void)
{
    u32 idx;
    I2CControl.head++;
    idx = I2CControl.head % I2C_BUF_SIZE;
    if (idx != I2CControl.tail ) {
        I2CControl.rfd[idx] = I2CControl.fd_wifi;
        I2CControl.irq[idx] = INTR_IRQ1_WIFI;
        I2CControl.seq[idx] = DEVICE_WIFI;
        I2CControl.head = idx;
        //pr_debug_msg("WIFI:head<%d> fd<%d> irq<%d>\n", I2CControl.head, I2CControl.fd_wifi, INTR_IRQ1_WIFI);
    } else {
        I2CControl.head--;
        pr_debug_msg("I2C_STATUS_BUSY.");
        I2CControl.rfd[I2CControl.tail] = I2CControl.fd_wifi;
        I2CControl.irq[I2CControl.tail] = INTR_IRQ1_WIFI;
    }
}

void isr_sensor(void)
{
    u32 idx;
    I2CControl.head++;
    idx = I2CControl.head % I2C_BUF_SIZE;
    if (idx != I2CControl.tail ) {
        I2CControl.rfd[idx] = I2CControl.fd_sensor;
        I2CControl.irq[idx] = INTR_IRQ2_SENSOR;
        I2CControl.seq[idx] = DEVICE_SENSOR;
        I2CControl.head = idx;
        //pr_debug_msg("SENS:head<%d> fd<%d> irq<%d>\n", I2CControl.head, I2CControl.fd_sensor, INTR_IRQ2_SENSOR);
    } else {
        I2CControl.head--;
        pr_debug_msg("I2C_STATUS_BUSY.");
        I2CControl.rfd[I2CControl.tail] = I2CControl.fd_sensor;
        I2CControl.irq[I2CControl.tail] = INTR_IRQ2_SENSOR;
    }
}

void isr_voice(void)
{
    u32 idx;
    I2CControl.head++;
    idx = I2CControl.head % I2C_BUF_SIZE;
    if (idx != I2CControl.tail ) {
        I2CControl.rfd[idx] = I2CControl.fd_voice;
        I2CControl.irq[idx] = INTR_IRQ3_VOICE;
        I2CControl.seq[idx] = DEVICE_VOICE;
        I2CControl.head = idx;
        //pr_debug_msg("VOICE:head<%d> fd<%d> irq<%d>\n", I2CControl.head, I2CControl.fd_voice, INTR_IRQ3_VOICE);
    } else {
        I2CControl.head--;
        pr_debug_msg("I2C_STATUS_BUSY.");
        I2CControl.rfd[I2CControl.tail] = I2CControl.fd_voice;
        I2CControl.irq[I2CControl.tail] = INTR_IRQ3_VOICE;
    }
}

void isr_motor(void)
{
    u32 idx;
    I2CControl.head++;
    idx = I2CControl.head % I2C_BUF_SIZE;
    if (idx != I2CControl.tail ) {
        I2CControl.rfd[idx] = I2CControl.fd_motor;
        I2CControl.irq[idx] = INTR_IRQ4_MOTOR;
        I2CControl.seq[idx] = DEVICE_MOTOR;
        I2CControl.head = idx;
        //pr_debug_msg("MOTOR:head<%d> fd<%d> irq<%d>\n", I2CControl.head, I2CControl.fd_motor, INTR_IRQ4_MOTOR);
    } else {
        I2CControl.head--;
        pr_debug_msg("I2C_STATUS_BUSY.");
        I2CControl.rfd[I2CControl.tail] = I2CControl.fd_motor;
        I2CControl.irq[I2CControl.tail] = INTR_IRQ4_MOTOR;
    }
}

void isr_camera(void)
{
    u32 idx;
    I2CControl.head++;
    idx = I2CControl.head % I2C_BUF_SIZE;
    if (idx != I2CControl.tail ) {
        I2CControl.rfd[idx] = I2CControl.fd_camera;
        I2CControl.irq[idx] = INTR_IRQ6_CAMERA;
        I2CControl.seq[idx] = DEVICE_CAMERA;
        I2CControl.head = idx;
        //pr_debug_msg("CAM:head<%d> fd<%d> irq<%d>\n", I2CControl.head, I2CControl.fd_camera, INTR_IRQ6_CAMERA);
    } else {
        I2CControl.head--;
        pr_debug_msg("I2C_STATUS_BUSY.");
        I2CControl.rfd[I2CControl.tail] = I2CControl.fd_camera;
        I2CControl.irq[I2CControl.tail] = INTR_IRQ6_CAMERA;
    }
}

void isr_power(void)
{
    u32 idx;
    I2CControl.head++;
    idx = I2CControl.head % I2C_BUF_SIZE;
    if (idx != I2CControl.tail ) {
        I2CControl.rfd[idx] = I2CControl.fd_power;
        I2CControl.irq[idx] = INTR_IRQ7_POWER;
        I2CControl.seq[idx] = DEVICE_POWER;
        I2CControl.head = idx;
        //pr_debug_msg("POWER:head<%d> fd<%d> irq<%d>\n", I2CControl.head, I2CControl.fd_power, INTR_IRQ7_POWER);
    } else {
        I2CControl.head--;
        pr_debug_msg("I2C_STATUS_BUSY.");
        I2CControl.rfd[I2CControl.tail] = I2CControl.fd_power;
        I2CControl.irq[I2CControl.tail] = INTR_IRQ7_POWER;
    }
}


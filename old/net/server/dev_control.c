/**
    file name:  sp_server/dev_control.c
    author:     JungJaeJoon(rgbi3307@nate.com)
    copyright:  www.kernel.bz
    comments:   device control module
                This source is licenced under the GPL.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>

#include "wiringPi.h"
#include "dev_control.h"

int dev_lib_setup(void)
{
    int ret;

    ret =  wiringPiSetup ();    ///-1: error
    if (ret < 0) {
        printf("wiringPiSetup error!\n");
    }
    return ret;
}

void dev_gpio_init(void)
{
    ///pinMode (MOTOR_IRQ7, OUTPUT);
    ///pinMode (MOTOR_IRA7, OUTPUT);
    pinMode (MOTOR_IRA7, INPUT);
}

void dev_motor_on(void)
{
    digitalWrite(MOTOR_IRQ7, 0);
    usleep(100);
    digitalWrite(MOTOR_IRQ7, 1);    ///Interrupt IRQ7
    usleep(100);
    digitalWrite(MOTOR_IRQ7, 0);
}

void dev_motor_off(void)
{
    digitalWrite(MOTOR_IRA7, 0);
    usleep(100);
    digitalWrite(MOTOR_IRA7, 1);    ///Interrupt IRA7
    usleep(100);
    digitalWrite(MOTOR_IRA7, 0);
}


/**
    file name:  sp_server/dev_control.h
    author:     JungJaeJoon(rgbi3307@nate.com)
    copyright:  www.kernel.bz
    comments:   device control module
                This source is licenced under the GPL.
 */

 #ifndef __DEV_CONTROL_H
#define __DEV_CONTROL_H

#define	MOTOR_IRQ7      10  ///P24
#define	MOTOR_IRA7      27  ///P36

int dev_lib_setup (void);
void dev_gpio_init(void);

void dev_motor_on(void);
void dev_motor_off(void);

#endif

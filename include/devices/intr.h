//
//	file name: intr.h
//	comments: Interrupt Control Module
//	author: Jung,JaeJoon(rgbi3307@nate.com) at the www.kernel.bz
//

#ifndef __INTR_H
#define __INTR_H

///Device Trigger Index
#define DEVICE_MAIN         0
#define DEVICE_WIFI         1
#define DEVICE_SENSOR       2
#define DEVICE_VOICE        3
#define DEVICE_MOTOR        4
#define DEVICE_CM           5
#define DEVICE_CAMERA       6
#define DEVICE_POWER        7

///Interrupt Request from Master to Slave
#define	INTR_IRQ0_MAIN       7      ///P7
#define	INTR_IRQ1_WIFI       3      ///P15
#define	INTR_IRQ2_SENSOR     0      ///P11
#define	INTR_IRQ3_VOICE     21      ///P29
#define	INTR_IRQ4_MOTOR     10      ///P24
#define	INTR_IRQ5_CM         7      ///P7
#define	INTR_IRQ6_CAMERA     1      ///P12
#define	INTR_IRQ7_POWER      2      ///P13

///Interrupt Answer from Slave To Master
#define	INTR_IRA0_MAIN       30     ///P27
#define	INTR_IRA1_WIFI        6     ///P22
#define	INTR_IRA2_SENSOR     24     ///P35
#define	INTR_IRA3_VOICE      31     ///P28
#define	INTR_IRA4_MOTOR      27     ///P36
#define	INTR_IRA5_CM         30     ///P27
#define	INTR_IRA6_CAMERA      4     ///P16
#define	INTR_IRA7_POWER       5     ///P18

///Interrupt Service Routine
void isr_sensor(void);
void isr_camera(void);
void isr_power(void);
void isr_wifi(void);
void isr_main(void);
void isr_voice(void);
void isr_motor(void);

#endif


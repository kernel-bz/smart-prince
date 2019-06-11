/**
 *	file name:	intr.c
 *	author:		  JungJaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   User Interrupt Module
 *
 *  Copyright(C) www.kernel.bz
 *  This code is licenced under the GPL.
  *
 *  Editted:
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "wiringPi.h"
#include "devices/serial.h"

extern int Relay_Index;

///Interrupt Service Routine for IRQ1(P11) from Sensor
void IRQ1(void)
{
    printf("IRQ1() From Senser\n");

    serial_send("AT+OSND=12\r\n");
    ///serial_recv();
}

///Interrupt Service Routine for IRQ1(P11) from Sensor
void IRA6(void)
{
    printf("IRA6() From Senser\n");

    serial_send("AT+OSND=12\r\n");
    ///serial_recv();
}

///Interrupt Service Routine for IRA1(P35) from Sensor
void IRA1(void)
{
    printf("IRA1() From Sensor\n");
    serial_send("AT+OSND=6\r\n");

    Relay_Index = (Relay_Index == 1) ? 5 : 1;   ///Off:On
}


void intr_init (void)
{
    ///wiringPiSetup();

    wiringPiISR (0, INT_EDGE_RISING, &IRQ1) ;	///IRQ1(From Sensor), P11
    ///wiringPiISR (24, INT_EDGE_RISING, &IRA1) ;	///IRA1(From Sensor), P35
    wiringPiISR (4, INT_EDGE_RISING, &IRA1) ;	///IRA1(From Sensor), P16
    wiringPiISR (27, INT_EDGE_RISING, &IRA6) ;	///IRA6(From Sensor), P36
}

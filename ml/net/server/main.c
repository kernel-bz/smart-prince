/**
 *	file name:  sp_server/main.c
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   TCP/IP Networks Main
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "networks.h"
#include "http_parser.h"
#include "dev_control.h"
#include "net_spi.h"
#include "intr.h"

extern int Relay_Index;

void signal_relay_on1(int signo)
{
    printf("signal_relay_on1(): %d\n", signo);
    Relay_Index = 1;
}

void signal_relay_on2(int signo)
{
    printf("signal_relay_on2(): %d\n", signo);
    Relay_Index = 2;
}

void signal_relay_on3(int signo)
{
    printf("signal_relay_on3(): %d\n", signo);
    Relay_Index = 3;

    ///dev_motor_on();
}

void signal_relay_on4(int signo)
{
    printf("signal_relay_on4(): %d\n", signo);
    Relay_Index = 4;
}


void signal_relay_off1(int signo)
{
    printf("signal_relay_off1(): %d\n", signo);
    Relay_Index = 5;
}

void signal_relay_off2(int signo)
{
    printf("signal_relay_off2(): %d\n", signo);
    Relay_Index = 6;
}

void signal_relay_off3(int signo)
{
    printf("signal_relay_off3(): %d\n", signo);
    Relay_Index = 7;

    ///dev_motor_off();
}

void signal_relay_off4(int signo)
{
    printf("signal_relay_off4(): %d\n", signo);
    Relay_Index = 8;
}

int main(void)
{
    ///signal 10
    if (signal(SIGUSR1, (void *)signal_relay_on1) == SIG_ERR) {
        printf("signal(SIGUSR1) init error!\n");
    }
    ///signal 12
    if (signal(SIGUSR2, (void *)signal_relay_on2) == SIG_ERR) {
        printf("signal(SIGUSR2) init error!\n");
    }
    ///signal 14
    if (signal(SIGALRM, (void *)signal_relay_on3) == SIG_ERR) {
        printf("signal(SIGALRM) init error!\n");
    }
    ///signal 26
    if (signal(SIGVTALRM, (void *)signal_relay_on4) == SIG_ERR) {
        printf("signal(SIGVTALRM) init error!\n");
    }

    ///signal 21
    if (signal(SIGTTIN, (void *)signal_relay_off1) == SIG_ERR) {
        printf("signal(SIGTTIN) init error!\n");
    }
    ///signal 22
    if (signal(SIGTTOU, (void *)signal_relay_off2) == SIG_ERR) {
        printf("signal(SIGTTOU) init error!\n");
    }
    ///signal 23
    if (signal(SIGURG, (void *)signal_relay_off3) == SIG_ERR) {
        printf("signal(SIGURG) init error!\n");
    }
    ///signal 27
    if (signal(SIGPROF, (void *)signal_relay_off4) == SIG_ERR) {
        printf("signal(SIGPROF) init error!\n");
    }

    dev_lib_setup();
    ///dev_gpio_init();

    intr_init();
    ///net_spi_init();

    ///net_server("10.90.0.1", 9000);
    net_server("10.30.0.1", 9000);

    while(1) pause();
    return 0;

}

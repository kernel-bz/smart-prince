/**
 *	file name:	dev_spi.c
 *	author:		  JungJaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   Device SPI Control
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
#include "wiringPiSPI.h"

typedef unsigned char  uint8_t;
typedef short int          int16_t;
typedef int                     int32_t;
typedef unsigned int    uint32_t;

#define	SPI_CHAN		0
#define STX         0xAA
#define ETX         0xAA
#define ACK         0xDD

static int spi_buffer_debug(int32_t ret, uint8_t *txd, uint8_t *rxd, int16_t len)
{
    if(ret == -1) {
        printf("SPI read error!\n");
        return -1;
    }

    int i;

    printf("SPI TXD: [ ");
    for(i=0; i<len; i++)  printf("%02X ", txd[i]);
    printf("]\nSPI RXD: [ ");
    for(i=0; i<len; i++)  printf("%02X ", rxd[i]);
    printf("]\n");


    printf("txd: %s\n", txd);
    printf("rxd: %s\n", rxd);
    return 0;
}

static int spi_open (int speed)
{
    int fd;

    if ((fd = wiringPiSPISetup (SPI_CHAN, speed)) < 0)
    {
        fprintf (stderr, "Can't open the SPI bus: %s\n", strerror (errno)) ;
        exit (EXIT_FAILURE) ;
    }

    return fd;
}

static void spi_read_loop(uint8_t *txd, uint8_t *rxd, int16_t len)
{
    int32_t ret;

    ///memset(rxd, 0, len);
    ret = wiringPiSPIDataTxRx(SPI_CHAN, txd, rxd, len);

    spi_buffer_debug(ret, txd, rxd, len);
}

static void spi_read (int speed)
{
    int32_t fd;
    uint8_t  *txd = "SPI RASPI3 RECV";  ///15 length
    uint8_t  rxd[25] = {0};
    int16_t len = strlen(txd) + 1;

    fd = spi_open(speed * 1000000) ;    //speed * 1MHz
    if (fd < 0) {
        printf("SPI device(/dev/spi*) open error!\n");
        return;
    }

    spi_read_loop(txd, rxd, len) ;

    close(fd);
}


///Interrupt Service Routine for GPIO 22 (P31)
void ISR1(void)
{
    static uint32_t idx=0;

    printf("\nISR1()...\n");

    if (idx%2) {
        system("sudo ./sp_client 0 sp_server &"); ///On
        printf("Relay Switch1 On\n");
    } else {
        system("sudo ./sp_client 4 sp_server &"); ///Off
        printf("Relay Switch1 Off\n");
    }

    idx++;
    //spi_read(2);    ///263kHz
    //spi_read(3);    ///384kHz
    ///spi_read(5);    ///666kHz
}


void net_spi_init (void)
{
    ///wiringPiSetup();

    ///wiringPiISR (0, INT_EDGE_RISING, &ISR1) ;	///IRQ1(From Sensor), P11
    /**
    wiringPiISR (23, INT_EDGE_RISING, &ISR2) ;	//P33
    wiringPiISR (24, INT_EDGE_RISING, &ISR3) ;	//P35
    wiringPiISR (25, INT_EDGE_RISING, &ISR4) ;	//P37
    wiringPiISR (27, INT_EDGE_RISING, &ISR5) ;	//P36
    */
}

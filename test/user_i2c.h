/**
  ******************************************************************************
  * @file    user_i2c.h
  * @author  JungJaeJoon(rgbi3307@nate.com)
  * @version V01
  * @date    2015-08-27
  * @brief   User I2C Module Header
  ******************************************************************************
  * @modification
  *
  * COPYRIGHT(c) 2015 www.kernel.bz
  *
  *
  ******************************************************************************
  */

#ifndef __USER_I2C_H
#define __USER_I2C_H

#include <stdint.h>

#include "stm32f0xx_hal.h"
#include "user_config.h"

#define I2C_ADDRESS         0x0F
#define I2C_ADDR_MAIN       0x21        //10
#define I2C_ADDR_WIFI       0x23        //11
#define I2C_ADDR_SENSOR     0x25        //12
#define I2C_ADDR_VOICE      0x27        //13
#define I2C_ADDR_MOTOR      0x29        //14
#define I2C_ADDR_CM         0x2B        //15
#define I2C_ADDR_CAMERA     0x2D        //16
#define I2C_ADDR_POWER      0x2F        //17

#define I2C_ADDR_RASPI      0x31        //18
#define I2C_ADDR_RASPI_CM   0x33        //19
#define I2C_ADDR_RASPI_W    0x35        //1a
#define I2C_ADDR_SMARTHOME  0x37        //1b

/* I2C TIMING Register define when I2C clock source is SYSCLK */
/* I2C TIMING is calculated in case of the I2C Clock source is the SYSCLK = 48 MHz */
/* This example use TIMING to 0x00A51314 to reach 1 MHz speed (Rise time = 100 ns, Fall time = 100 ns) */

#define I2C_TIMING      0x00A5090A      //583KHz
//#define I2C_TIMING      0x00A51314      //500KHz
//#define I2C_TIMING      0x00C82628      //355KHz
//#define I2C_TIMING      0x00C84C50      //225Khz
//#define I2C_TIMING      0x00C898A0      //130Khz

/* User can use this section to tailor I2Cx/I2Cx instance used and associated resources */
/* Definition for I2Cx clock resources */
#define I2Cx                            I2C1
#define RCC_PERIPHCLK_I2Cx              RCC_PERIPHCLK_I2C1
#define RCC_I2CxCLKSOURCE_SYSCLK        RCC_I2C1CLKSOURCE_SYSCLK
#define I2Cx_CLK_ENABLE()               __HAL_RCC_I2C1_CLK_ENABLE()
#define I2Cx_SDA_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()
#define I2Cx_SCL_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE() 

#define I2Cx_FORCE_RESET()              __HAL_RCC_I2C1_FORCE_RESET()
#define I2Cx_RELEASE_RESET()            __HAL_RCC_I2C1_RELEASE_RESET()

/* Definition for I2Cx Pins */
#define I2Cx_SCL_PIN                    GPIO_PIN_6
#define I2Cx_SCL_GPIO_PORT              GPIOB
#define I2Cx_SDA_PIN                    GPIO_PIN_7
#define I2Cx_SDA_GPIO_PORT              GPIOB
#define I2Cx_SCL_SDA_AF                 GPIO_AF1_I2C1

/* Definition for I2Cx's NVIC */
#define I2Cx_IRQn                    I2C1_IRQn
#define I2Cx_IRQHandler              I2C1_IRQHandler


#define I2C_TIMEOUT             3000    //3s

//I2C Status
#define I2C_STATUS_NONE           0xF8
#define I2C_STATUS_BUSY           0xF9
#define I2C_STATUS_OK             0xFA
#define I2C_STATUS_READY          0xFB
#define I2C_STATUS_CHECK          0xFC
#define I2C_STATUS_DAT            0xFD
#define I2C_STATUS_ERROR          0xFE

//Command
#define I2C_CMD_MAIN            0x80    //, 0x88
#define I2C_CMD_WIFI            0x90    //, 0x98
#define I2C_CMD_SENSOR          0xA0    //, 0xA8
#define I2C_CMD_VOICE           0xB0    //, 0xB8
#define I2C_CMD_MOTOR           0xC0    //, 0xC8
#define I2C_CMD_CM              0xD0    //, 0xD8
#define I2C_CMD_CAMERA          0xE0    //, 0xE8
#define I2C_CMD_POWER           0xF0
#define I2C_CMD_MASK            0xF8

#define I2C_RETRY_MAX           4       //for loop
#define I2C_WAIT_LOOP_MAX    1000       //for loop
#define I2C_WAIT_TIME        3000       //ms


typedef struct {
    uint8_t I2cRecvBuf[2];
    uint8_t I2cSendBuf[2];
    uint8_t status;
} I2CContol_T;

extern I2CContol_T I2CControl;

int32_t user_i2c_init(void);
int32_t user_i2c_recv(uint8_t *data, uint32_t len);
int32_t user_i2c_send(uint8_t *data);

void user_i2c_recv_wait(void);

void user_i2c_status_check(uint32_t cnt);
int32_t user_i2c_task(void);
int32_t user_i2c_test(uint32_t cnt);

#endif
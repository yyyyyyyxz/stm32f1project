/*------------------------------------------------------------------------------
 * File name   : init.h
 * Author      : yyyyyyyxz@gmail.com
 * Copyright(C): yyyyyyyxz@gmail.com. All rights reserved.
 *------------------------------------------------------------------------------
 */
#ifndef __INIT_H__
#define __INIT_H__
/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/USART/usart.h"
#include "./BSP/OLED/oled.h"
#include "./BSP/MOTOR/motor.h"
#include "./BSP/SERVO/servo.h"
#include "./BSP/SONAR/sonar.h"
#include <string.h>

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
/* 使能宏定义 */
#define ENABLE                  1
#define DISABLE                 0

/* 设备使能管理，0为不使能，1为使能 */
#define DEVICE_LED              DISABLE
#define DEVICE_KEY              DISABLE
/* 串口1的 PA9/PA10 已接前轮 PWM，保持关闭 */
#define DEVICE_USART            DISABLE
/* 串口2接 OpenMV，串口3接上位机 */
#define DEVICE_USART2           ENABLE
#define DEVICE_USART3           ENABLE
#define DEVICE_MOTOR            ENABLE
#define DEVICE_SERVO            ENABLE
#define DEVICE_SONAR            ENABLE    /* H2，HC-SR04 */

/* 软件/硬件 I2C 各自开关。OLED_MODE_SOFT、OLED_MODE_HARD 是总线类型，不能拿来当开关 */
#define DEVICE_OLED             ENABLE
#define DEVICE_OLED_SOFT        ENABLE
#define DEVICE_OLED_HARD        DISABLE

/*----------------------------------------------------------------------------*/
/*                           FUNCTION DECLARATION                             */
/*----------------------------------------------------------------------------*/
void System_Init(void);        /* 系统初始化 */
void Device_Init(void);        /* 设备初始化 */

#endif /* __INIT_H__ */

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

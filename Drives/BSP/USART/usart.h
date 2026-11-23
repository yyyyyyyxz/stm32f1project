/*------------------------------------------------------------------------------
 * File name   : usart.h
 * Author      : yyyyyyyxz@gmail.com
 * Copyright(C): yyyyyyyxz@gmail.com. All rights reserved.
 *------------------------------------------------------------------------------
 */
#ifndef __USART_H__
#define __USART_H__
/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "stdio.h"
#include "./SYSTEM/sys/sys.h"

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
/* 以下为串口1定义
 * 默认是选择USART1的
 * 注意: 通过修改这几个宏定义,可以支持USART1~UART5中的任意一个串口
 */
#define USART_TX_GPIO_PORT                  GPIOA
#define USART_TX_GPIO_PIN                   GPIO_PIN_9
#define USART_TX_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA口时钟使能 */

#define USART_RX_GPIO_PORT                  GPIOA
#define USART_RX_GPIO_PIN                   GPIO_PIN_10
#define USART_RX_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA口时钟使能 */

#define USART_UX                            USART1
#define USART_UX_IRQn                       USART1_IRQn
#define USART_UX_IRQHandler                 USART1_IRQHandler
#define USART_UX_CLK_ENABLE()               do{ __HAL_RCC_USART1_CLK_ENABLE(); }while(0)  /* USART1 时钟使能 */

#define USART_REC_LEN               200         /* 定义最大接收字节数 200 */
#define USART_EN_RX                 1           /* 使能（1）/禁止（0）串口1接收 */
#define RXBUFFERSIZE                10          /* 缓存大小 */

/* 串口2，OpenMV。PA2 接 P5(RX)，PA3 接 P4(TX)。5V 只接 VIN */
#define USART2_TX_GPIO_PORT                 GPIOA
#define USART2_TX_GPIO_PIN                  GPIO_PIN_2
#define USART2_TX_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define USART2_RX_GPIO_PORT                 GPIOA
#define USART2_RX_GPIO_PIN                  GPIO_PIN_3
#define USART2_RX_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define USART2_UX                           USART2
#define USART2_UX_IRQn                      USART2_IRQn
#define USART2_UX_CLK_ENABLE()              do{ __HAL_RCC_USART2_CLK_ENABLE(); }while(0)

/* 串口3，上位机。PB10 接模块 RXD，PB11 接模块 TXD。115200 8N1 */
#define USART3_TX_GPIO_PORT                 GPIOB
#define USART3_TX_GPIO_PIN                  GPIO_PIN_10
#define USART3_TX_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define USART3_RX_GPIO_PORT                 GPIOB
#define USART3_RX_GPIO_PIN                  GPIO_PIN_11
#define USART3_RX_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define USART3_UX                           USART3
#define USART3_UX_IRQn                      USART3_IRQn
#define USART3_UX_CLK_ENABLE()              do{ __HAL_RCC_USART3_CLK_ENABLE(); }while(0)

extern UART_HandleTypeDef UART_InitStruct;       /* HAL UART句柄 */

extern uint8_t  g_usart_rx_buf[USART_REC_LEN];  /* 接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 */
extern uint16_t g_usart_rx_sta;                 /* 接收状态标记 */
extern uint8_t g_rx_buffer[RXBUFFERSIZE];       /* HAL库UART接收buffer */

/*----------------------------------------------------------------------------*/
/*                           FUNCTION DECLARATION                             */
/*----------------------------------------------------------------------------*/
void Usart_Init(uint32_t bound);                /* 串口1初始化函数 */
void Usart2_Init(uint32_t baudrate);            /* 串口2初始化函数 */
void Usart3_Init(uint32_t baudrate);            /* 串口3初始化函数 */


#endif /* __USART_H__ */

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 * File name   : key.h
 * Author      : yyyyyyyxz@gmail.com
 * Copyright(C): yyyyyyyxz@gmail.com. All rights reserved.
 *------------------------------------------------------------------------------
 */
#ifndef __KEY_H__
#define __KEY_H__
/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./SYSTEM/sys/sys.h"

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
/* 引脚定义 */
#define KEY1_Pin                 GPIO_PIN_12
#define KEY1_GPIO_Port           GPIOB
#define KEY1_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define KEY2_Pin                 GPIO_PIN_13
#define KEY2_GPIO_Port           GPIOB             
#define KEY2_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define KEY3_Pin                 GPIO_PIN_15
#define KEY3_GPIO_Port           GPIOB             
#define KEY3_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define KEY4_Pin                 GPIO_PIN_8
#define KEY4_GPIO_Port           GPIOA             
#define KEY4_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

/* 按键返回值定义 */
#define KEY_NONE                 0
#define KEY1_PRESSED             1
#define KEY2_PRESSED             2
#define KEY3_PRESSED             3
#define KEY4_PRESSED             4

/*----------------------------------------------------------------------------*/
/*                           FUNCTION DECLARATION                             */
/*----------------------------------------------------------------------------*/
void Key_Init(void);
uint8_t Key_Scan(void);

#endif /* __KEY_H__ */

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/


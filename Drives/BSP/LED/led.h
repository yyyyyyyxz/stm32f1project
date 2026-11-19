/*------------------------------------------------------------------------------
 * File name   : led.h
 * Author      : yyyyyyyxz@gmail.com
 * Copyright(C): yyyyyyyxz@gmail.com. All rights reserved.
 *------------------------------------------------------------------------------
 */
#ifndef __LED_H__
#define __LED_H__
/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./SYSTEM/sys/sys.h"

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
/* 引脚定义 */
#define LED1_Pin                 GPIO_PIN_14
#define LED1_GPIO_Port           GPIOC
#define LED1_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOC_CLK_ENABLE()

#define LED2_Pin                 GPIO_PIN_15
#define LED2_GPIO_Port           GPIOC             
#define LED2_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOC_CLK_ENABLE()
/* 带参宏，控制 LED 灯亮灭的宏，可以像内联函数一样使用
 * 若 LED 低电平亮，设置 ON=0，OFF=1;若 LED 高电平亮，设置 ON=1 ，OFF=0
*/
#define OFF  	    	GPIO_PIN_SET
#define ON 		      GPIO_PIN_RESET
#define LED1(a)   	HAL_GPIO_WritePin(LED1_GPIO_Port,LED1_Pin,a)
#define LED1_TURN() HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin)
#define LED2(a)   	HAL_GPIO_WritePin(LED2_GPIO_Port,LED2_Pin,a)
#define LED2_TURN() HAL_GPIO_TogglePin(LED2_GPIO_Port,LED2_Pin)
/*----------------------------------------------------------------------------*/
/*                           FUNCTION DECLARATION                             */
/*----------------------------------------------------------------------------*/
void Led_Init(void);


#endif /* __LED_H__ */

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

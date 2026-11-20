/*------------------------------------------------------------------------------
 * File name   : oled_i2c.h
 * Author      : yyyyyyyxz@gmail.com
 * Copyright(C): yyyyyyyxz@gmail.com. All rights reserved.
 *------------------------------------------------------------------------------
 */
#ifndef __OLED_I2C_H__
#define __OLED_I2C_H__
/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./BSP/OLED/oled.h"

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
/* I2C1 默认引脚：PB6 = SCL，PB7 = SDA */
#define OLED_HW_SCL_Pin                 GPIO_PIN_6
#define OLED_HW_SCL_GPIO_Port           GPIOB
#define OLED_HW_SDA_Pin                 GPIO_PIN_7
#define OLED_HW_SDA_GPIO_Port           GPIOB
#define OLED_HW_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOB_CLK_ENABLE()

#define OLED_HW_I2C                     I2C1
#define OLED_HW_I2C_CLK_ENABLE()        __HAL_RCC_I2C1_CLK_ENABLE()
#define OLED_HW_I2C_SPEED               400000U
#define OLED_HW_I2C_TIMEOUT             100U

/*----------------------------------------------------------------------------*/
/*                           FUNCTION DECLARATION                             */
/*----------------------------------------------------------------------------*/
void Oled_I2c_Init(void);
void Oled_I2c_Clear(void);
void Oled_I2c_Refresh(void);
void Oled_I2c_Display(uint8_t sw);
void Oled_I2c_ColorTurn(uint8_t mode);
void Oled_I2c_DisplayTurn(uint8_t dir);
void Oled_I2c_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);
void Oled_I2c_ShowChinese(uint8_t x, uint8_t y, uint8_t index, uint8_t size);

#endif /* __OLED_I2C_H__ */

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

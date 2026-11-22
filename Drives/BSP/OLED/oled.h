/*------------------------------------------------------------------------------
 * File name   : oled.h
 * Author      : yyyyyyyxz@gmail.com
 * Copyright(C): yyyyyyyxz@gmail.com. All rights reserved.
 *------------------------------------------------------------------------------
 */
#ifndef __OLED_H__
#define __OLED_H__
/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./SYSTEM/sys/sys.h"

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
/* 传给 Oled_Init 的总线类型。同一端口上换一对引脚就是另一块屏 */
#define OLED_MODE_SOFT               0
#define OLED_MODE_HARD               1
#define OLED_SCREEN_MAX              4U

/* 引脚都在这个端口上。软件 I2C 可任选两脚，硬件 I2C 只能用芯片规定的脚 */
#define OLED_SCL_GPIO_Port               GPIOB
#define OLED_SCL_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOB_CLK_ENABLE()
#define OLED_SDA_GPIO_Port               GPIOB
#define OLED_SDA_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOB_CLK_ENABLE()
#define OLED_LOW                         GPIO_PIN_RESET
#define OLED_HIGH                        GPIO_PIN_SET
#define OLED_SCL_PIN(pin, a)             HAL_GPIO_WritePin(OLED_SCL_GPIO_Port, (pin), (a))
#define OLED_SDA_PIN(pin, a)             HAL_GPIO_WritePin(OLED_SDA_GPIO_Port, (pin), (a))

#define OLED_SOFT_SCL_Pin                GPIO_PIN_6
#define OLED_SOFT_SDA_Pin                GPIO_PIN_7
#define OLED_HARD_SCL_Pin                GPIO_PIN_6
#define OLED_HARD_SDA_Pin                GPIO_PIN_7

#define OLED_I2C1_CLK_ENABLE()           __HAL_RCC_I2C1_CLK_ENABLE()
#define OLED_I2C2_CLK_ENABLE()           __HAL_RCC_I2C2_CLK_ENABLE()
#define OLED_AFIO_CLK_ENABLE()           __HAL_RCC_AFIO_CLK_ENABLE()
#define OLED_I2C_SPEED                   10000U
#define OLED_I2C_TIMEOUT                 100U

/* 屏幕与 SSD1306 命令。改分辨率或器件地址只改这里 */
#define OLED_WIDTH                   128U
#define OLED_HEIGHT                  64U
#define OLED_PAGE                    8U

#define OLED_I2C_ADDR                0x78U
#define OLED_CTRL_CMD                0x00U
#define OLED_CTRL_DATA               0x40U

#define OLED_CMD_DISPLAY_OFF         0xAEU
#define OLED_CMD_DISPLAY_ON          0xAFU
#define OLED_CMD_CHARGE_PUMP         0x8DU
#define OLED_CMD_CHARGE_PUMP_ON      0x14U
#define OLED_CMD_CHARGE_PUMP_OFF     0x10U
#define OLED_CMD_NORMAL              0xA6U
#define OLED_CMD_INVERSE             0xA7U
#define OLED_CMD_SEG_NORMAL          0xA1U
#define OLED_CMD_SEG_REMAP           0xA0U
#define OLED_CMD_COM_NORMAL          0xC8U
#define OLED_CMD_COM_REMAP           0xC0U
#define OLED_CMD_PAGE_ADDR           0xB0U
#define OLED_CMD_LOW_COL             0x00U
#define OLED_CMD_HIGH_COL            0x10U

#define OLED_SHOW_ON                 1U
#define OLED_SHOW_OFF                0U
#define OLED_DIR_NORMAL              0U
#define OLED_DIR_TURN                1U

/*----------------------------------------------------------------------------*/
/*                           FUNCTION DECLARATION                             */
/*----------------------------------------------------------------------------*/
void Oled_Init(uint8_t mode, uint16_t scl_pin, uint16_t sda_pin);
void Oled_Clear(void);
void Oled_Refresh(void);
void Oled_Display(uint8_t sw);
void Oled_ColorTurn(uint8_t mode);
void Oled_DisplayTurn(uint8_t dir);
void Oled_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);
void Oled_ShowChinese(uint8_t x, uint8_t y, uint8_t index, uint8_t size);

#endif /* __OLED_H__ */

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

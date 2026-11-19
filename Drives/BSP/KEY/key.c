/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved 
 *-----------------------------------------------------------------------------
 * File name   : key.c
 * Description : 按键初始化及扫描
 * Author      : yyyyyyyxz@gmail.com
 * Update date : 2026.11.17
 *-----------------------------------------------------------------------------
 * Note        : 根据原理图，按键接 3.3V，按下为高电平，需要开启内部下拉。
 *
**/

/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./BSP/KEY/key.h"

/*----------------------------------------------------------------------------*/
/*                                DECLARATION                                 */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                 FUNCTION                                   */
/*----------------------------------------------------------------------------*/
/**
 * 功能: 初始化KEY相关IO口, 并使能时钟
 * 参数: None
 * 返回: None
 * 说明: 配置为输入模式，开启内部下拉电阻
 */
void Key_Init(void)
{
    /*定义一个GPIO_InitTypeDef类型的结构体*/
    GPIO_InitTypeDef  GPIO_InitStruct = {0};

    /*开启KEY相关的GPIO外设时钟*/
    KEY1_GPIO_CLK_ENABLE();
    KEY2_GPIO_CLK_ENABLE();
    KEY3_GPIO_CLK_ENABLE();
    KEY4_GPIO_CLK_ENABLE();
  
    /*设置引脚为输入模式，并开启内部下拉*/
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;        /* 按键为输入设备 */
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;          /* 按下为高电平，松开需下拉至低电平 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;   /* 速率设置 */

    /*初始化 KEY1*/
    GPIO_InitStruct.Pin = KEY1_Pin;
    HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);	

		/*初始化 KEY2*/
    GPIO_InitStruct.Pin = KEY2_Pin;
    HAL_GPIO_Init(KEY2_GPIO_Port, &GPIO_InitStruct);	
		
		/*初始化 KEY3*/
    GPIO_InitStruct.Pin = KEY3_Pin;
    HAL_GPIO_Init(KEY3_GPIO_Port, &GPIO_InitStruct);	
	
    /*初始化 KEY4 (在 GPIOA 端口)*/
    GPIO_InitStruct.Pin = KEY4_Pin;
    HAL_GPIO_Init(KEY4_GPIO_Port, &GPIO_InitStruct);
}

/**
 * 功能: 按键扫描函数
 * 参数: None
 * 返回: 0-无按键按下，1~4-对应按键按下
 * 说明: 检测到按下后，等待松手才返回，实现单次触发
 */
uint8_t Key_Scan(void)
{
    /* 检测 KEY1 */
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_SET) 
    {
        HAL_Delay(5); /* 软件消抖 */
        if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_SET) 
        {
            while (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_SET); /* 等待松手 */
            return KEY1_PRESSED;
        }
    }
    /* 检测 KEY2 */
    else if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET) 
    {
        HAL_Delay(5);
        if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET) 
        {
            while (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET);
            return KEY2_PRESSED;
        }
    }
    /* 检测 KEY3 */
    else if (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_SET) 
    {
        HAL_Delay(5);
        if (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_SET) 
        {
            while (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_SET);
            return KEY3_PRESSED;
        }
    }
    /* 检测 KEY4 */
    else if (HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == GPIO_PIN_SET) 
    {
        HAL_Delay(5);
        if (HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == GPIO_PIN_SET) 
        {
            while (HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == GPIO_PIN_SET);
            return KEY4_PRESSED;
        }
    }
    
    return KEY_NONE; /* 无按键按下 */
}

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

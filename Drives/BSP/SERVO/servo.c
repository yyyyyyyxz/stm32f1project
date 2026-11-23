/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved
 *-----------------------------------------------------------------------------
 * File name   : servo.c
 * Description : SG90 舵机，TIM2 中断输出 50Hz 脉宽
 * Author      : yyyyyyyxz@gmail.com
 * Update date : 2026.11.23
 *-----------------------------------------------------------------------------
 * Note        : TIM2 只做 1us 时基。不做引脚重映射，避免 PA2/PA3 离开 USART2。
 *
**/

/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./BSP/SERVO/servo.h"

/*----------------------------------------------------------------------------*/
/*                                DECLARATION                                 */
/*----------------------------------------------------------------------------*/
static void servo_out(uint8_t high);

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
static uint8_t s_angle = 90;

/*----------------------------------------------------------------------------*/
/*                                 FUNCTION                                   */
/*----------------------------------------------------------------------------*/
/**
 * 功能: 写出舵机信号脚电平
 * 参数: high 为 1 时拉高，为 0 时拉低
 * 返回: None
 * 说明: None
 */
static void servo_out(uint8_t high)
{
    HAL_GPIO_WritePin(SERVO_GPIO_Port, SERVO_Pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * 功能: 初始化舵机
 * 参数: None
 * 返回: None
 * 说明: 上电停在 90°。TIM2 时钟 72MHz，1us 一拍，周期 20ms
 */
void Servo_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct = {0};

    SERVO_GPIO_CLK_ENABLE();
    SERVO_TIM_CLK_ENABLE();

    GPIO_InitStruct.Pin = SERVO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SERVO_GPIO_Port, &GPIO_InitStruct);
    servo_out(0);

    TIM2->CR1 = 0;
    TIM2->PSC = 71;
    TIM2->ARR = 19999;
    TIM2->CCR1 = 1500;
    TIM2->DIER = TIM_DIER_UIE | TIM_DIER_CC1IE;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0;
    TIM2->CR1 = TIM_CR1_CEN;

    HAL_NVIC_SetPriority(TIM2_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    s_angle = 90;
}

/**
 * 功能: 设置舵机角度
 * 参数: angle 0..180
 * 返回: None
 * 说明: 超出范围按 180° 处理
 */
void Servo_SetAngle(uint8_t angle)
{
    uint32_t pulse;

    if (angle > 180U)
    {
        angle = 180U;
    }
    s_angle = angle;
    pulse = 500U + ((uint32_t)angle * 2000U) / 180U;
    TIM2->CCR1 = pulse;
}

/**
 * 功能: 读取当前舵机角度
 * 参数: None
 * 返回: 0..180
 * 说明: None
 */
uint8_t Servo_GetAngle(void)
{
    return s_angle;
}

/**
 * 功能: TIM2 中断，在周期起点拉高、比较点拉低
 * 参数: None
 * 返回: None
 * 说明: 脉宽由 CCR1 决定
 */
void TIM2_IRQHandler(void)
{
    uint32_t sr = TIM2->SR;
    uint32_t clear = 0;

    if (sr & TIM_SR_UIF)
    {
        servo_out(1);
        clear |= TIM_SR_UIF;
    }
    if (sr & TIM_SR_CC1IF)
    {
        servo_out(0);
        clear |= TIM_SR_CC1IF;
    }
    if (clear != 0U)
    {
        TIM2->SR = ~clear;
    }
}

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved
 *-----------------------------------------------------------------------------
 * File name   : sonar.c
 * Description : HC-SR04 超声波测距
 * Author      : yyyyyyyxz@gmail.com
 * Update date : 2026.11.23
 *-----------------------------------------------------------------------------
 * Note        : HC-SR04。TRIG 脉宽约 12us，距离 cm = ECHO 脉宽 us / 58。
 *               ECHO 脉宽用 DWT 计数。Delay_Init 已经打开 DWT。
 *
**/

/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./BSP/SONAR/sonar.h"
#include "./SYSTEM/delay/delay.h"

/*----------------------------------------------------------------------------*/
/*                                DECLARATION                                 */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
static uint8_t s_dwt_ok;

/*----------------------------------------------------------------------------*/
/*                                 FUNCTION                                   */
/*----------------------------------------------------------------------------*/
/**
 * 功能: 初始化 HC-SR04 的 TRIG 和 ECHO
 * 参数: None
 * 返回: None
 * 说明: ECHO 下拉。顺带确认 DWT 计数器在走
 */
void Sonar_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct = {0};
    uint32_t start;

    SONAR_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin = SONAR_TRIG_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SONAR_TRIG_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SONAR_TRIG_GPIO_Port, SONAR_TRIG_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = SONAR_ECHO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(SONAR_ECHO_GPIO_Port, &GPIO_InitStruct);

    start = DWT->CYCCNT;
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    s_dwt_ok = (DWT->CYCCNT != start) ? 1U : 0U;
}

/**
 * 功能: 用 HC-SR04 测一次距离
 * 参数: None
 * 返回: 厘米，0 表示超时、超量程或 DWT 不可用
 * 说明: 阻塞不超过约 25ms。HC-SR04 换算 cm = us / 58，有效约 2..400cm
 */
uint16_t Sonar_ReadCm(void)
{
    uint32_t cyc_us;
    uint32_t mark;
    uint32_t us;
    uint32_t cm;

    if (s_dwt_ok == 0U)
    {
        return 0;
    }

    cyc_us = SystemCoreClock / 1000000U;
    if (cyc_us == 0U)
    {
        return 0;
    }

    HAL_GPIO_WritePin(SONAR_TRIG_GPIO_Port, SONAR_TRIG_Pin, GPIO_PIN_SET);
    Delay_us(12);
    HAL_GPIO_WritePin(SONAR_TRIG_GPIO_Port, SONAR_TRIG_Pin, GPIO_PIN_RESET);

    mark = DWT->CYCCNT;
    while (HAL_GPIO_ReadPin(SONAR_ECHO_GPIO_Port, SONAR_ECHO_Pin) == GPIO_PIN_RESET)
    {
        if ((DWT->CYCCNT - mark) > (cyc_us * 25000U))
        {
            return 0;
        }
    }

    mark = DWT->CYCCNT;
    while (HAL_GPIO_ReadPin(SONAR_ECHO_GPIO_Port, SONAR_ECHO_Pin) == GPIO_PIN_SET)
    {
        if ((DWT->CYCCNT - mark) > (cyc_us * 25000U))
        {
            return 0;
        }
    }

    us = (DWT->CYCCNT - mark) / cyc_us;
    cm = us / 58U;
    if ((cm == 0U) || (cm > 400U))
    {
        return 0;
    }
    return (uint16_t)cm;
}

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

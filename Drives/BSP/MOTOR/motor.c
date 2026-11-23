/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved
 *-----------------------------------------------------------------------------
 * File name   : motor.c
 * Description : TB6612 四轮驱动
 * Author      : yyyyyyyxz@gmail.com
 * Update date : 2026.11.23
 *-----------------------------------------------------------------------------
 * Note        : 前轮 TIM1，后轮 TIM4。PB3/PB4/PA15 默认是 JTAG，初始化时关掉 JTAG，保留 SWD。
 *
**/

/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./BSP/MOTOR/motor.h"

/*----------------------------------------------------------------------------*/
/*                                DECLARATION                                 */
/*----------------------------------------------------------------------------*/
static void motor_gpio_clock(void);
static void motor_gpio_out(GPIO_TypeDef *port, uint16_t pin, uint32_t speed);
static void motor_gpio_af(GPIO_TypeDef *port, uint16_t pin);
static void motor_pwm_init(TIM_TypeDef *tim, uint8_t advanced);
static void motor_bridge(GPIO_TypeDef *port, uint16_t in1, uint16_t in2,
                         volatile uint32_t *ccr, int16_t speed);

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                 FUNCTION                                   */
/*----------------------------------------------------------------------------*/
/**
 * 功能: 打开电机用到的 GPIO 和复用时钟
 * 参数: None
 * 返回: None
 * 说明: None
 */
static void motor_gpio_clock(void)
{
    MOTOR_GPIOA_CLK_ENABLE();
    MOTOR_GPIOB_CLK_ENABLE();
    MOTOR_GPIOC_CLK_ENABLE();
    MOTOR_AFIO_CLK_ENABLE();
}

/**
 * 功能: 把一组引脚配成推挽输出
 * 参数: port 端口，pin 引脚，speed 翻转速度
 * 返回: None
 * 说明: PC14/PC15 只能用低速
 */
static void motor_gpio_out(GPIO_TypeDef *port, uint16_t pin, uint32_t speed)
{
    GPIO_InitTypeDef  GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = speed;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

/**
 * 功能: 把一组引脚配成复用推挽，给定时器 PWM 用
 * 参数: port 端口，pin 引脚
 * 返回: None
 * 说明: None
 */
static void motor_gpio_af(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef  GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

/**
 * 功能: 打开一个定时器的两路 PWM
 * 参数: tim 定时器，advanced 为 1 时打开高级定时器主输出
 * 返回: None
 * 说明: 占空比寄存器初值为 0
 */
static void motor_pwm_init(TIM_TypeDef *tim, uint8_t advanced)
{
    tim->CR1 = 0;
    tim->PSC = 2;
    tim->ARR = MOTOR_PWM_MAX - 1U;
    tim->CCR1 = 0;
    tim->CCR2 = 0;
    tim->CCMR1 = (6U << 4) | (1U << 3) | (6U << 12) | (1U << 11);
    tim->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;
    if (advanced)
    {
        tim->BDTR |= TIM_BDTR_MOE;
    }
    tim->CR1 |= TIM_CR1_ARPE;
    tim->EGR = TIM_EGR_UG;
    tim->CR1 |= TIM_CR1_CEN;
}

/**
 * 功能: 按速度设置一路 H 桥方向和占空比
 * 参数: port 方向口，in1 正转脚，in2 反转脚，ccr 占空比寄存器，speed -100..100
 * 返回: None
 * 说明: 速度为 0 时两脚都拉低，电机滑行
 */
static void motor_bridge(GPIO_TypeDef *port, uint16_t in1, uint16_t in2,
                         volatile uint32_t *ccr, int16_t speed)
{
    uint16_t duty;

    if (speed > 100)
    {
        speed = 100;
    }
    if (speed < -100)
    {
        speed = -100;
    }

    if (speed == 0)
    {
        HAL_GPIO_WritePin(port, in1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(port, in2, GPIO_PIN_RESET);
        duty = 0;
    }
    else if (speed > 0)
    {
        HAL_GPIO_WritePin(port, in1, GPIO_PIN_SET);
        HAL_GPIO_WritePin(port, in2, GPIO_PIN_RESET);
        duty = (uint16_t)((int16_t)speed * 10);
    }
    else
    {
        HAL_GPIO_WritePin(port, in1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(port, in2, GPIO_PIN_SET);
        duty = (uint16_t)((int16_t)(-speed) * 10);
    }
    *ccr = duty;
}

/**
 * 功能: 初始化四路电机
 * 参数: None
 * 返回: None
 * 说明: 关掉 JTAG 后才能使用 PB4。结束后四轮速度为 0，后轮 STBY 拉高
 */
void Motor_Init(void)
{
    motor_gpio_clock();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    MOTOR_TIM1_CLK_ENABLE();
    MOTOR_TIM4_CLK_ENABLE();

    motor_gpio_af(MOTOR_FL_PWM_GPIO_Port, MOTOR_FL_PWM_Pin | MOTOR_FR_PWM_Pin);
    motor_gpio_af(MOTOR_RL_PWM_GPIO_Port, MOTOR_RL_PWM_Pin | MOTOR_RR_PWM_Pin);

    motor_gpio_out(MOTOR_FL_IN1_GPIO_Port, MOTOR_FL_IN1_Pin | MOTOR_FL_IN2_Pin, GPIO_SPEED_FREQ_LOW);
    motor_gpio_out(MOTOR_FR_IN1_GPIO_Port, MOTOR_FR_IN1_Pin | MOTOR_FR_IN2_Pin, GPIO_SPEED_FREQ_HIGH);
    motor_gpio_out(MOTOR_RL_IN1_GPIO_Port,
                   MOTOR_RL_IN1_Pin | MOTOR_RL_IN2_Pin | MOTOR_RR_IN1_Pin | MOTOR_RR_IN2_Pin | MOTOR_STBY_Pin,
                   GPIO_SPEED_FREQ_HIGH);

    HAL_GPIO_WritePin(MOTOR_STBY_GPIO_Port, MOTOR_STBY_Pin, GPIO_PIN_SET);

    motor_pwm_init(TIM1, 1U);
    motor_pwm_init(TIM4, 0U);
    Motor_Set4(0, 0, 0, 0);
}

/**
 * 功能: 设置麦轮四轮速度
 * 参数: fl 左前，fr 右前，rl 左后，rr 右后，单位 -100..100
 * 返回: None
 * 说明: 正数对应 IN1 为高、IN2 为低
 */
void Motor_Set4(int16_t fl, int16_t fr, int16_t rl, int16_t rr)
{
    motor_bridge(MOTOR_FL_IN1_GPIO_Port, MOTOR_FL_IN1_Pin, MOTOR_FL_IN2_Pin, &TIM1->CCR2, fl);
    motor_bridge(MOTOR_FR_IN1_GPIO_Port, MOTOR_FR_IN1_Pin, MOTOR_FR_IN2_Pin, &TIM1->CCR1, fr);
    motor_bridge(MOTOR_RL_IN1_GPIO_Port, MOTOR_RL_IN1_Pin, MOTOR_RL_IN2_Pin, &TIM4->CCR1, rl);
    motor_bridge(MOTOR_RR_IN1_GPIO_Port, MOTOR_RR_IN1_Pin, MOTOR_RR_IN2_Pin, &TIM4->CCR2, rr);
}

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

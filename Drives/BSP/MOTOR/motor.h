/*------------------------------------------------------------------------------
 * File name   : motor.h
 * Author      : yyyyyyyxz@gmail.com
 * Copyright(C): yyyyyyyxz@gmail.com. All rights reserved.
 *------------------------------------------------------------------------------
 */
#ifndef __MOTOR_H__
#define __MOTOR_H__
/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./SYSTEM/sys/sys.h"

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
/* 原理图两块 TB6612。速度 -100..100。前轮模块 STBY 需外接 3.3V */
/* 前左 U10：PWMA=PA9，AIN1=PC14，AIN2=PC15 */
#define MOTOR_FL_PWM_Pin                 GPIO_PIN_9
#define MOTOR_FL_PWM_GPIO_Port           GPIOA
#define MOTOR_FL_IN1_Pin                 GPIO_PIN_14
#define MOTOR_FL_IN1_GPIO_Port           GPIOC
#define MOTOR_FL_IN2_Pin                 GPIO_PIN_15
#define MOTOR_FL_IN2_GPIO_Port           GPIOC

/* 前右 U11：PWMB=PA8，BIN1=PB5，BIN2=PB4 */
#define MOTOR_FR_PWM_Pin                 GPIO_PIN_8
#define MOTOR_FR_PWM_GPIO_Port           GPIOA
#define MOTOR_FR_IN1_Pin                 GPIO_PIN_5
#define MOTOR_FR_IN1_GPIO_Port           GPIOB
#define MOTOR_FR_IN2_Pin                 GPIO_PIN_4
#define MOTOR_FR_IN2_GPIO_Port           GPIOB

/* 后左 U12：PWMA=PB6，AIN1=PA4，AIN2=PA10 */
#define MOTOR_RL_PWM_Pin                 GPIO_PIN_6
#define MOTOR_RL_PWM_GPIO_Port           GPIOB
#define MOTOR_RL_IN1_Pin                 GPIO_PIN_4
#define MOTOR_RL_IN1_GPIO_Port           GPIOA
#define MOTOR_RL_IN2_Pin                 GPIO_PIN_10
#define MOTOR_RL_IN2_GPIO_Port           GPIOA

/* 后右 U13：PWMB=PB7，BIN1=PA0，BIN2=PA1，STBY=PA11 */
#define MOTOR_RR_PWM_Pin                 GPIO_PIN_7
#define MOTOR_RR_PWM_GPIO_Port           GPIOB
#define MOTOR_RR_IN1_Pin                 GPIO_PIN_0
#define MOTOR_RR_IN1_GPIO_Port           GPIOA
#define MOTOR_RR_IN2_Pin                 GPIO_PIN_1
#define MOTOR_RR_IN2_GPIO_Port           GPIOA
#define MOTOR_STBY_Pin                   GPIO_PIN_11
#define MOTOR_STBY_GPIO_Port             GPIOA

#define MOTOR_GPIOA_CLK_ENABLE()         __HAL_RCC_GPIOA_CLK_ENABLE()
#define MOTOR_GPIOB_CLK_ENABLE()         __HAL_RCC_GPIOB_CLK_ENABLE()
#define MOTOR_GPIOC_CLK_ENABLE()         __HAL_RCC_GPIOC_CLK_ENABLE()
#define MOTOR_AFIO_CLK_ENABLE()          __HAL_RCC_AFIO_CLK_ENABLE()
#define MOTOR_TIM1_CLK_ENABLE()          __HAL_RCC_TIM1_CLK_ENABLE()
#define MOTOR_TIM4_CLK_ENABLE()          __HAL_RCC_TIM4_CLK_ENABLE()

/* 72MHz 下 PSC=2、ARR=999，约 24kHz，占空比 0..1000 */
#define MOTOR_PWM_MAX                    1000U

/*----------------------------------------------------------------------------*/
/*                           FUNCTION DECLARATION                             */
/*----------------------------------------------------------------------------*/
void Motor_Init(void);
void Motor_Set4(int16_t fl, int16_t fr, int16_t rl, int16_t rr);

#endif /* __MOTOR_H__ */

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

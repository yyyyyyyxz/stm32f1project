/*------------------------------------------------------------------------------
 * File name   : sonar.h
 * Author      : yyyyyyyxz@gmail.com
 * Copyright(C): yyyyyyyxz@gmail.com. All rights reserved.
 *------------------------------------------------------------------------------
 */
#ifndef __SONAR_H__
#define __SONAR_H__
/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./SYSTEM/sys/sys.h"

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
/* H2：HC-SR04。5V、TRIG=PA12、ECHO=PA15、GND。返回厘米，0 表示超时或无效 */
#define SONAR_TRIG_Pin                   GPIO_PIN_12
#define SONAR_TRIG_GPIO_Port             GPIOA
#define SONAR_ECHO_Pin                   GPIO_PIN_15
#define SONAR_ECHO_GPIO_Port             GPIOA
#define SONAR_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOA_CLK_ENABLE()

/*----------------------------------------------------------------------------*/
/*                           FUNCTION DECLARATION                             */
/*----------------------------------------------------------------------------*/
void Sonar_Init(void);
uint16_t Sonar_ReadCm(void);

#endif /* __SONAR_H__ */

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

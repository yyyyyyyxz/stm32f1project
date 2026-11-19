/*------------------------------------------------------------------------------
 * 文件名称 : delay.h
 * 作者     : yyyyyyyxz@gmail.com
 * 版权(C)  : yyyyyyyxz@gmail.com. All rights reserved.
 *------------------------------------------------------------------------------
 */
 
#ifndef __DELAY_H__
#define __DELAY_H__

/*----------------------------------------------------------------------------*/
/*                                 头文件包含                                 */
/*----------------------------------------------------------------------------*/
#include "./SYSTEM/sys/sys.h"

/*----------------------------------------------------------------------------*/
/*                                   数据                                     */
/*----------------------------------------------------------------------------*/
/* 无 */

/*----------------------------------------------------------------------------*/
/*                                函数声明                                    */
/*----------------------------------------------------------------------------*/
void Delay_Init(uint16_t sysclk);       /* 延时函数初始化，sysclk 单位 MHz */
void Delay_ms(uint32_t nms);            /* 延时 nms 毫秒（0 ~ 0xFFFFFFFF） */
void Delay_us(uint32_t nus);            /* 延时 nus 微秒（0 ~ 0xFFFFFFFF） */

/* 这里覆写了 HAL_Delay()：采用忙等方式，因此在任何中断里、以及关中断状态下
 * 都能安全调用。HAL_GetTick() 和 HAL 的超时机制仍然跑在 1ms 的 SysTick
 * 节拍上，本模块不再去碰 SysTick。 */
void HAL_Delay(uint32_t Delay);

/*----------------------------------------------------------------------------*/
/*                               非阻塞定时器                                 */
/*----------------------------------------------------------------------------*/
/* 软件定时器只保存一个时间戳，调用它不会阻塞，CPU 可以继续干别的事。
 * 启动一次后，在主循环里随时用 Expired() / Remain() 查询即可。
 *
 *   static Delay_TimerMs t_blink;
 *
 *   Delay_TimerMsStart(&t_blink, 500);
 *   while (1)
 *   {
 *       if (Delay_TimerMsExpired(&t_blink))         // 500ms 到了吗？
 *       {
 *           Delay_TimerMsStart(&t_blink, 500);      // 重新装载，进入下一轮
 *           HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);  // 要执行的“任务”
 *       }
 *
 *       Task_Uart();                                // 不会被上面的定时器阻塞
 *       Task_Key();
 *   }
 *
 * Delay_TimerMs：1ms 分辨率，时间基准为 SysTick（HAL_GetTick()）。
 * Delay_TimerUs：1 个周期分辨率，时间基准为 DWT->CYCCNT；若 DWT 不可用，
 *                会自动降级为 1ms 分辨率。
 * 两者都做了回绕处理；单次时长请保持在 2^31 个单位以内。
 */
typedef struct
{
    uint32_t start;                     /* 启动时记录的时间戳 */
    uint32_t duration;                  /* 定时时长，单位毫秒 */
} Delay_TimerMs;

typedef struct
{
    uint32_t start;                     /* DWT 周期戳；use_tick != 0 时为 HAL tick 戳 */
    uint32_t span;                      /* 时长（周期数）；use_tick != 0 时为毫秒数 */
    uint8_t  use_tick;                  /* 1: DWT 不可用，已降级为 1ms 分辨率 */
} Delay_TimerUs;

void     Delay_TimerMsStart(Delay_TimerMs *t, uint32_t ms);   /* 启动/重装毫秒定时器 */
uint8_t  Delay_TimerMsExpired(const Delay_TimerMs *t);        /* 1: 已到期 */
uint32_t Delay_TimerMsRemain(const Delay_TimerMs *t);         /* 剩余毫秒数，到期返回 0 */

void     Delay_TimerUsStart(Delay_TimerUs *t, uint32_t us);   /* 启动/重装微秒定时器 */
uint8_t  Delay_TimerUsExpired(const Delay_TimerUs *t);        /* 1: 已到期 */
uint32_t Delay_TimerUsRemain(const Delay_TimerUs *t);         /* 剩余微秒数，到期返回 0 */

#endif /* __DELAY_H__ */

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

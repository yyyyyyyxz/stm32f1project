/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved 
 *-----------------------------------------------------------------------------
 * 文件名称 : delay.c
 * 描述     : 延时函数
 * 作者     : yyyyyyyxz@gmail.com
 * 更新日期 : 2026.11.9
 *-----------------------------------------------------------------------------
 * 备注     : 延时的时间基准取自 DWT 周期计数器（Cortex-M3 内核的调试部件）。
 *            它与 SysTick 互相独立，所以延时期间 HAL 的 1ms 节拍
 *            （uwTick / HAL_GetTick / HAL_Delay）依然正常工作。
 *            如果 DWT 计数器不可用，本模块会回退到 SysTick 方案，
 *            并在延时前后保存/恢复 SysTick 寄存器。
 *
 *-----------------------------------------------------------------------------
 * 
 *
**/

/*----------------------------------------------------------------------------*/
/*                                 头文件包含                                 */
/*----------------------------------------------------------------------------*/
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"

/*----------------------------------------------------------------------------*/
/*                                   声明                                     */
/*----------------------------------------------------------------------------*/
/* 无 */

/*----------------------------------------------------------------------------*/
/*                                   数据                                     */
/*----------------------------------------------------------------------------*/
/* 每次忙等分块使用的周期数。保持在 2^31 以下，保证 (DWT->CYCCNT - start)
 * 这种自由回绕差值在一次分块内不会翻转。 */
#define DELAY_CYCLES_CHUNK      (1UL << 30)

static uint32_t g_cyc_per_us = 0;   /* 每微秒的 HCLK 周期数；0 表示尚未初始化 */
static uint8_t  g_use_dwt    = 0;   /* 1: DWT 周期计数器可用，0: 使用 SysTick 回退方案 */

/*----------------------------------------------------------------------------*/
/*                                   函数                                     */
/*----------------------------------------------------------------------------*/
/**
 * 功能  : 使能并自检 DWT 周期计数器
 * 参数  : 无
 * 返回  : 无
 * 说明  : DWT->CYCCNT 记录 HCLK 周期数，HAL 不会碰它，也不与 SysTick 共用。
 */
static void delay_dwt_init(void)
{
    uint32_t start;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;     /* 允许访问 DWT 寄存器 */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;                /* 启动周期计数器 */
    DWT->CYCCNT = 0;

    start = DWT->CYCCNT;                                /* 确认计数器真的在递增 */
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    g_use_dwt = (DWT->CYCCNT != start) ? 1U : 0U;
}

/**
 * 功能  : 忙等指定数量的内核时钟周期（DWT 版本）
 * 参数  : cycles: HCLK 周期数，必须小于 2^31
 * 返回  : 无
 * 说明  : 无符号减法按设计允许回绕，因此只要 cycles 小于 2^31 就不必再做溢出判断。
 */
static void delay_cycles(uint32_t cycles)
{
    uint32_t start = DWT->CYCCNT;

    while ((DWT->CYCCNT - start) < cycles)
    {
        /* 忙等。CYCCNT 是 volatile 的，编译器无法把这个循环优化掉。 */
    }
}

/**
 * 功能  : 微秒级延时（SysTick 版本，仅作回退）
 * 参数  : nus: 要延时的微秒数
 * 返回  : 无
 * 说明  : 延时前后保存/恢复 SysTick 配置，保证 HAL 的 1ms 节拍不会因此丢失。
 *         能用 DWT 版本时应优先使用 DWT 版本。
 */
static void delay_us_systick(uint32_t nus)
{
    uint32_t ctrl, load, fac, step, maxstep, tmp;

    fac = g_cyc_per_us / 8U;                            /* SysTick 由 HCLK / 8 分频时钟驱动 */
    if (fac == 0U)
    {
        fac = 1U;
    }

    maxstep = ((1UL << 24) - 1UL) / fac;                /* 一个 24 位重载值能覆盖的最长延时 */
    if (maxstep == 0U)
    {
        maxstep = 1U;
    }

    ctrl = SysTick->CTRL;                               /* 保存原来的配置 */
    load = SysTick->LOAD;

    while (nus)
    {
        step = (nus > maxstep) ? maxstep : nus;

        SysTick->CTRL = 0U;                             /* 重新配置前先停止计数器 */
        SysTick->LOAD = step * fac;                     /* 重载值 */
        SysTick->VAL  = 0U;                             /* 清零计数器 */
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;   /* HCLK / 8，启动计数 */

        do
        {
            tmp = SysTick->CTRL;
        }
        while ((tmp & SysTick_CTRL_ENABLE_Msk) && !(tmp & SysTick_CTRL_COUNTFLAG_Msk));

        nus -= step;
    }

    SysTick->CTRL = 0U;                                 /* 恢复原来的配置 */
    SysTick->LOAD = load;
    SysTick->VAL  = 0U;                                 /* 让保存的周期从头开始计数 */
    SysTick->CTRL = ctrl;
}

/**
 * 功能  : 确保延时基准已经配置
 * 参数  : 无
 * 返回  : 无
 * 说明  : 正常应在 main() 中调用 Delay_Init()。若忘记调用，这里用当前系统主频
 *         自动补上，避免除以 0。
 */
static void delay_ensure_init(void)
{
    if (g_cyc_per_us == 0U)
    {
        Delay_Init((uint16_t)(SystemCoreClock / 1000000U));
    }
}

/**
 * 功能  : 延时函数初始化
 * 参数  : sysclk: 系统时钟频率，即 CPU 频率（HCLK），单位 MHz
 * 返回  : 无
 * 说明  : 请在时钟树配置完成后调用。传入 0 时自动使用 SystemCoreClock。
 */
void Delay_Init(uint16_t sysclk)
{
    if (sysclk == 0U)
    {
        sysclk = (uint16_t)(SystemCoreClock / 1000000U);
    }

    g_cyc_per_us = sysclk;                              /* 1MHz 即每微秒 1 个周期，所以 MHz 数值等于每微秒周期数 */
    delay_dwt_init();
}

/**
 * 功能  : 微秒级延时
 * 参数  : nus: 要延时的微秒数，0 是合法输入
 * 返回  : 无
 * 说明  : 范围只受 uint32_t 限制；长延时会被拆成 2^30 周期的分块。
 *         延时期间中断保持开启，因此中断服务程序只会让实际延时变长，绝不会变短。
 */
void Delay_us(uint32_t nus)
{
    uint64_t cycles;

    if (nus == 0U)                                      /* 重载值为 0 时 COUNTFLAG 永远不会置位 */
    {
        return;
    }

    delay_ensure_init();

    if (g_use_dwt)
    {
        cycles = (uint64_t)nus * (uint64_t)g_cyc_per_us;

        while (cycles)
        {
            uint32_t chunk = (cycles > (uint64_t)DELAY_CYCLES_CHUNK) ? DELAY_CYCLES_CHUNK : (uint32_t)cycles;

            delay_cycles(chunk);
            cycles -= chunk;
        }
    }
    else
    {
        delay_us_systick(nus);
    }
}

/**
 * 功能  : 毫秒级延时
 * 参数  : nms: 要延时的毫秒数，0 是合法输入
 * 返回  : 无
 * 说明  : 旧版本参数是 uint16_t，会把 HAL_Delay(uint32_t) 传入的
 *         大于 65535ms 的值静默截断。
 */
void Delay_ms(uint32_t nms)
{
    uint32_t repeat = nms / 1000;       /* 拆成整秒，保证每次 Delay_us 调用都在范围内 */
    uint32_t remain = nms % 1000;

    while (repeat)
    {
        Delay_us(1000 * 1000);          /* 用 Delay_us 实现 1000ms 延时 */
        repeat--;
    }

    if (remain)
    {
        Delay_us(remain * 1000);        /* 剩余毫秒部分同样用 Delay_us 补齐 */
    }
}

/**
 * 功能  : HAL 库内部调用的延时函数
 * 参数  : Delay: 要延时的毫秒数
 * 返回  : 无
 * 说明  : 这里有意保留覆写。HAL 自带的版本依赖 uwTick，而 SysTick 处于最低
 *         优先级（TICK_INT_PRIORITY = 0x0F），在中断里或关中断时调用会死锁。
 *         这个忙等版本在任何上下文都能用，而且不会再把 uint32_t 参数
 *         截断成 uint16_t。
 */
void HAL_Delay(uint32_t Delay)
{
    Delay_ms(Delay);
}

/*----------------------------------------------------------------------------*/
/*                               非阻塞定时器                                 */
/*----------------------------------------------------------------------------*/
/**
 * 功能  : 启动（或重新装载）毫秒级软件定时器
 * 参数  : t : 定时器对象
 *           ms: 定时时长，单位毫秒
 * 返回  : 无
 * 说明  : 只记录一个时间戳，调用它不会阻塞。
 */
void Delay_TimerMsStart(Delay_TimerMs *t, uint32_t ms)
{
    t->start    = HAL_GetTick();
    t->duration = ms;
}

/**
 * 功能  : 查询毫秒定时器是否到期
 * 参数  : t: 定时器对象
 * 返回  : 1 表示已到期，0 表示尚未到期
 * 说明  : 无符号减法让 32 位 tick 回绕也不会算错。
 */
uint8_t Delay_TimerMsExpired(const Delay_TimerMs *t)
{
    return ((uint32_t)(HAL_GetTick() - t->start) >= t->duration) ? 1U : 0U;
}

/**
 * 功能  : 查询毫秒定时器的剩余时间
 * 参数  : t: 定时器对象
 * 返回  : 剩余毫秒数，已到期时返回 0
 * 说明  : 无
 */
uint32_t Delay_TimerMsRemain(const Delay_TimerMs *t)
{
    uint32_t elapsed = (uint32_t)(HAL_GetTick() - t->start);

    return (elapsed >= t->duration) ? 0U : (t->duration - elapsed);
}

/**
 * 功能  : 启动（或重新装载）微秒级软件定时器
 * 参数  : t : 定时器对象
 *           us: 定时时长，单位微秒
 * 返回  : 无
 * 说明  : 基于 DWT，分辨率 1 个周期。若 DWT 不可用，会自动降级为 1ms 分辨率
 *         （use_tick = 1），而不是失效。72MHz 下时长被钳位在约 29.8 秒。
 */
void Delay_TimerUsStart(Delay_TimerUs *t, uint32_t us)
{
    uint64_t cycles;

    delay_ensure_init();

    if (g_use_dwt == 0U)                                /* 降级模式：1ms 分辨率 */
    {
        t->use_tick = 1U;
        t->start    = HAL_GetTick();
        t->span     = (us / 1000U) + (((us % 1000U) != 0U) ? 1U : 0U);   /* 向上取整，且不会溢出 */
        return;
    }

    cycles = (uint64_t)us * (uint64_t)g_cyc_per_us;
    if (cycles > (uint64_t)0x7FFFFFFFUL)                /* 保证一次回绕区间内判断无歧义 */
    {
        cycles = (uint64_t)0x7FFFFFFFUL;
    }

    t->use_tick = 0U;
    t->start    = DWT->CYCCNT;
    t->span     = (uint32_t)cycles;
}

/**
 * 功能  : 查询微秒定时器是否到期
 * 参数  : t: 定时器对象
 * 返回  : 1 表示已到期，0 表示尚未到期
 * 说明  : 无
 */
uint8_t Delay_TimerUsExpired(const Delay_TimerUs *t)
{
    uint32_t now;

    if (t->use_tick != 0U)
    {
        now = HAL_GetTick();
    }
    else
    {
        now = DWT->CYCCNT;
    }

    return ((uint32_t)(now - t->start) >= t->span) ? 1U : 0U;
}

/**
 * 功能  : 查询微秒定时器的剩余时间
 * 参数  : t: 定时器对象
 * 返回  : 剩余微秒数，已到期时返回 0
 * 说明  : 无
 */
uint32_t Delay_TimerUsRemain(const Delay_TimerUs *t)
{
    uint32_t elapsed, left;

    if (t->use_tick != 0U)
    {
        elapsed = (uint32_t)(HAL_GetTick() - t->start);
        if (elapsed >= t->span)
        {
            return 0U;
        }

        left = t->span - elapsed;                       /* 剩余毫秒数 */
        return (left > 4294967U) ? 0xFFFFFFFFU : (left * 1000U);
    }

    if (g_cyc_per_us == 0U)
    {
        return 0U;
    }

    elapsed = (uint32_t)(DWT->CYCCNT - t->start);
    if (elapsed >= t->span)
    {
        return 0U;
    }

    left = t->span - elapsed;
    return left / g_cyc_per_us;
}

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

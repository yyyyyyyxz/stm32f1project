/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved 
 *-----------------------------------------------------------------------------
 * 文件名称 : sys.c
 * 描述     : 
 * 作者     : yyyyyyyxz@gmail.com
 * 更新日期 : 2026.11.9
 *-----------------------------------------------------------------------------
 * 备注     :
 *
 *-----------------------------------------------------------------------------
 * 
 *
**/

/*----------------------------------------------------------------------------*/
/*                                 头文件包含                                 */
/*----------------------------------------------------------------------------*/
#include "./SYSTEM/sys/sys.h"

/*----------------------------------------------------------------------------*/
/*                                   声明                                     */
/*----------------------------------------------------------------------------*/
/* 无 */

/*----------------------------------------------------------------------------*/
/*                                   数据                                     */
/*----------------------------------------------------------------------------*/
/* 无 */

/*----------------------------------------------------------------------------*/
/*                                   函数                                     */
/*----------------------------------------------------------------------------*/
/**
 * 功能  : 设置 NVIC 向量表地址
 * 参数  : baseaddr: 基地址
 *           offset  : 偏移（必须是 0 或 0x200 的整数倍）
 * 返回  : 无
 * 说明  : 无
 */
void sys_nvic_set_vector_table(uint32_t baseaddr, uint32_t offset)
{
    /* 写 NVIC 向量表寄存器。VTOR 的 [8:0] 位是保留位，读出来恒为 0，
     * 这里一并清掉（offset 必须按 0x200 对齐）。 */
    SCB->VTOR = baseaddr | (offset & (uint32_t)0xFFFFFE00);
}


/**
 * 功能  : 执行 WFI 指令进入低功耗模式，等待中断唤醒
 * 参数  : 无
 * 返回  : 无
 * 说明  : 无
 */
void sys_wfi_set(void)
{
    __ASM volatile("wfi");
}

/**
 * 功能  : 关闭所有中断（不包括 fault 和 NMI 中断）
 * 参数  : 无
 * 返回  : 之前的 PRIMASK 值，供嵌套的临界区恢复
 * 说明  : 如果调用方本身可能就处在关中断状态，把返回值交给
 *         sys_intx_restore()，而不是调用 sys_intx_enable()。
 */
uint32_t sys_intx_disable(void)
{
    uint32_t primask = __get_PRIMASK();     /* 先记住原来的状态 */

    __ASM volatile("cpsid i");
    return primask;
}

/**
 * 功能  : 打开所有中断
 * 参数  : 无
 * 返回  : 无
 * 说明  : 无条件开中断；嵌套临界区里请改用 sys_intx_restore()。
 */
void sys_intx_enable(void)
{
    __ASM volatile("cpsie i");
}

/**
 * 功能  : 恢复 sys_intx_disable() 保存的中断状态
 * 参数  : primask: sys_intx_disable() 返回的值
 * 返回  : 无
 * 说明  : 保证临界区可以安全嵌套。
 */
void sys_intx_restore(uint32_t primask)
{
    __set_PRIMASK(primask);
}

/**
 * 功能  : 设置栈顶地址
 * 参数  : addr: 栈顶地址
 * 返回  : 无
 * 说明  : 用于设置栈顶地址，常见于 MDK 启动代码中。
 */
void sys_msr_msp(uint32_t addr)
{
    __set_MSP(addr);    /* 设置栈顶地址 */
}

/**
 * 功能  : 进入待机（standby）模式
 * 参数  : 无
 * 返回  : 无
 * 说明  : 无
 */
void sys_standby(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();                 /* 使能 PWR 时钟 */
    SET_BIT(PWR->CR, PWR_CR_CWUF);              /* 清掉旧的唤醒标志，否则 WFI 会立刻返回 */
    SET_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);   /* 选择深度睡眠，PDDS 才能选到 standby */
    SET_BIT(PWR->CR, PWR_CR_PDDS);              /* PDDS = 1：进入 standby 模式 */

    __WFI();                                    /* 进入 standby；唤醒后 MCU 会复位重启 */
}

/**
 * 功能  : 系统软件复位
 * 参数  : 无
 * 返回  : 无
 * 说明  : 无
 */
void sys_soft_reset(void)
{
    NVIC_SystemReset();
}

/**
 * 功能  : 系统时钟初始化函数
 * 参数  : plln: PLL 倍频系数，范围 2~16
 * 返回  : 0 表示成功，1 表示 HSE / PLL / 时钟切换失败
 * 说明  : 启动时 SystemInit() 已经初始化过一次系统时钟。
 *         若此处失败，CPU 会继续跑在 HSI（8MHz）上，由调用方决定如何提示或处理。
 */
uint8_t SystemClock_Config(uint32_t plln)
{
    HAL_StatusTypeDef ret = HAL_ERROR;
    RCC_OscInitTypeDef rcc_oscinitstructure;
    RCC_ClkInitTypeDef rcc_clkinitstructure;

    rcc_oscinitstructure.OscillatorType = RCC_OSCILLATORTYPE_HSE;       /* 使用外部高速时钟 HSE */
    rcc_oscinitstructure.HSEState = RCC_HSE_ON;                         /* 使能 HSE */
    rcc_oscinitstructure.HSEPredivValue = RCC_HSE_PREDIV_DIV1;          /* HSE 预分频 */
    rcc_oscinitstructure.PLL.PLLState = RCC_PLL_ON;                     /* 使能 PLL */
    rcc_oscinitstructure.PLL.PLLSource = RCC_PLLSOURCE_HSE;             /* 选择 HSE 作为 PLL 时钟源 */
    rcc_oscinitstructure.PLL.PLLMUL = plln;                             /* 设置 PLL 倍频系数 */
    ret = HAL_RCC_OscConfig(&rcc_oscinitstructure);                     /* 初始化 */

    if (ret != HAL_OK)
    {
        return 1;                                                       /* HSE 或 PLL 没有锁定，继续跑在 HSI 上 */
    }

    /* 选择 PLL 作为系统时钟源，并配置 HCLK、PCLK1 和 PCLK2 */
    rcc_clkinitstructure.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    rcc_clkinitstructure.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;        /* 系统时钟源选择 PLL */
    rcc_clkinitstructure.AHBCLKDivider = RCC_SYSCLK_DIV1;               /* AHB 预分频 = 1 */
    rcc_clkinitstructure.APB1CLKDivider = RCC_HCLK_DIV2;                /* APB1 预分频 = 2 */
    rcc_clkinitstructure.APB2CLKDivider = RCC_HCLK_DIV1;                /* APB2 预分频 = 1 */
    ret = HAL_RCC_ClockConfig(&rcc_clkinitstructure, FLASH_LATENCY_2);  /* 同时把 FLASH 等待周期设为 2WS，适配高主频。 */

    if (ret != HAL_OK)
    {
        return 1;                                                       /* 无法把 PLL 作为系统时钟源 */
    }

    return 0;
}

/**
 * 功能  : 重新应用系统时钟配置
 * 参数  : plln: PLL 倍频系数，范围 2~16
 * 返回  : 0 表示成功，1 表示失败
 * 说明  : 与 SystemClock_Config() 相同，保留这个带返回值的入口，方便需要在
 *         运行中重新配置时钟的场景，例如从 STOP 模式唤醒之后。
 */
uint8_t sys_clock_set(uint32_t plln)
{
    return SystemClock_Config(plln);
}


/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

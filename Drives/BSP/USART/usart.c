/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved 
 *-----------------------------------------------------------------------------
 * File name   : usart.c
 * Description : 串口初始化代码(一般串口1)，支持printf
 * Author      : yyyyyyyxz@gmail.com
 * Update date : 2026.11.9
 *-----------------------------------------------------------------------------
 * Note        :
 *
 *
**/

/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./SYSTEM/sys/sys.h"
#include "./BSP/USART/usart.h"

/*----------------------------------------------------------------------------*/
/*                                DECLARATION                                 */
/*----------------------------------------------------------------------------*/
/* 加入以下代码, 支持printf函数, 而不需要选择use MicroLIB */

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)

#if (__ARMCC_VERSION >= 6010050)            /* 使用AC6编译器时 */
__asm(".global __use_no_semihosting\n\t");  /* 声明不使用半主机模式 */
__asm(".global __ARM_use_no_argv \n\t");    /* AC6需要声明main函数为无参数形式，否则可能无法正常使用半主机模式 */

#else
/* 使用AC5编译器时, 要在这里定义__FILE 和 不使用半主机模式 */
#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
    /* Whatever you require here. If the only file you are using is */
    /* standard output using printf() for debugging, no file handling */
    /* is required. */
};

#endif

/* 不使用半主机模式，需要定义_ttywrch\_sys_exit\_sys_command_string函数, 以同时兼容AC6和AC5模式 */
int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

/* 定义_sys_exit()以避免使用半主机模式 */
void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}


/* FILE 在 stdio.h里面定义. */
FILE __stdout;

/* MDK需要定义fputc函数, printf函数最终会通过调用fputc输出字符串到串口 */
int fputc(int ch, FILE *f)
{
    while ((USART_UX->SR & 0X40) == 0);     /* 等待上一个字符发送完成 */

    USART_UX->DR = (uint8_t)ch;             /* 将要发送的字符 ch 写入到DR寄存器 */
    return ch;
}
#endif

#if defined(__GNUC__)
int _write(int file, char *ptr, int len)
{
    int i;

    (void)file;
    for (i = 0; i < len; i++)
    {
        while ((USART_UX->SR & 0x40) == 0);
        USART_UX->DR = (uint8_t)ptr[i];
    }
    return len;
}
#endif

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
#if USART_EN_RX /* 如果使能了接收 */

/* 接收缓冲区, 大小为USART_REC_LEN个字节 */
uint8_t g_usart_rx_buf[USART_REC_LEN];

/* 接收状态
 *  bit15，      接收完成标志
 *  bit14，      接收到0x0d
 *  bit13~0，    接收到的有效字节数目
*/
uint16_t g_usart_rx_sta = 0;

uint8_t g_rx_buffer[RXBUFFERSIZE];  /* HAL库使用的串口接收缓冲区 */

UART_HandleTypeDef UART_InitStruct;  /* UART句柄（UART结构体初始化） */
#endif

/*----------------------------------------------------------------------------*/
/*                                 FUNCTION                                   */
/*----------------------------------------------------------------------------*/
#if USART_EN_RX /* 如果使能了接收 */
 /**
 * 功能: 串口初始化函数
 * 参数: baudrate: 波特率, 根据自己需要设置波特率值
 * 返回值: None
 * 说明: 必须设置正确的时钟源, 否则串口波特率就会不正常.
 *       这里使用的USART的时钟源在 SystemClock_Config() 函数中已经设置过了
 */
void Usart_Init(uint32_t baudrate)
{
    /*UART 初始化设置*/
    UART_InitStruct.Instance = USART_UX;                                       /* USART_UX */
    UART_InitStruct.Init.BaudRate = baudrate;                                  /* 波特率 */
    UART_InitStruct.Init.WordLength = UART_WORDLENGTH_8B;                      /* 字长为8位数据格式 */
    UART_InitStruct.Init.StopBits = UART_STOPBITS_1;                           /* 一个停止位 */
    UART_InitStruct.Init.Parity = UART_PARITY_NONE;                            /* 无奇偶校验位 */
    UART_InitStruct.Init.HwFlowCtl = UART_HWCONTROL_NONE;                      /* 无硬件流控 */
    UART_InitStruct.Init.Mode = UART_MODE_TX_RX;                               /* 收发模式 */
    HAL_UART_Init(&UART_InitStruct);                                           /* HAL_UART_Init()会使能UART1 */

    /* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
    HAL_UART_Receive_IT(&UART_InitStruct, (uint8_t *)g_rx_buffer, RXBUFFERSIZE); 
}

/**
 * 功能: UART底层初始化函数
 * 参数: huart: UART句柄类型指针
 * 返回值: None
 * 说明: 此函数会被HAL_UART_Init()调用,完成时钟使能，引脚配置，中断配置
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init_struct;

    if (huart->Instance == USART_UX)                            /* 如果使用的是串口1，进入串口1 MSP初始化 */
    {
        USART_TX_GPIO_CLK_ENABLE();                             /* 使能串口TX引脚时钟 */
        USART_RX_GPIO_CLK_ENABLE();                             /* 使能串口RX引脚时钟 */
        USART_UX_CLK_ENABLE();                                  /* 使能串口时钟 */

        gpio_init_struct.Pin = USART_TX_GPIO_PIN;               /* 串口发送引脚号 */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* 复用推挽输出 */
        gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* IO速度设置为高速 */
        HAL_GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);
                
        gpio_init_struct.Pin = USART_RX_GPIO_PIN;               /* 串口RX引脚模式设置 */
        gpio_init_struct.Mode = GPIO_MODE_AF_INPUT;    
        HAL_GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);   /* 串口RX引脚必须设置成输入模式 */
        
#if USART_EN_RX
        HAL_NVIC_EnableIRQ(USART_UX_IRQn);                      /* 使能USART1中断通道 */
        HAL_NVIC_SetPriority(USART_UX_IRQn, 3, 3);              /* 抢占优先级3，子优先级3 */
#endif
    }
}

/**
 * 功能: 串口接收数据回调函数
 * 参数: huart:串口句柄
 * 返回值: None
 * 说明: 数据处理在这部分
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART_UX)                    /* 如果是串口1 */
    {
        if ((g_usart_rx_sta & 0x8000) == 0)             /* 接收未完成 */
        {
            if (g_usart_rx_sta & 0x4000)                /* 接收到了0x0d（即回车键） */
            {
                if (g_rx_buffer[0] != 0x0a)             /* 接收到的不是0x0a（即不是换行键） */
                {
                    g_usart_rx_sta = 0;                 /* 接收错误,重新开始 */
                }
                else                                    /* 接收到的是0x0a（即换行键） */
                {
                    g_usart_rx_sta |= 0x8000;           /* 接收完成了 */
                }
            }
            else                                        /* 还没收到0X0d（即回车键） */
            {
                if (g_rx_buffer[0] == 0x0d)
                    g_usart_rx_sta |= 0x4000;
                else
                {
                    g_usart_rx_buf[g_usart_rx_sta & 0X3FFF] = g_rx_buffer[0];
                    g_usart_rx_sta++;

                    if (g_usart_rx_sta > (USART_REC_LEN - 1))
                    {
                        g_usart_rx_sta = 0;             /* 接收数据错误,重新开始接收 */
                    }
                }
            }
        }
    }
}

/**
 * 功能: 串口中断服务函数
 * 参数: None
 * 返回值: None
 * 说明: 注意,读取USARTx->SR能避免莫名其妙的错误
 */
void USART_UX_IRQHandler(void)
{
		 /* 调用HAL库中断处理函数 */
    HAL_UART_IRQHandler(&UART_InitStruct);                              

		/* 重新开启中断并接收数据 */
    while (HAL_UART_Receive_IT(&UART_InitStruct, (uint8_t *)g_rx_buffer, RXBUFFERSIZE) != HAL_OK)     
    {
        /* 如果出错卡在这里 */
    }
}
#endif


/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

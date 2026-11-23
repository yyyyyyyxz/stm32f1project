/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved
 *-----------------------------------------------------------------------------
 * File name   : link.c
 * Description : OpenMV 与上位机的行缓冲
 * Author      : yyyyyyyxz@gmail.com
 * Update date : 2026.11.23
 *-----------------------------------------------------------------------------
 * Note        : 串口初始化在 usart.c。这里只组行、发送和收中断。
 *               一行以 \n 结束，\r 丢掉。每路缓存 4 行。
 *
**/

/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./BSP/LINK/link.h"
#include "./BSP/USART/usart.h"

/*----------------------------------------------------------------------------*/
/*                                DECLARATION                                 */
/*----------------------------------------------------------------------------*/
typedef struct
{
    char line[LINK_LINE];
    uint16_t len;
    char q[LINK_Q][LINK_LINE];
    volatile uint8_t head;
    volatile uint8_t tail;
} LinkPort;

static void link_copy(char *dst, const char *src, uint16_t max);
static void link_rx_byte(LinkPort *port, uint8_t ch);
static uint8_t link_take_line(LinkPort *port, char *out, uint16_t max);
static void link_send(USART_TypeDef *uart, const char *text);

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
static LinkPort s_host;
static LinkPort s_cam;

/*----------------------------------------------------------------------------*/
/*                                 FUNCTION                                   */
/*----------------------------------------------------------------------------*/
/**
 * 功能: 把字符串拷进调用者的缓冲区
 * 参数: dst 目标，src 源，max 目标容量，含结尾 0
 * 返回: None
 * 说明: None
 */
static void link_copy(char *dst, const char *src, uint16_t max)
{
    uint16_t i = 0;

    if (max == 0U)
    {
        return;
    }
    while ((src[i] != '\0') && ((uint16_t)(i + 1U) < max))
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/**
 * 功能: 把收到的一个字节组进行缓冲
 * 参数: port 端口状态，ch 字节
 * 返回: None
 * 说明: 遇到换行才入队。队满则丢掉这一行
 */
static void link_rx_byte(LinkPort *port, uint8_t ch)
{
    uint8_t next;

    if (ch == '\r')
    {
        return;
    }
    if (ch == '\n')
    {
        port->line[port->len] = '\0';
        next = (uint8_t)((port->head + 1U) % LINK_Q);
        if ((port->len > 0U) && (next != port->tail))
        {
            link_copy(port->q[port->head], port->line, LINK_LINE);
            port->head = next;
        }
        port->len = 0;
        return;
    }
    if (port->len < (LINK_LINE - 1U))
    {
        port->line[port->len++] = (char)ch;
    }
    else
    {
        port->len = 0;
    }
}

/**
 * 功能: 取出一行已经收齐的文本
 * 参数: port 端口状态，out 输出缓冲，max 容量
 * 返回: 1 取到，0 队列空
 * 说明: 出队时关中断
 */
static uint8_t link_take_line(LinkPort *port, char *out, uint16_t max)
{
    if (port->head == port->tail)
    {
        return 0;
    }
    __disable_irq();
    link_copy(out, port->q[port->tail], max);
    port->tail = (uint8_t)((port->tail + 1U) % LINK_Q);
    __enable_irq();
    return 1;
}

/**
 * 功能: 阻塞发送一串字符
 * 参数: uart 串口，text 以 0 结尾
 * 返回: None
 * 说明: text 为空指针时直接返回
 */
static void link_send(USART_TypeDef *uart, const char *text)
{
    if (text == NULL)
    {
        return;
    }
    while (*text != '\0')
    {
        while ((uart->SR & USART_SR_TXE) == 0U)
        {
        }
        uart->DR = (uint8_t)(*text++);
    }
}

/**
 * 功能: 向上位机发送一串文本
 * 参数: text 以 0 结尾，调用者自己带换行
 * 返回: None
 * 说明: None
 */
void Link_HostSend(const char *text)
{
    link_send(USART3_UX, text);
}

/**
 * 功能: 向 OpenMV 发送一串文本
 * 参数: text 以 0 结尾
 * 返回: None
 * 说明: None
 */
void Link_OpenmvSend(const char *text)
{
    link_send(USART2_UX, text);
}

/**
 * 功能: 取上位机已经收齐的一行
 * 参数: out 输出缓冲，max 容量
 * 返回: 1 取到，0 没有
 * 说明: None
 */
uint8_t Link_HostLine(char *out, uint16_t max)
{
    return link_take_line(&s_host, out, max);
}

/**
 * 功能: 取 OpenMV 已经收齐的一行
 * 参数: out 输出缓冲，max 容量
 * 返回: 1 取到，0 没有
 * 说明: None
 */
uint8_t Link_OpenmvLine(char *out, uint16_t max)
{
    return link_take_line(&s_cam, out, max);
}

/**
 * 功能: USART2 中断，收 OpenMV 的字节
 * 参数: None
 * 返回: None
 * 说明: 溢出、帧错误、噪声错误时读一次 DR 清标志
 */
void USART2_IRQHandler(void)
{
    uint32_t sr = USART2_UX->SR;

    if (sr & USART_SR_RXNE)
    {
        link_rx_byte(&s_cam, (uint8_t)USART2_UX->DR);
    }
    else if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE))
    {
        (void)USART2_UX->DR;
    }
}

/**
 * 功能: USART3 中断，收上位机的字节
 * 参数: None
 * 返回: None
 * 说明: 溢出、帧错误、噪声错误时读一次 DR 清标志
 */
void USART3_IRQHandler(void)
{
    uint32_t sr = USART3_UX->SR;

    if (sr & USART_SR_RXNE)
    {
        link_rx_byte(&s_host, (uint8_t)USART3_UX->DR);
    }
    else if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE))
    {
        (void)USART3_UX->DR;
    }
}

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

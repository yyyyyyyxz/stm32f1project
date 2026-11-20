/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved 
 *-----------------------------------------------------------------------------
 * File name   : oled.c
 * Description : 0.96 寸 SSD1306，软件 I2C
 * Author      : yyyyyyyxz@gmail.com
 * Update date : 2026.11.20
 *-----------------------------------------------------------------------------
 * Note        : 引脚、电平和命令都来自 oled.h 的宏。字模在 oled_font.c。
 *
**/

/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./BSP/OLED/oled.h"
#include "./BSP/OLED/oled_font.h"
#include <string.h>

/*----------------------------------------------------------------------------*/
/*                                DECLARATION                                 */
/*----------------------------------------------------------------------------*/
static void oled_delay(void);
static void oled_start(void);
static void oled_stop(void);
static void oled_wait_ack(void);
static void oled_write_byte(uint8_t data);
static void oled_write_cmd(uint8_t cmd);
static void oled_draw_point(uint8_t x, uint8_t y, uint8_t on);
static void oled_show_ascii(uint8_t x, uint8_t y, char ch, uint8_t size);

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
static uint8_t s_gram[OLED_WIDTH][OLED_PAGE];

/*----------------------------------------------------------------------------*/
/*                                 FUNCTION                                   */
/*----------------------------------------------------------------------------*/
/**
 * 功能: 软件 I2C 短延时
 * 参数: None
 * 返回: None
 * 说明: 只服务 SCL/SDA 时序，不占用 SysTick
 */
static void oled_delay(void)
{
    volatile uint8_t count = 8U;

    while (count--)
    {
    }
}

/**
 * 功能: 发送 I2C 起始信号
 * 参数: None
 * 返回: None
 * 说明: 电平由 OLED_SCL / OLED_SDA 宏写出
 */
static void oled_start(void)
{
    OLED_SDA(OLED_HIGH);
    OLED_SCL(OLED_HIGH);
    oled_delay();
    OLED_SDA(OLED_LOW);
    oled_delay();
    OLED_SCL(OLED_LOW);
    oled_delay();
}

/**
 * 功能: 发送 I2C 停止信号
 * 参数: None
 * 返回: None
 * 说明: None
 */
static void oled_stop(void)
{
    OLED_SDA(OLED_LOW);
    OLED_SCL(OLED_HIGH);
    oled_delay();
    OLED_SDA(OLED_HIGH);
}

/**
 * 功能: 给出应答时钟，不读取 SDA
 * 参数: None
 * 返回: None
 * 说明: SSD1306 作为从机，这里只完成第 9 个时钟
 */
static void oled_wait_ack(void)
{
    OLED_SDA(OLED_HIGH);
    oled_delay();
    OLED_SCL(OLED_HIGH);
    oled_delay();
    OLED_SCL(OLED_LOW);
    oled_delay();
}

/**
 * 功能: 发送一个字节，高位在前
 * 参数: data 待发送数据
 * 返回: None
 * 说明: None
 */
static void oled_write_byte(uint8_t data)
{
    uint8_t bit;

    for (bit = 0U; bit < 8U; bit++)
    {
        if ((data & 0x80U) != 0U)
        {
            OLED_SDA(OLED_HIGH);
        }
        else
        {
            OLED_SDA(OLED_LOW);
        }
        oled_delay();
        OLED_SCL(OLED_HIGH);
        oled_delay();
        OLED_SCL(OLED_LOW);
        data <<= 1;
    }
}

/**
 * 功能: 写一条 SSD1306 命令
 * 参数: cmd 命令字节
 * 返回: None
 * 说明: 控制字节使用 OLED_CTRL_CMD
 */
static void oled_write_cmd(uint8_t cmd)
{
    oled_start();
    oled_write_byte(OLED_I2C_ADDR);
    oled_wait_ack();
    oled_write_byte(OLED_CTRL_CMD);
    oled_wait_ack();
    oled_write_byte(cmd);
    oled_wait_ack();
    oled_stop();
}

/**
 * 功能: 在显存中画一个点
 * 参数: x 列 0~127，y 行 0~63，on 为 1 点亮
 * 返回: None
 * 说明: 不立即刷新屏幕
 */
static void oled_draw_point(uint8_t x, uint8_t y, uint8_t on)
{
    uint8_t page;
    uint8_t mask;

    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT))
    {
        return;
    }

    page = (uint8_t)(y / 8U);
    mask = (uint8_t)(1U << (y % 8U));
    if (on != 0U)
    {
        s_gram[x][page] |= mask;
    }
    else
    {
        s_gram[x][page] &= (uint8_t)(~mask);
    }
}

/**
 * 功能: 显示一个 ASCII 字符
 * 参数: x、y 左上角，ch 字符，size 为 12/16/24/32
 * 返回: None
 * 说明: 字模取自 oled_font.c，高位在前
 */
static void oled_show_ascii(uint8_t x, uint8_t y, char ch, uint8_t size)
{
    const uint8_t *font;
    uint8_t temp;
    uint8_t count;
    uint8_t bit;
    uint8_t y0;
    uint8_t index;

    if ((ch < ' ') || (ch > '~'))
    {
        return;
    }

    index = (uint8_t)(ch - ' ');
    y0 = y;
    count = (uint8_t)((size / 8U + ((size % 8U) != 0U ? 1U : 0U)) * (size / 2U));

    switch (size)
    {
    case 12U:
        font = aFontASCII12[index];
        break;
    case 16U:
        font = aFontASCII16[index];
        break;
    case 24U:
        font = aFontASCII24[index];
        break;
    case 32U:
        font = aFontASCII32[index];
        break;
    default:
        return;
    }

    while (count--)
    {
        temp = *font++;
        for (bit = 0U; bit < 8U; bit++)
        {
            oled_draw_point(x, y, (temp & 0x80U) != 0U ? 1U : 0U);
            temp <<= 1;
            y++;
            if ((uint8_t)(y - y0) == size)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}

/**
 * 功能: 初始化 OLED 引脚和 SSD1306
 * 参数: None
 * 返回: None
 * 说明: SCL、SDA 配置为开漏输出并上拉
 */
void Oled_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    OLED_SCL_GPIO_CLK_ENABLE();
    OLED_SDA_GPIO_CLK_ENABLE();

    gpio.Pin = OLED_SCL_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(OLED_SCL_GPIO_Port, &gpio);

    gpio.Pin = OLED_SDA_Pin;
    HAL_GPIO_Init(OLED_SDA_GPIO_Port, &gpio);

    OLED_SCL(OLED_HIGH);
    OLED_SDA(OLED_HIGH);
    HAL_Delay(100U);

    oled_write_cmd(OLED_CMD_DISPLAY_OFF);
    oled_write_cmd(OLED_CMD_LOW_COL);
    oled_write_cmd(OLED_CMD_HIGH_COL);
    oled_write_cmd(0x40U);
    oled_write_cmd(0x81U);
    oled_write_cmd(0xCFU);
    oled_write_cmd(OLED_CMD_SEG_NORMAL);
    oled_write_cmd(OLED_CMD_COM_NORMAL);
    oled_write_cmd(OLED_CMD_NORMAL);
    oled_write_cmd(0xA8U);
    oled_write_cmd(0x3FU);
    oled_write_cmd(0xD3U);
    oled_write_cmd(0x00U);
    oled_write_cmd(0xD5U);
    oled_write_cmd(0x80U);
    oled_write_cmd(0xD9U);
    oled_write_cmd(0xF1U);
    oled_write_cmd(0xDAU);
    oled_write_cmd(0x12U);
    oled_write_cmd(0xDBU);
    oled_write_cmd(0x40U);
    oled_write_cmd(0x20U);
    oled_write_cmd(0x02U);
    oled_write_cmd(OLED_CMD_CHARGE_PUMP);
    oled_write_cmd(OLED_CMD_CHARGE_PUMP_ON);
    oled_write_cmd(0xA4U);
    oled_write_cmd(OLED_CMD_NORMAL);
    Oled_Clear();
    oled_write_cmd(OLED_CMD_DISPLAY_ON);
}

/**
 * 功能: 把显存刷到屏幕
 * 参数: None
 * 返回: None
 * 说明: 按页地址模式发送 8 页数据
 */
void Oled_Refresh(void)
{
    uint8_t page;
    uint8_t col;

    for (page = 0U; page < OLED_PAGE; page++)
    {
        oled_write_cmd((uint8_t)(OLED_CMD_PAGE_ADDR + page));
        oled_write_cmd(OLED_CMD_LOW_COL);
        oled_write_cmd(OLED_CMD_HIGH_COL);

        oled_start();
        oled_write_byte(OLED_I2C_ADDR);
        oled_wait_ack();
        oled_write_byte(OLED_CTRL_DATA);
        oled_wait_ack();
        for (col = 0U; col < OLED_WIDTH; col++)
        {
            oled_write_byte(s_gram[col][page]);
            oled_wait_ack();
        }
        oled_stop();
    }
}

/**
 * 功能: 清空显存并刷新
 * 参数: None
 * 返回: None
 * 说明: None
 */
void Oled_Clear(void)
{
    memset(s_gram, 0, sizeof(s_gram));
    Oled_Refresh();
}

/**
 * 功能: 打开或关闭显示
 * 参数: sw 为 OLED_SHOW_ON 或 OLED_SHOW_OFF
 * 返回: None
 * 说明: 同时控制电荷泵
 */
void Oled_Display(uint8_t sw)
{
    oled_write_cmd(OLED_CMD_CHARGE_PUMP);
    if (sw != OLED_SHOW_OFF)
    {
        oled_write_cmd(OLED_CMD_CHARGE_PUMP_ON);
        oled_write_cmd(OLED_CMD_DISPLAY_ON);
    }
    else
    {
        oled_write_cmd(OLED_CMD_CHARGE_PUMP_OFF);
        oled_write_cmd(OLED_CMD_DISPLAY_OFF);
    }
}

/**
 * 功能: 正常显示或反色显示
 * 参数: mode 为 0 正常，非 0 反色
 * 返回: None
 * 说明: None
 */
void Oled_ColorTurn(uint8_t mode)
{
    if (mode == 0U)
    {
        oled_write_cmd(OLED_CMD_NORMAL);
    }
    else
    {
        oled_write_cmd(OLED_CMD_INVERSE);
    }
}

/**
 * 功能: 设置显示方向
 * 参数: dir 为 OLED_DIR_NORMAL 或 OLED_DIR_TURN
 * 返回: None
 * 说明: 翻转是旋转 180 度
 */
void Oled_DisplayTurn(uint8_t dir)
{
    if (dir == OLED_DIR_NORMAL)
    {
        oled_write_cmd(OLED_CMD_COM_NORMAL);
        oled_write_cmd(OLED_CMD_SEG_NORMAL);
    }
    else
    {
        oled_write_cmd(OLED_CMD_COM_REMAP);
        oled_write_cmd(OLED_CMD_SEG_REMAP);
    }
}

/**
 * 功能: 显示 ASCII 字符串
 * 参数: x、y 起点，str 字符串，size 为 12/16/24/32
 * 返回: None
 * 说明: 超出屏幕宽高后停止。汉字请用 Oled_ShowChinese
 */
void Oled_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    uint8_t x_start;

    if (str == NULL)
    {
        return;
    }
    if ((size != 12U) && (size != 16U) && (size != 24U) && (size != 32U))
    {
        size = 16U;
    }

    x_start = x;
    while (*str != '\0')
    {
        if ((uint16_t)x + (size / 2U) > OLED_WIDTH)
        {
            x = x_start;
            y = (uint8_t)(y + size);
        }
        if ((uint16_t)y + size > OLED_HEIGHT)
        {
            break;
        }
        oled_show_ascii(x, y, *str, size);
        str++;
        x = (uint8_t)(x + size / 2U);
    }
    Oled_Refresh();
}

/**
 * 功能: 按字库序号显示一个汉字
 * 参数: x、y 左上角，index 字模下标，size 为 12/16/24/32
 * 返回: None
 * 说明: 字模在 oled_font.c，低位在前、逐列向下
 */
void Oled_ShowChinese(uint8_t x, uint8_t y, uint8_t index, uint8_t size)
{
    const uint8_t *font;
    uint16_t count;
    uint16_t i;
    uint8_t bit;
    uint8_t temp;
    uint8_t x0;
    uint8_t y0;

    x0 = x;
    y0 = y;
    count = (uint16_t)(size / 8U + ((size % 8U) != 0U ? 1U : 0U)) * size;

    switch (size)
    {
    case 12U:
        font = aFontChinese12[index];
        break;
    case 16U:
        font = aFontChinese16[index];
        break;
    case 24U:
        font = aFontChinese24[index];
        break;
    case 32U:
        font = aFontChinese32[index];
        break;
    default:
        return;
    }

    for (i = 0U; i < count; i++)
    {
        temp = font[i];
        for (bit = 0U; bit < 8U; bit++)
        {
            oled_draw_point(x, y, (temp & 0x01U) != 0U ? 1U : 0U);
            temp >>= 1;
            y++;
        }
        x++;
        if ((uint8_t)(x - x0) == size)
        {
            x = x0;
            y0 = (uint8_t)(y0 + 8U);
        }
        y = y0;
    }
    Oled_Refresh();
}

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

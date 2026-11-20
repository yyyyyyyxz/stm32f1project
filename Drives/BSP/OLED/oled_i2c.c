/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved 
 *-----------------------------------------------------------------------------
 * File name   : oled_i2c.c
 * Description : 0.96 寸 SSD1306，硬件 I2C1，PB6/PB7
 * Author      : yyyyyyyxz@gmail.com
 * Update date : 2026.11.20
 *-----------------------------------------------------------------------------
 * Note        : 与软件 I2C 的 oled.c 分开。命令宏仍用 oled.h。
 *               参与编译时还要加入 stm32f1xx_hal_i2c.c。
 *
**/

/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "./BSP/OLED/oled_i2c.h"
#include "./BSP/OLED/oled_font.h"
#include <string.h>

/*----------------------------------------------------------------------------*/
/*                                DECLARATION                                 */
/*----------------------------------------------------------------------------*/
static HAL_StatusTypeDef Oled_I2c_write(uint8_t ctrl, const uint8_t *data, uint16_t len);
static void Oled_I2c_write_cmd(uint8_t cmd);
static void Oled_I2c_draw_point(uint8_t x, uint8_t y, uint8_t on);
static void Oled_I2c_show_ascii(uint8_t x, uint8_t y, char ch, uint8_t size);

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
static I2C_HandleTypeDef s_hi2c;
static uint8_t s_gram[OLED_WIDTH][OLED_PAGE];

/*----------------------------------------------------------------------------*/
/*                                 FUNCTION                                   */
/*----------------------------------------------------------------------------*/
/**
 * 功能: 通过硬件 I2C 写命令或数据
 * 参数: ctrl 为 OLED_CTRL_CMD 或 OLED_CTRL_DATA
 * 返回: HAL 状态
 * 说明: 控制字节作为 Mem 地址发出
 */
static HAL_StatusTypeDef Oled_I2c_write(uint8_t ctrl, const uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Write(&s_hi2c,
                             OLED_I2C_ADDR,
                             ctrl,
                             I2C_MEMADD_SIZE_8BIT,
                             (uint8_t *)data,
                             len,
                             OLED_HW_I2C_TIMEOUT);
}

/**
 * 功能: 写一条 SSD1306 命令
 * 参数: cmd 命令字节
 * 返回: None
 * 说明: None
 */
static void Oled_I2c_write_cmd(uint8_t cmd)
{
    (void)Oled_I2c_write(OLED_CTRL_CMD, &cmd, 1U);
}

/**
 * 功能: 在显存中画一个点
 * 参数: x 列 0~127，y 行 0~63，on 为 1 点亮
 * 返回: None
 * 说明: 不立即刷新屏幕
 */
static void Oled_I2c_draw_point(uint8_t x, uint8_t y, uint8_t on)
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
static void Oled_I2c_show_ascii(uint8_t x, uint8_t y, char ch, uint8_t size)
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
            Oled_I2c_draw_point(x, y, (temp & 0x80U) != 0U ? 1U : 0U);
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
 * 功能: 初始化 I2C1 和 SSD1306
 * 参数: None
 * 返回: None
 * 说明: PB6 为 SCL，PB7 为 SDA，复用开漏
 */
void Oled_I2c_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    OLED_HW_GPIO_CLK_ENABLE();
    OLED_HW_I2C_CLK_ENABLE();

    gpio.Pin = OLED_HW_SCL_Pin | OLED_HW_SDA_Pin;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(OLED_HW_SCL_GPIO_Port, &gpio);

    s_hi2c.Instance = OLED_HW_I2C;
    s_hi2c.Init.ClockSpeed = OLED_HW_I2C_SPEED;
    s_hi2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
    s_hi2c.Init.OwnAddress1 = 0U;
    s_hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    s_hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    s_hi2c.Init.OwnAddress2 = 0U;
    s_hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    s_hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&s_hi2c);

    HAL_Delay(100U);

    Oled_I2c_write_cmd(OLED_CMD_DISPLAY_OFF);
    Oled_I2c_write_cmd(OLED_CMD_LOW_COL);
    Oled_I2c_write_cmd(OLED_CMD_HIGH_COL);
    Oled_I2c_write_cmd(0x40U);
    Oled_I2c_write_cmd(0x81U);
    Oled_I2c_write_cmd(0xCFU);
    Oled_I2c_write_cmd(OLED_CMD_SEG_NORMAL);
    Oled_I2c_write_cmd(OLED_CMD_COM_NORMAL);
    Oled_I2c_write_cmd(OLED_CMD_NORMAL);
    Oled_I2c_write_cmd(0xA8U);
    Oled_I2c_write_cmd(0x3FU);
    Oled_I2c_write_cmd(0xD3U);
    Oled_I2c_write_cmd(0x00U);
    Oled_I2c_write_cmd(0xD5U);
    Oled_I2c_write_cmd(0x80U);
    Oled_I2c_write_cmd(0xD9U);
    Oled_I2c_write_cmd(0xF1U);
    Oled_I2c_write_cmd(0xDAU);
    Oled_I2c_write_cmd(0x12U);
    Oled_I2c_write_cmd(0xDBU);
    Oled_I2c_write_cmd(0x40U);
    Oled_I2c_write_cmd(0x20U);
    Oled_I2c_write_cmd(0x02U);
    Oled_I2c_write_cmd(OLED_CMD_CHARGE_PUMP);
    Oled_I2c_write_cmd(OLED_CMD_CHARGE_PUMP_ON);
    Oled_I2c_write_cmd(0xA4U);
    Oled_I2c_write_cmd(OLED_CMD_NORMAL);
    Oled_I2c_Clear();
    Oled_I2c_write_cmd(OLED_CMD_DISPLAY_ON);
}

/**
 * 功能: 把显存刷到屏幕
 * 参数: None
 * 返回: None
 * 说明: 每一页一次发送 128 字节
 */
void Oled_I2c_Refresh(void)
{
    uint8_t page;
    uint8_t col;
    uint8_t line[OLED_WIDTH];

    for (page = 0U; page < OLED_PAGE; page++)
    {
        Oled_I2c_write_cmd((uint8_t)(OLED_CMD_PAGE_ADDR + page));
        Oled_I2c_write_cmd(OLED_CMD_LOW_COL);
        Oled_I2c_write_cmd(OLED_CMD_HIGH_COL);

        for (col = 0U; col < OLED_WIDTH; col++)
        {
            line[col] = s_gram[col][page];
        }
        (void)Oled_I2c_write(OLED_CTRL_DATA, line, OLED_WIDTH);
    }
}

/**
 * 功能: 清空显存并刷新
 * 参数: None
 * 返回: None
 * 说明: None
 */
void Oled_I2c_Clear(void)
{
    memset(s_gram, 0, sizeof(s_gram));
    Oled_I2c_Refresh();
}

/**
 * 功能: 打开或关闭显示
 * 参数: sw 为 OLED_SHOW_ON 或 OLED_SHOW_OFF
 * 返回: None
 * 说明: 同时控制电荷泵
 */
void Oled_I2c_Display(uint8_t sw)
{
    Oled_I2c_write_cmd(OLED_CMD_CHARGE_PUMP);
    if (sw != OLED_SHOW_OFF)
    {
        Oled_I2c_write_cmd(OLED_CMD_CHARGE_PUMP_ON);
        Oled_I2c_write_cmd(OLED_CMD_DISPLAY_ON);
    }
    else
    {
        Oled_I2c_write_cmd(OLED_CMD_CHARGE_PUMP_OFF);
        Oled_I2c_write_cmd(OLED_CMD_DISPLAY_OFF);
    }
}

/**
 * 功能: 正常显示或反色显示
 * 参数: mode 为 0 正常，非 0 反色
 * 返回: None
 * 说明: None
 */
void Oled_I2c_ColorTurn(uint8_t mode)
{
    if (mode == 0U)
    {
        Oled_I2c_write_cmd(OLED_CMD_NORMAL);
    }
    else
    {
        Oled_I2c_write_cmd(OLED_CMD_INVERSE);
    }
}

/**
 * 功能: 设置显示方向
 * 参数: dir 为 OLED_DIR_NORMAL 或 OLED_DIR_TURN
 * 返回: None
 * 说明: 翻转是旋转 180 度
 */
void Oled_I2c_DisplayTurn(uint8_t dir)
{
    if (dir == OLED_DIR_NORMAL)
    {
        Oled_I2c_write_cmd(OLED_CMD_COM_NORMAL);
        Oled_I2c_write_cmd(OLED_CMD_SEG_NORMAL);
    }
    else
    {
        Oled_I2c_write_cmd(OLED_CMD_COM_REMAP);
        Oled_I2c_write_cmd(OLED_CMD_SEG_REMAP);
    }
}

/**
 * 功能: 显示 ASCII 字符串
 * 参数: x、y 起点，str 字符串，size 为 12/16/24/32
 * 返回: None
 * 说明: 超出屏幕宽高后停止。汉字请用 Oled_I2c_ShowChinese
 */
void Oled_I2c_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size)
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
        Oled_I2c_show_ascii(x, y, *str, size);
        str++;
        x = (uint8_t)(x + size / 2U);
    }
    Oled_I2c_Refresh();
}

/**
 * 功能: 按字库序号显示一个汉字
 * 参数: x、y 左上角，index 字模下标，size 为 12/16/24/32
 * 返回: None
 * 说明: 字模在 oled_font.c，低位在前、逐列向下
 */
void Oled_I2c_ShowChinese(uint8_t x, uint8_t y, uint8_t index, uint8_t size)
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
            Oled_I2c_draw_point(x, y, (temp & 0x01U) != 0U ? 1U : 0U);
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
    Oled_I2c_Refresh();
}

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

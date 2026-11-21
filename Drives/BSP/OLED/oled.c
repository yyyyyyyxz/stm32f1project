/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved
 *-----------------------------------------------------------------------------
 * File name   : oled.c
 * Description : 0.96 寸 SSD1306，软件 I2C 与硬件 I2C
 * Author      : yyyyyyyxz@gmail.com
 * Update date : 2026.11.21
 *-----------------------------------------------------------------------------
 * Note        : Oled_Init(mode, scl, sda) 登记一块屏。引脚在 OLED_GPIO_Port。
 *               软件 I2C 可任选两脚。硬件 I2C 只认 PB6/PB7、PB8/PB9、PB10/PB11。
 *               多块屏共用显存，刷新时逐块送出。
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
typedef struct
{
    uint8_t used;
    uint8_t mode;
    uint16_t scl_pin;
    uint16_t sda_pin;
    I2C_HandleTypeDef i2c;
} OledLink;

static void oled_delay(void);
static void oled_start(const OledLink *link);
static void oled_stop(const OledLink *link);
static void oled_wait_ack(const OledLink *link);
static void oled_write_byte(const OledLink *link, uint8_t data);
static void oled_soft_write_cmd(const OledLink *link, uint8_t cmd);
static void oled_soft_write_data(const OledLink *link, const uint8_t *data, uint16_t len);
static void oled_hard_write(OledLink *link, uint8_t ctrl, const uint8_t *data, uint16_t len);
static I2C_TypeDef *oled_i2c_inst(uint16_t scl_pin, uint16_t sda_pin);
static OledLink *oled_link_get(uint8_t mode, uint16_t scl_pin, uint16_t sda_pin);
static void oled_write_cmd_to(OledLink *link, uint8_t cmd);
static void oled_write_data_to(OledLink *link, const uint8_t *data, uint16_t len);
static void oled_write_cmd(uint8_t cmd);
static void oled_refresh_to(OledLink *link);
static void oled_panel_init(OledLink *link);
static uint8_t oled_mode_ok(uint8_t mode);
static void oled_draw_point(uint8_t x, uint8_t y, uint8_t on);
static void oled_show_ascii(uint8_t x, uint8_t y, char ch, uint8_t size);

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
static uint8_t s_gram[OLED_WIDTH][OLED_PAGE];
static OledLink s_link[OLED_SCREEN_MAX];

/*----------------------------------------------------------------------------*/
/*                                 FUNCTION                                   */
/*----------------------------------------------------------------------------*/
/**
 * 功能: 判断初始化参数是否有效
 * 参数: mode 为 OLED_MODE_SOFT 或 OLED_MODE_HARD
 * 返回: 1 有效，0 无效
 * 说明: None
 */
static uint8_t oled_mode_ok(uint8_t mode)
{
    return (uint8_t)((mode == OLED_MODE_SOFT) || (mode == OLED_MODE_HARD));
}

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
 * 功能: 发送软件 I2C 起始信号
 * 参数: link 目标屏，使用其中的 SCL/SDA
 * 返回: None
 * 说明: 电平由 OLED_PIN 宏写出
 */
static void oled_start(const OledLink *link)
{
    OLED_PIN(link->sda_pin, OLED_HIGH);
    OLED_PIN(link->scl_pin, OLED_HIGH);
    oled_delay();
    OLED_PIN(link->sda_pin, OLED_LOW);
    oled_delay();
    OLED_PIN(link->scl_pin, OLED_LOW);
    oled_delay();
}

/**
 * 功能: 发送软件 I2C 停止信号
 * 参数: link 目标屏
 * 返回: None
 * 说明: None
 */
static void oled_stop(const OledLink *link)
{
    OLED_PIN(link->sda_pin, OLED_LOW);
    OLED_PIN(link->scl_pin, OLED_HIGH);
    oled_delay();
    OLED_PIN(link->sda_pin, OLED_HIGH);
}

/**
 * 功能: 给出应答时钟，不读取 SDA
 * 参数: link 目标屏
 * 返回: None
 * 说明: SSD1306 作为从机，这里只完成第 9 个时钟
 */
static void oled_wait_ack(const OledLink *link)
{
    OLED_PIN(link->sda_pin, OLED_HIGH);
    oled_delay();
    OLED_PIN(link->scl_pin, OLED_HIGH);
    oled_delay();
    OLED_PIN(link->scl_pin, OLED_LOW);
    oled_delay();
}

/**
 * 功能: 软件 I2C 发送一个字节，高位在前
 * 参数: link 目标屏，data 待发送数据
 * 返回: None
 * 说明: None
 */
static void oled_write_byte(const OledLink *link, uint8_t data)
{
    uint8_t bit;

    for (bit = 0U; bit < 8U; bit++)
    {
        if ((data & 0x80U) != 0U)
        {
            OLED_PIN(link->sda_pin, OLED_HIGH);
        }
        else
        {
            OLED_PIN(link->sda_pin, OLED_LOW);
        }
        oled_delay();
        OLED_PIN(link->scl_pin, OLED_HIGH);
        oled_delay();
        OLED_PIN(link->scl_pin, OLED_LOW);
        data <<= 1;
    }
}

/**
 * 功能: 软件 I2C 写一条命令
 * 参数: link 目标屏，cmd 命令字节
 * 返回: None
 * 说明: 控制字节使用 OLED_CTRL_CMD
 */
static void oled_soft_write_cmd(const OledLink *link, uint8_t cmd)
{
    oled_start(link);
    oled_write_byte(link, OLED_I2C_ADDR);
    oled_wait_ack(link);
    oled_write_byte(link, OLED_CTRL_CMD);
    oled_wait_ack(link);
    oled_write_byte(link, cmd);
    oled_wait_ack(link);
    oled_stop(link);
}

/**
 * 功能: 软件 I2C 写一串显示数据
 * 参数: link 目标屏，data 数据，len 字节数
 * 返回: None
 * 说明: 控制字节使用 OLED_CTRL_DATA
 */
static void oled_soft_write_data(const OledLink *link, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    oled_start(link);
    oled_write_byte(link, OLED_I2C_ADDR);
    oled_wait_ack(link);
    oled_write_byte(link, OLED_CTRL_DATA);
    oled_wait_ack(link);
    for (i = 0U; i < len; i++)
    {
        oled_write_byte(link, data[i]);
        oled_wait_ack(link);
    }
    oled_stop(link);
}

/**
 * 功能: 按引脚选出硬件 I2C
 * 参数: scl_pin、sda_pin
 * 返回: I2C1、I2C2，引脚不是硬件脚时返回 NULL
 * 说明: PB6/PB7 与 PB8/PB9 是 I2C1，PB10/PB11 是 I2C2
 */
static I2C_TypeDef *oled_i2c_inst(uint16_t scl_pin, uint16_t sda_pin)
{
    if (((scl_pin == GPIO_PIN_6) && (sda_pin == GPIO_PIN_7)) ||
        ((scl_pin == GPIO_PIN_8) && (sda_pin == GPIO_PIN_9)))
    {
        return I2C1;
    }
    if ((scl_pin == GPIO_PIN_10) && (sda_pin == GPIO_PIN_11))
    {
        return I2C2;
    }
    return NULL;
}

/**
 * 功能: 找到已有屏，或取一个空位
 * 参数: mode、scl_pin、sda_pin
 * 返回: 屏槽指针，没有空位时返回 NULL
 * 说明: 同一对引脚再次初始化会复用原槽
 */
static OledLink *oled_link_get(uint8_t mode, uint16_t scl_pin, uint16_t sda_pin)
{
    uint8_t i;
    OledLink *free_link;
    I2C_TypeDef *inst;

    free_link = NULL;
    for (i = 0U; i < OLED_SCREEN_MAX; i++)
    {
        if ((s_link[i].used != 0U) &&
            (s_link[i].scl_pin == scl_pin) &&
            (s_link[i].sda_pin == sda_pin))
        {
            return &s_link[i];
        }
        if ((s_link[i].used == 0U) && (free_link == NULL))
        {
            free_link = &s_link[i];
        }
    }

    if (mode != OLED_MODE_HARD)
    {
        return free_link;
    }

    inst = oled_i2c_inst(scl_pin, sda_pin);
    for (i = 0U; i < OLED_SCREEN_MAX; i++)
    {
        if ((s_link[i].used != 0U) &&
            (s_link[i].mode == OLED_MODE_HARD) &&
            (s_link[i].i2c.Instance == inst))
        {
            return NULL;
        }
    }
    return free_link;
}

/**
 * 功能: 硬件 I2C 写命令或数据
 * 参数: link 目标屏，ctrl 为 OLED_CTRL_CMD 或 OLED_CTRL_DATA
 * 返回: None
 * 说明: 控制字节作为 Mem 地址发出
 */
static void oled_hard_write(OledLink *link, uint8_t ctrl, const uint8_t *data, uint16_t len)
{
    (void)HAL_I2C_Mem_Write(&link->i2c,
                            OLED_I2C_ADDR,
                            ctrl,
                            I2C_MEMADD_SIZE_8BIT,
                            (uint8_t *)data,
                            len,
                            OLED_I2C_TIMEOUT);
}

/**
 * 功能: 向指定屏写一条命令
 * 参数: link 目标屏，cmd 命令字节
 * 返回: None
 * 说明: None
 */
static void oled_write_cmd_to(OledLink *link, uint8_t cmd)
{
    if (link->mode == OLED_MODE_SOFT)
    {
        oled_soft_write_cmd(link, cmd);
    }
    else
    {
        oled_hard_write(link, OLED_CTRL_CMD, &cmd, 1U);
    }
}

/**
 * 功能: 向指定屏写显示数据
 * 参数: link 目标屏，data 数据，len 字节数
 * 返回: None
 * 说明: None
 */
static void oled_write_data_to(OledLink *link, const uint8_t *data, uint16_t len)
{
    if (link->mode == OLED_MODE_SOFT)
    {
        oled_soft_write_data(link, data, len);
    }
    else
    {
        oled_hard_write(link, OLED_CTRL_DATA, data, len);
    }
}

/**
 * 功能: 向所有已初始化的屏写一条命令
 * 参数: cmd 命令字节
 * 返回: None
 * 说明: None
 */
static void oled_write_cmd(uint8_t cmd)
{
    uint8_t i;

    for (i = 0U; i < OLED_SCREEN_MAX; i++)
    {
        if (s_link[i].used != 0U)
        {
            oled_write_cmd_to(&s_link[i], cmd);
        }
    }
}

/**
 * 功能: 把显存刷到指定屏
 * 参数: link 目标屏
 * 返回: None
 * 说明: 按页地址模式发送 8 页数据
 */
static void oled_refresh_to(OledLink *link)
{
    uint8_t page;
    uint8_t col;
    uint8_t line[OLED_WIDTH];

    for (page = 0U; page < OLED_PAGE; page++)
    {
        oled_write_cmd_to(link, (uint8_t)(OLED_CMD_PAGE_ADDR + page));
        oled_write_cmd_to(link, OLED_CMD_LOW_COL);
        oled_write_cmd_to(link, OLED_CMD_HIGH_COL);

        for (col = 0U; col < OLED_WIDTH; col++)
        {
            line[col] = s_gram[col][page];
        }
        oled_write_data_to(link, line, OLED_WIDTH);
    }
}

/**
 * 功能: 写入 SSD1306 初始化序列并点亮指定屏
 * 参数: link 目标屏
 * 返回: None
 * 说明: 调用前该屏的引脚和总线必须已经就绪
 */
static void oled_panel_init(OledLink *link)
{
    oled_write_cmd_to(link, OLED_CMD_DISPLAY_OFF);
    oled_write_cmd_to(link, OLED_CMD_LOW_COL);
    oled_write_cmd_to(link, OLED_CMD_HIGH_COL);
    oled_write_cmd_to(link, 0x40U);
    oled_write_cmd_to(link, 0x81U);
    oled_write_cmd_to(link, 0xCFU);
    oled_write_cmd_to(link, OLED_CMD_SEG_NORMAL);
    oled_write_cmd_to(link, OLED_CMD_COM_NORMAL);
    oled_write_cmd_to(link, OLED_CMD_NORMAL);
    oled_write_cmd_to(link, 0xA8U);
    oled_write_cmd_to(link, 0x3FU);
    oled_write_cmd_to(link, 0xD3U);
    oled_write_cmd_to(link, 0x00U);
    oled_write_cmd_to(link, 0xD5U);
    oled_write_cmd_to(link, 0x80U);
    oled_write_cmd_to(link, 0xD9U);
    oled_write_cmd_to(link, 0xF1U);
    oled_write_cmd_to(link, 0xDAU);
    oled_write_cmd_to(link, 0x12U);
    oled_write_cmd_to(link, 0xDBU);
    oled_write_cmd_to(link, 0x40U);
    oled_write_cmd_to(link, 0x20U);
    oled_write_cmd_to(link, 0x02U);
    oled_write_cmd_to(link, OLED_CMD_CHARGE_PUMP);
    oled_write_cmd_to(link, OLED_CMD_CHARGE_PUMP_ON);
    oled_write_cmd_to(link, 0xA4U);
    oled_write_cmd_to(link, OLED_CMD_NORMAL);
    memset(s_gram, 0, sizeof(s_gram));
    Oled_Refresh();
    oled_write_cmd_to(link, OLED_CMD_DISPLAY_ON);
}

/**
 * 功能: 在显存中画一个点
 * 参数: x 列 0~127，y 行 0~63，on 为 1 点亮
 * 返回: None
 * 说明: 不立即刷新屏幕。已初始化的屏共用这块显存
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
 * 功能: 初始化一块 OLED
 * 参数: mode 为 OLED_MODE_SOFT 或 OLED_MODE_HARD，scl_pin、sda_pin 为 SCL 和 SDA
 * 返回: None
 * 说明: 每对引脚一块屏，最多 OLED_SCREEN_MAX 块。端口是 OLED_GPIO_Port
 */
void Oled_Init(uint8_t mode, uint16_t scl_pin, uint16_t sda_pin)
{
    GPIO_InitTypeDef gpio = {0};
    OledLink *link;
    I2C_TypeDef *inst;

    if ((oled_mode_ok(mode) == 0U) || (scl_pin == 0U) || (sda_pin == 0U) || (scl_pin == sda_pin))
    {
        return;
    }

    inst = oled_i2c_inst(scl_pin, sda_pin);
    if ((mode == OLED_MODE_HARD) && (inst == NULL))
    {
        return;
    }

    link = oled_link_get(mode, scl_pin, sda_pin);
    if (link == NULL)
    {
        return;
    }

    link->mode = mode;
    link->scl_pin = scl_pin;
    link->sda_pin = sda_pin;
    OLED_GPIO_CLK_ENABLE();

    gpio.Pin = scl_pin | sda_pin;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    if (mode == OLED_MODE_SOFT)
    {
        gpio.Mode = GPIO_MODE_OUTPUT_OD;
        HAL_GPIO_Init(OLED_GPIO_Port, &gpio);
        OLED_PIN(scl_pin, OLED_HIGH);
        OLED_PIN(sda_pin, OLED_HIGH);
    }
    else
    {
        gpio.Mode = GPIO_MODE_AF_OD;
        HAL_GPIO_Init(OLED_GPIO_Port, &gpio);

        if (inst == I2C1)
        {
            OLED_AFIO_CLK_ENABLE();
            OLED_I2C1_CLK_ENABLE();
            if ((scl_pin == GPIO_PIN_8) && (sda_pin == GPIO_PIN_9))
            {
                __HAL_AFIO_REMAP_I2C1_ENABLE();
            }
            else
            {
                __HAL_AFIO_REMAP_I2C1_DISABLE();
            }
        }
        else
        {
            OLED_I2C2_CLK_ENABLE();
        }

        link->i2c.Instance = inst;
        link->i2c.Init.ClockSpeed = OLED_I2C_SPEED;
        link->i2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
        link->i2c.Init.OwnAddress1 = 0U;
        link->i2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
        link->i2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
        link->i2c.Init.OwnAddress2 = 0U;
        link->i2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
        link->i2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
        HAL_I2C_Init(&link->i2c);
    }

    link->used = 1U;
    HAL_Delay(100U);
    oled_panel_init(link);
}

/**
 * 功能: 把显存刷到所有已初始化的屏幕
 * 参数: None
 * 返回: None
 * 说明: 按页地址模式发送 8 页数据
 */
void Oled_Refresh(void)
{
    uint8_t i;

    for (i = 0U; i < OLED_SCREEN_MAX; i++)
    {
        if (s_link[i].used != 0U)
        {
            oled_refresh_to(&s_link[i]);
        }
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
 * 说明: 同时控制电荷泵。已初始化的总线都会收到命令
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

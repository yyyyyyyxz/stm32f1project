/*------------------------------------------------------------------------------
 * File name   : oled_font.h
 * Author      : yyyyyyyxz@gmail.com
 * Copyright(C): yyyyyyyxz@gmail.com. All rights reserved.
 *------------------------------------------------------------------------------
 */
#ifndef __OLED_FONT_H__
#define __OLED_FONT_H__
/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include <stdint.h>

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
/* ASCII：空格到 '~'，共 95 个。取模为纵向、高位在前 */
#define OLED_FONT_ASCII_COUNT          95U
#define OLED_FONT_ASCII12_BYTES        12U
#define OLED_FONT_ASCII16_BYTES        16U
#define OLED_FONT_ASCII24_BYTES        36U
#define OLED_FONT_ASCII32_BYTES        64U

/* 汉字：纵向、低位在前。序号见下面的宏 */
#define OLED_FONT_CN12_COUNT           4U
#define OLED_FONT_CN12_BYTES           24U
#define OLED_FONT_CN16_COUNT           5U
#define OLED_FONT_CN16_BYTES           32U
#define OLED_FONT_CN24_COUNT           4U
#define OLED_FONT_CN24_BYTES           72U
#define OLED_FONT_CN32_COUNT           4U
#define OLED_FONT_CN32_BYTES           128U
#define OLED_FONT_CN64_COUNT           3U
#define OLED_FONT_CN64_BYTES           512U

/* 12 / 24 / 32 号：温度显示 */
#define OLED_FONT_CN_WEN               0U
#define OLED_FONT_CN_DU                1U
#define OLED_FONT_CN_XIAN              2U
#define OLED_FONT_CN_SHI               3U

/* 16 号：温湿度采集。下标与上面的 12 号不同 */
#define OLED_FONT_CN16_WEN             0U
#define OLED_FONT_CN16_SHI             1U
#define OLED_FONT_CN16_DU              2U
#define OLED_FONT_CN16_CAI             3U
#define OLED_FONT_CN16_JI              4U

/* 64 号：同、色、异。当前显示函数未使用这档 */
#define OLED_FONT_CN64_TONG            0U
#define OLED_FONT_CN64_SE              1U
#define OLED_FONT_CN64_YI              2U

extern const uint8_t aFontASCII12[OLED_FONT_ASCII_COUNT][OLED_FONT_ASCII12_BYTES];
extern const uint8_t aFontASCII16[OLED_FONT_ASCII_COUNT][OLED_FONT_ASCII16_BYTES];
extern const uint8_t aFontASCII24[OLED_FONT_ASCII_COUNT][OLED_FONT_ASCII24_BYTES];
extern const uint8_t aFontASCII32[OLED_FONT_ASCII_COUNT][OLED_FONT_ASCII32_BYTES];

extern const uint8_t aFontChinese12[OLED_FONT_CN12_COUNT][OLED_FONT_CN12_BYTES];
extern const uint8_t aFontChinese16[OLED_FONT_CN16_COUNT][OLED_FONT_CN16_BYTES];
extern const uint8_t aFontChinese24[OLED_FONT_CN24_COUNT][OLED_FONT_CN24_BYTES];
extern const uint8_t aFontChinese32[OLED_FONT_CN32_COUNT][OLED_FONT_CN32_BYTES];
extern const uint8_t aFontChinese64[OLED_FONT_CN64_COUNT][OLED_FONT_CN64_BYTES];

#endif /* __OLED_FONT_H__ */

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

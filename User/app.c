/*
 *-----------------------------------------------------------------------------
 *            (C) Copyright yyyyyyyxz@gmail.com. All Rights Reserved
 *-----------------------------------------------------------------------------
 * File name   : app.c
 * Description : 网球小车控制
 * Author      : yyyyyyyxz@gmail.com
 * Update date : 2026.11.23
 *-----------------------------------------------------------------------------
 * Note        : 上位机走 USART3，一行一条，以 \n 结束。
 *               PING / STOP / MODE,n / MOVE,vx,vy,wz / PID,kp,ki,kd / BASE,n
 *               SERVO,angle / GRAB / INV,left,right / FLIP,0|1 / SEEK,0|1
 *               THR,lmin,lmax,amin,amax,bmin,bmax / RST / QUERY
 *               MODE：0 停止，1 手动，2 追踪，3 巡场捡球。
 *               MOVE 为麦轮，前正、右移正、顺时针正，各 -100..100。400ms 没再收到就停车。
 *               PID 单位 0.01。回传 PONG、ACK、ERR、STAT、BLOBS。
 *               OpenMV 发来 @n,x,y,w,h,pixels,...，坐标是 QQVGA 160x120。
 *
**/

/*----------------------------------------------------------------------------*/
/*                                 INCLUDES                                   */
/*----------------------------------------------------------------------------*/
#include "app.h"
#include "init.h"
#include "./BSP/MOTOR/motor.h"
#include "./BSP/SERVO/servo.h"
#include "./BSP/SONAR/sonar.h"
#include "./BSP/LINK/link.h"
#include <stdio.h>
#include <string.h>

/*----------------------------------------------------------------------------*/
/*                                DECLARATION                                 */
/*----------------------------------------------------------------------------*/
typedef struct
{
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    int16_t area;
} Blob;

static int app_abs(int value);
static int16_t app_clamp(int value);
static int app_parse_int(const char **pp);
static int app_starts(const char *text, const char *key);
static void app_reply(const char *text);
static void app_choose_target(void);
static void app_parse_cam(const char *line);
static void app_begin_grab(uint32_t now);
static void app_track(uint32_t now, int16_t *vx, int16_t *vy, int16_t *wz);
static void app_step_action(uint32_t now, int16_t *vx, int16_t *vy, int16_t *wz);
static void app_mecanum(int vx, int vy, int wz, int16_t *fl, int16_t *fr, int16_t *rl, int16_t *rr);
static void app_apply(int16_t vx, int16_t vy, int16_t wz);
static void app_control(uint32_t now);
static void app_send_telem(void);
static void app_handle_host(char *line);
static void app_pad16(char *text);
static void app_paint_oled(void);

/*----------------------------------------------------------------------------*/
/*                                   DATA                                     */
/*----------------------------------------------------------------------------*/
#define CAM_W                       160
#define CAM_CX                      80
#define BLOB_MAX                    5
#define MODE_STOP                   0
#define MODE_MANUAL                 1
#define MODE_TRACK                  2
#define MODE_SWEEP                  3

#define ACT_IDLE                    0
#define ACT_SCOOP                   1
#define ACT_LIFT                    2
#define ACT_SPIN                    3

static Blob s_blob[BLOB_MAX];
static uint8_t s_blob_n;
static int8_t s_tgt = -1;
static uint8_t s_found;
static int16_t s_cx;
static int16_t s_cy;
static int16_t s_area;
static uint32_t s_cam_ms;

static uint8_t s_mode = MODE_STOP;
static uint8_t s_act = ACT_IDLE;
static uint32_t s_act_ms;
static uint16_t s_picked;
static uint16_t s_cm;
static int16_t s_cmd_vx;
static int16_t s_cmd_vy;
static int16_t s_cmd_wz;
static int16_t s_vx;
static int16_t s_vy;
static int16_t s_wz;
static int16_t s_fl;
static int16_t s_fr;
static int16_t s_rl;
static int16_t s_rr;
static uint8_t s_flip_y;
static uint32_t s_move_ms;
static uint8_t s_inv_l;
static uint8_t s_inv_r;
static uint8_t s_seek = 1;
static int16_t s_base = 42;
static int16_t s_kp = 45;
static int16_t s_ki = 0;
static int16_t s_kd = 12;
static int32_t s_isum;
static int16_t s_perr;
static uint32_t s_cool_ms;

/*----------------------------------------------------------------------------*/
/*                                 FUNCTION                                   */
/*----------------------------------------------------------------------------*/
/**
 * 功能: 取绝对值
 * 参数: value 有符号整数
 * 返回: 绝对值
 * 说明: None
 */
static int app_abs(int value)
{
    return (value < 0) ? -value : value;
}

/**
 * 功能: 把速度限制在 -100..100
 * 参数: value 原始值
 * 返回: 限制后的速度
 * 说明: None
 */
static int16_t app_clamp(int value)
{
    if (value > 100)
    {
        return 100;
    }
    if (value < -100)
    {
        return -100;
    }
    return (int16_t)value;
}

/**
 * 功能: 从一行文本里取出下一个整数
 * 参数: pp 指向当前扫描位置，调用后前移
 * 返回: 整数，没有数字时返回 0
 * 说明: 允许前导空格和逗号
 */
static int app_parse_int(const char **pp)
{
    const char *text = *pp;
    int sign = 1;
    int value = 0;
    int any = 0;

    while ((*text == ' ') || (*text == ','))
    {
        text++;
    }
    if (*text == '-')
    {
        sign = -1;
        text++;
    }
    else if (*text == '+')
    {
        text++;
    }
    while ((*text >= '0') && (*text <= '9'))
    {
        value = (value * 10) + (*text - '0');
        text++;
        any = 1;
    }
    *pp = text;
    if (!any)
    {
        return 0;
    }
    return sign * value;
}

/**
 * 功能: 判断一行是不是以某个命令开头
 * 参数: text 收到的行，key 命令字
 * 返回: 1 是，0 不是
 * 说明: None
 */
static int app_starts(const char *text, const char *key)
{
    while (*key != '\0')
    {
        if (*text++ != *key++)
        {
            return 0;
        }
    }
    return 1;
}

/**
 * 功能: 向上位机回一行
 * 参数: text 以 0 结尾，调用者自己带换行
 * 返回: None
 * 说明: None
 */
static void app_reply(const char *text)
{
    Link_HostSend(text);
}

/**
 * 功能: 从色块列表里选一个目标
 * 参数: None
 * 返回: None
 * 说明: 追踪时优先锁住附近的色块。巡场时面积大、靠近中线、更靠下的优先
 */
static void app_choose_target(void)
{
    int i;
    int best = -1;
    int best_score = -1000000;
    int lock = -1;
    int lock_d = 100000;

    s_found = 0;
    s_tgt = -1;
    if (s_blob_n == 0U)
    {
        return;
    }

    if (s_cx != 0 || s_cy != 0)
    {
        for (i = 0; i < (int)s_blob_n; i++)
        {
            int dist = app_abs(s_blob[i].x - s_cx) + app_abs(s_blob[i].y - s_cy);
            if ((dist < 36) && (dist < lock_d))
            {
                lock_d = dist;
                lock = i;
            }
        }
    }

    if ((lock >= 0) && (s_mode != MODE_SWEEP))
    {
        best = lock;
    }
    else
    {
        for (i = 0; i < (int)s_blob_n; i++)
        {
            /* 面积大的更近；偏离中心要多移动，扣分。巡场时靠下的更近，加分 */
            int score = s_blob[i].area - app_abs(s_blob[i].x - CAM_CX) * 12;
            if (s_mode == MODE_SWEEP)
            {
                score += s_blob[i].y;
            }
            if (score > best_score)
            {
                best_score = score;
                best = i;
            }
        }
    }

    if (best >= 0)
    {
        s_tgt = (int8_t)best;
        s_cx = s_blob[best].x;
        s_cy = s_blob[best].y;
        s_area = s_blob[best].area;
        s_found = 1;
    }
}

/**
 * 功能: 解析 OpenMV 发来的一行色块
 * 参数: line 以 @ 开头
 * 返回: None
 * 说明: 格式 @n,x,y,w,h,pixels,...
 */
static void app_parse_cam(const char *line)
{
    const char *text;
    int count;
    int i;

    if (line[0] != '@')
    {
        return;
    }
    text = line + 1;
    count = app_parse_int(&text);
    if (count < 0)
    {
        count = 0;
    }
    if (count > BLOB_MAX)
    {
        count = BLOB_MAX;
    }
    for (i = 0; i < count; i++)
    {
        s_blob[i].x = (int16_t)app_parse_int(&text);
        s_blob[i].y = (int16_t)app_parse_int(&text);
        s_blob[i].w = (int16_t)app_parse_int(&text);
        s_blob[i].h = (int16_t)app_parse_int(&text);
        s_blob[i].area = (int16_t)app_parse_int(&text);
    }
    s_blob_n = (uint8_t)count;
    s_cam_ms = HAL_GetTick();
    if (count == 0)
    {
        s_found = 0;
        s_tgt = -1;
        s_area = 0;
    }
    else
    {
        app_choose_target();
    }
}

/**
 * 功能: 开始一次捡球
 * 参数: now 当前毫秒
 * 返回: None
 * 说明: 舵机先收到铲球角度
 */
static void app_begin_grab(uint32_t now)
{
    s_act = ACT_SCOOP;
    s_act_ms = now + 380U;
    Servo_SetAngle(165);
    s_isum = 0;
}

/**
 * 功能: 按色块偏差算出前进、平移和转向
 * 参数: now 当前毫秒，vx/vy/wz 输出
 * 返回: None
 * 说明: 目标在右边时向右平移，并带一点顺时针转向。丢失目标时按 SEEK 原地转
 */
static void app_track(uint32_t now, int16_t *vx, int16_t *vy, int16_t *wz)
{
    int err;
    int derr;
    int strafe;
    int base;

    if ((s_found == 0U) || ((now - s_cam_ms) > 500U))
    {
        s_found = 0;
        s_isum = 0;
        *vx = 0;
        *vy = 0;
        *wz = s_seek ? 28 : 0;
        return;
    }

    err = (int)s_cx - CAM_CX;
    if (app_abs(err) < 6)
    {
        err = 0;
    }
    s_isum += err;
    if (s_isum > 2000)
    {
        s_isum = 2000;
    }
    if (s_isum < -2000)
    {
        s_isum = -2000;
    }
    derr = err - s_perr;
    s_perr = (int16_t)err;
    strafe = (int)((s_kp * err) + (s_ki * s_isum) + (s_kd * derr)) / 100;
    strafe = app_clamp(strafe);

    base = s_base;
    if (app_abs(strafe) > 35)
    {
        base = base * (100 - (app_abs(strafe) - 35)) / 100;
        if (base < 12)
        {
            base = 12;
        }
    }
    if (s_area > 1600)
    {
        base = base * 2 / 3;
    }
    if (s_area > 2800)
    {
        base = base / 2;
    }

    if ((s_act == ACT_IDLE) && ((now - s_cool_ms) > 1200U))
    {
        if ((s_area >= 4200) || ((s_cm > 0U) && (s_cm < 16U) && (s_area > 700)))
        {
            app_begin_grab(now);
            *vx = 0;
            *vy = 0;
            *wz = 0;
            return;
        }
    }

    *vx = (int16_t)base;
    *vy = (int16_t)strafe;
    *wz = (int16_t)app_clamp(strafe / 4);
}

/**
 * 功能: 推进捡球动作
 * 参数: now 当前毫秒，vx/vy/wz 输出
 * 返回: None
 * 说明: 铲球、抬起，巡场时再原地转一截去找下一颗
 */
static void app_step_action(uint32_t now, int16_t *vx, int16_t *vy, int16_t *wz)
{
    *vx = 0;
    *vy = 0;
    *wz = 0;
    if (s_act == ACT_IDLE)
    {
        return;
    }
    if ((int32_t)(now - s_act_ms) < 0)
    {
        if (s_act == ACT_SPIN)
        {
            *wz = 32;
        }
        return;
    }

    if (s_act == ACT_SCOOP)
    {
        s_act = ACT_LIFT;
        s_act_ms = now + 380U;
        Servo_SetAngle(35);
        return;
    }
    if (s_act == ACT_LIFT)
    {
        Servo_SetAngle(90);
        if (s_picked < 999U)
        {
            s_picked++;
        }
        s_cool_ms = now;
        s_cx = 0;
        s_cy = 0;
        if (s_mode == MODE_SWEEP)
        {
            s_act = ACT_SPIN;
            s_act_ms = now + 650U;
            *wz = 32;
        }
        else
        {
            s_act = ACT_IDLE;
        }
        return;
    }
    s_act = ACT_IDLE;
}

/**
 * 功能: 麦轮运动学解算
 * 参数: vx 前正，vy 右移正，wz 顺时针正，fl/fr/rl/rr 四轮输出
 * 返回: None
 * 说明: X 型。任一车轮超过 100 时四轮同比缩小
 */
static void app_mecanum(int vx, int vy, int wz, int16_t *fl, int16_t *fr, int16_t *rl, int16_t *rr)
{
    int wfl = vx + vy + wz;
    int wfr = vx - vy - wz;
    int wrl = vx - vy + wz;
    int wrr = vx + vy - wz;
    int peak = app_abs(wfl);

    if (app_abs(wfr) > peak)
    {
        peak = app_abs(wfr);
    }
    if (app_abs(wrl) > peak)
    {
        peak = app_abs(wrl);
    }
    if (app_abs(wrr) > peak)
    {
        peak = app_abs(wrr);
    }
    if (peak > 100)
    {
        wfl = wfl * 100 / peak;
        wfr = wfr * 100 / peak;
        wrl = wrl * 100 / peak;
        wrr = wrr * 100 / peak;
    }
    *fl = (int16_t)wfl;
    *fr = (int16_t)wfr;
    *rl = (int16_t)wrl;
    *rr = (int16_t)wrr;
}

/**
 * 功能: 把车体速度写成四轮输出
 * 参数: vx 前正，vy 右移正，wz 顺时针正
 * 返回: None
 * 说明: 平移反向和左右轮反向在这里处理
 */
static void app_apply(int16_t vx, int16_t vy, int16_t wz)
{
    if (s_flip_y)
    {
        vy = (int16_t)(-vy);
    }
    s_vx = vx;
    s_vy = vy;
    s_wz = wz;
    app_mecanum(vx, vy, wz, &s_fl, &s_fr, &s_rl, &s_rr);
    if (s_inv_l)
    {
        s_fl = (int16_t)(-s_fl);
        s_rl = (int16_t)(-s_rl);
    }
    if (s_inv_r)
    {
        s_fr = (int16_t)(-s_fr);
        s_rr = (int16_t)(-s_rr);
    }
    Motor_Set4(s_fl, s_fr, s_rl, s_rr);
}

/**
 * 功能: 按当前模式更新电机
 * 参数: now 当前毫秒
 * 返回: None
 * 说明: 手动模式 400ms 没有新的 MOVE 就停车
 */
static void app_control(uint32_t now)
{
    int16_t vx = 0;
    int16_t vy = 0;
    int16_t wz = 0;

    if (s_act != ACT_IDLE)
    {
        app_step_action(now, &vx, &vy, &wz);
        app_apply(vx, vy, wz);
        return;
    }

    if (s_mode == MODE_STOP)
    {
        app_apply(0, 0, 0);
        return;
    }
    if (s_mode == MODE_MANUAL)
    {
        if ((now - s_move_ms) > 400U)
        {
            app_apply(0, 0, 0);
        }
        else
        {
            app_apply(s_cmd_vx, s_cmd_vy, s_cmd_wz);
        }
        return;
    }

    app_track(now, &vx, &vy, &wz);
    app_apply(vx, vy, wz);
}

/**
 * 功能: 向上位机发送状态和色块
 * 参数: None
 * 返回: None
 * 说明: STAT 后面是 BLOBS
 */
static void app_send_telem(void)
{
    char buf[160];
    int i;
    int count;
    int pos;

    snprintf(buf, sizeof(buf),
             "STAT,%u,%u,%d,%d,%d,%u,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%d\n",
             (unsigned)s_mode,
             (unsigned)s_found,
             (int)s_cx,
             (int)s_cy,
             (int)s_area,
             (unsigned)s_cm,
             (int)s_vx,
             (int)s_vy,
             (int)s_wz,
             (int)s_fl,
             (int)s_fr,
             (int)s_rl,
             (int)s_rr,
             (unsigned)Servo_GetAngle(),
             (unsigned)s_blob_n,
             (unsigned)s_picked,
             (int)s_tgt);
    app_reply(buf);

    pos = snprintf(buf, sizeof(buf), "BLOBS,%u", (unsigned)s_blob_n);
    count = (int)s_blob_n;
    for (i = 0; (i < count) && (pos > 0) && (pos < (int)sizeof(buf) - 24); i++)
    {
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos,
                        ",%d,%d,%d,%d,%d",
                        (int)s_blob[i].x, (int)s_blob[i].y,
                        (int)s_blob[i].w, (int)s_blob[i].h,
                        (int)s_blob[i].area);
    }
    if ((pos > 0) && (pos < (int)sizeof(buf) - 2))
    {
        buf[pos++] = '\n';
        buf[pos] = '\0';
        app_reply(buf);
    }
}

/**
 * 功能: 处理上位机的一行命令
 * 参数: line 不含换行
 * 返回: None
 * 说明: 认不出的命令回 ERR,bad
 */
static void app_handle_host(char *line)
{
    const char *text;
    int a;
    int b;
    int c;

    if (app_starts(line, "PING"))
    {
        app_reply("PONG\n");
        return;
    }
    if (app_starts(line, "STOP"))
    {
        s_mode = MODE_STOP;
        s_act = ACT_IDLE;
        s_cmd_vx = 0;
        s_cmd_vy = 0;
        s_cmd_wz = 0;
        s_isum = 0;
        Servo_SetAngle(90);
        app_apply(0, 0, 0);
        app_reply("ACK,STOP\n");
        return;
    }
    if (app_starts(line, "QUERY"))
    {
        app_send_telem();
        return;
    }
    if (app_starts(line, "RST"))
    {
        s_picked = 0;
        app_reply("ACK,RST\n");
        return;
    }
    if (app_starts(line, "GRAB"))
    {
        app_begin_grab(HAL_GetTick());
        app_reply("ACK,GRAB\n");
        return;
    }
    if (app_starts(line, "MODE,"))
    {
        text = line + 5;
        a = app_parse_int(&text);
        if ((a < MODE_STOP) || (a > MODE_SWEEP))
        {
            a = MODE_STOP;
        }
        s_mode = (uint8_t)a;
        s_act = ACT_IDLE;
        s_isum = 0;
        s_perr = 0;
        if (s_mode == MODE_STOP)
        {
            app_apply(0, 0, 0);
        }
        app_reply("ACK,MODE\n");
        return;
    }
    if (app_starts(line, "MOVE,"))
    {
        text = line + 5;
        s_cmd_vx = app_clamp(app_parse_int(&text));
        s_cmd_vy = app_clamp(app_parse_int(&text));
        s_cmd_wz = app_clamp(app_parse_int(&text));
        s_move_ms = HAL_GetTick();
        if (s_mode != MODE_MANUAL)
        {
            s_mode = MODE_MANUAL;
            s_act = ACT_IDLE;
        }
        return;
    }
    if (app_starts(line, "PID,"))
    {
        text = line + 4;
        a = app_parse_int(&text);
        b = app_parse_int(&text);
        c = app_parse_int(&text);
        s_kp = (int16_t)a;
        s_ki = (int16_t)b;
        s_kd = (int16_t)c;
        s_isum = 0;
        app_reply("ACK,PID\n");
        return;
    }
    if (app_starts(line, "BASE,"))
    {
        text = line + 5;
        a = app_parse_int(&text);
        if (a < 0)
        {
            a = 0;
        }
        if (a > 100)
        {
            a = 100;
        }
        s_base = (int16_t)a;
        app_reply("ACK,BASE\n");
        return;
    }
    if (app_starts(line, "SERVO,"))
    {
        text = line + 6;
        a = app_parse_int(&text);
        if (a < 0)
        {
            a = 0;
        }
        if (a > 180)
        {
            a = 180;
        }
        Servo_SetAngle((uint8_t)a);
        app_reply("ACK,SERVO\n");
        return;
    }
    if (app_starts(line, "INV,"))
    {
        text = line + 4;
        s_inv_l = (uint8_t)(app_parse_int(&text) ? 1 : 0);
        s_inv_r = (uint8_t)(app_parse_int(&text) ? 1 : 0);
        app_reply("ACK,INV\n");
        return;
    }
    if (app_starts(line, "FLIP,"))
    {
        text = line + 5;
        s_flip_y = (uint8_t)(app_parse_int(&text) ? 1 : 0);
        app_reply("ACK,FLIP\n");
        return;
    }
    if (app_starts(line, "SEEK,"))
    {
        text = line + 5;
        s_seek = (uint8_t)(app_parse_int(&text) ? 1 : 0);
        app_reply("ACK,SEEK\n");
        return;
    }
    if (app_starts(line, "THR,"))
    {
        char fwd[80];
        text = line + 4;
        a = app_parse_int(&text);
        snprintf(fwd, sizeof(fwd), "T,%d", a);
        for (b = 0; b < 5; b++)
        {
            int n = (int)strlen(fwd);
            a = app_parse_int(&text);
            snprintf(fwd + n, sizeof(fwd) - (size_t)n, ",%d", a);
        }
        {
            int n = (int)strlen(fwd);
            if (n < (int)sizeof(fwd) - 2)
            {
                fwd[n++] = '\n';
                fwd[n] = '\0';
            }
        }
        Link_OpenmvSend(fwd);
        app_reply("ACK,THR\n");
        return;
    }
    app_reply("ERR,bad\n");
}

/**
 * 功能: 把一行补到 16 个字符
 * 参数: text 缓冲区至少 17 字节
 * 返回: None
 * 说明: OLED 字宽 8，补空格才能清掉上一帧的残影
 */
static void app_pad16(char *text)
{
    int n = (int)strlen(text);

    while (n < 16)
    {
        text[n++] = ' ';
    }
    text[n] = '\0';
}

/**
 * 功能: 刷新 OLED 两行状态
 * 参数: None
 * 返回: None
 * 说明: None
 */
static void app_paint_oled(void)
{
    char line_a[20];
    char line_b[20];
    const char *name = "STP";
    int area = s_area;

    if (s_mode == MODE_MANUAL)
    {
        name = "MAN";
    }
    else if (s_mode == MODE_TRACK)
    {
        name = "TRK";
    }
    else if (s_mode == MODE_SWEEP)
    {
        name = "SWP";
    }
    if (area < 0)
    {
        area = 0;
    }
    if (area > 9999)
    {
        area = 9999;
    }
    snprintf(line_a, sizeof(line_a), "%s X%03d Y%03d", name, (int)s_cx, (int)s_cy);
    snprintf(line_b, sizeof(line_b), "D%3u A%4d P%2u",
             (unsigned)((s_cm > 999U) ? 999U : s_cm),
             area,
             (unsigned)((s_picked > 99U) ? 99U : s_picked));
    app_pad16(line_a);
    app_pad16(line_b);
    Oled_ShowString(0, 0, line_a, 16);
    Oled_ShowString(0, 16, line_b, 16);
}

/**
 * 功能: 初始化小车控制
 * 参数: None
 * 返回: None
 * 说明: 外设在 Device_Init 里按设备开关初始化。这里只写屏幕和上线应答
 */
void App_Init(void)
{
    Oled_Clear();
    Oled_ShowString(0, 0, "Tennis car", 16);
    Oled_ShowString(0, 16, "USART3 115200", 16);
    app_reply("PONG\n");
}

/**
 * 功能: 主循环里轮询通信和控制
 * 参数: None
 * 返回: None
 * 说明: 超声波 200ms，控制 20ms，回传 100ms，屏幕 500ms
 */
void App_Poll(void)
{
    char line[160];
    uint32_t now;
    static uint32_t sonar_ms;
    static uint32_t ctl_ms;
    static uint32_t tx_ms;
    static uint32_t oled_ms;

    while (Link_HostLine(line, sizeof(line)))
    {
        app_handle_host(line);
    }
    while (Link_OpenmvLine(line, sizeof(line)))
    {
        app_parse_cam(line);
    }

    now = HAL_GetTick();
    if ((now - sonar_ms) >= 200U)
    {
        s_cm = Sonar_ReadCm();
        sonar_ms = HAL_GetTick();
        now = sonar_ms;
    }
    if ((now - ctl_ms) >= 20U)
    {
        app_control(now);
        ctl_ms = now;
    }
    if ((now - tx_ms) >= 100U)
    {
        app_send_telem();
        tx_ms = now;
    }
    if ((now - oled_ms) >= 500U)
    {
        app_paint_oled();
        oled_ms = now;
    }
}

/*----------------------------------------------------------------------------*/
/*          END OF FLIE         (C) COPYRIGHT yyyyyyyxz@gmail.com             */
/*----------------------------------------------------------------------------*/

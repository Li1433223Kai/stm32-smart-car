#ifndef __BSP_OLED_H
#define __BSP_OLED_H

#include "main.h"   /* 复用CubeMX生成的HAL类型(GPIO/HAL_GPIO_*) */

/*
 * 0.96寸 4针 I2C OLED (SSD1306, 128x64, 7位地址0x3C)
 *
 * 硬件连接:
 *   SCL -> PB3
 *   SDA -> PB4
 *   VCC -> 3.3V
 *   GND -> GND
 *
 * 本模块自己用软件模拟 I2C, 不依赖 STM32 的 I2C1 外设:
 * PB3/PB4 当普通 GPIO 用, 时序在 bsp_oled.c 里自己拉。
 *
 * 显示坐标采用"行/列"方式, 每个字符为 8x16 点阵:
 *   row (0~3): 共4行, 每行高16像素 (64 / 16)
 *   col (0~15): 共16列, 每字宽8像素 (128 / 8)
 */

/* 屏幕行列数(字符单位), 方便调用者做边界检查 */
#define OLED_ROW_NUM        4
#define OLED_COL_NUM        16

/* 一行字符串缓冲长度: 16 字符 + '\0' */
#define OLED_LINE_MAX       (OLED_COL_NUM + 1)

/* 7位地址 0x3C 左移1位、写方向置0, 直接发到总线上 = 0x78 */
#define OLED_ADDR           (0x3C << 1)

/* 从机不应答的累计次数. 屏没接/没上电时它只涨 */
extern uint32_t g_i2c_nack_count;

/* 函数声明 --------------------------------------------------------*/

/* 初始化 OLED(发初始化命令序列), 上电后调用一次 */
void OLED_Init(void);

/* 清空整屏 */
void OLED_Clear(void);

/*
 * 在(row, col)处显示单个ASCII字符.
 * row: 0~3, col: 0~15; 超出屏幕用空白/忽略处理
 */
void OLED_ShowChar(uint8_t row, uint8_t col, char ch);

/* 在(row, col)处显示字符串, 自动跨列, 遇'\0'结束 */
void OLED_ShowString(uint8_t row, uint8_t col, const char *str);

/*
 * 显示一整行: 16个8x16字符正好占满128列, 不足部分用空格补齐。
 * 会覆盖整行旧内容, 所以连续刷新时不需要先 OLED_Clear()。
 */
void OLED_ShowLine(uint8_t row, const char *str);

/* 在(row, col)处显示有符号整数(带正负号), 自动把数字转成字符 */
void OLED_ShowNum(uint8_t row, uint8_t col, int32_t num);

#endif

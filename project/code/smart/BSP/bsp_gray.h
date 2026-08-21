#ifndef __BSP_GRAY_H
#define __BSP_GRAY_H

#include "main.h"

/* 八路灰度传感器通道数 */
#define GRAY_CHANNEL_NUM    8

/*
 * 传感器输出电平定义
 * 注意: 不同模块可能高低电平相反!
 * 如果实测发现黑白反过来了, 把下面两个值互换即可
 *   OUT=低电平(0) → 检测到黑线(低反射率)
 *   OUT=高电平(1) → 检测到白线(高反射率)
 */
#define GRAY_BLACK          0
#define GRAY_WHITE          1

/* 丢线/全黑标志值 */
#define GRAY_ALL_WHITE      9999    /* 8路全白(丢线) */
#define GRAY_ALL_BLACK      -9999   /* 8路全黑(十字/起点) */

/* 函数声明 --------------------------------------------------------*/

void Gray_Init(void);                                    /* 初始化(GPIO已在MX_GPIO_Init中配置) */
uint8_t Gray_ReadChannel(uint8_t ch);                    /* 读取单个通道: 返回GRAY_BLACK或GRAY_WHITE */
void Gray_ReadAll(uint8_t values[GRAY_CHANNEL_NUM]);     /* 读取全部8路, 结果存入values数组 */
int16_t Gray_GetPosition(void);                          /* 计算黑线位置: -7000~+7000, 0居中, 负偏左, 正偏右 */

#endif

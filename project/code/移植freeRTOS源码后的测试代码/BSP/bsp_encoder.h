#ifndef __BSP_ENCODER_H
#define __BSP_ENCODER_H

#include "main.h"

#define ENCODER_PULSE_PER_REV   (20 * 13 * 4)   /* 1040 */


/* 编码器初始化：启动 TIM2(左) 和 TIM4(右) 编码器计数 */
void Encoder_Init(void);

/* 读左/右编码器上次到这次的脉冲数(带符号)，读后清零 */
int16_t Encoder_GetCount_Left(void);
int16_t Encoder_GetCount_Right(void);

/* 返回速度 */
int16_t Encoder_GetSpeed_Left(void);
int16_t Encoder_GetSpeed_Right(void);

//计算速度
void Encoder_UpdateSpeed(void);
#endif
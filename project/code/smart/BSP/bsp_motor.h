#ifndef __BSP_MOTOR_H
#define __BSP_MOTOR_H

#include "main.h"   /* 包含CubeMX生成的引脚宏定义(BIN1_Pin/htim3等) */

/*
 * PWM占空比最大值, 对应ARR=7199
 * PWM频率 = 72MHz / (PSC+1) / (ARR+1) = 72M / 1 / 7200 = 10kHz
 * 范围0~7199共7200级调速, 官方例程使用同一参数
 */
#define MOTOR_PWM_MAX        7200

/* 电机编号枚举: 对应D153C的A/B两个通道
 * A通道(左轮): PWMA=PB1(TIM3_CH4), AIN1=PB10, AIN2=PB11
 * B通道(右轮): PWMB=PB0(TIM3_CH3), BIN1=PB12, BIN2=PB13
 */
typedef enum {
    MOTOR_A = 1,    /* A通道(左轮) */
    MOTOR_B         /* B通道(右轮) */
} MotorID_t;

/* 函数声明 --------------------------------------------------------*/

void Motor_Init(void);                       /* 初始化: 启动PWM输出(STBY已用跳线帽接3.3V) */
void Motor_SetSpeed(MotorID_t motor, int16_t speed);  /* 设置电机速度: speed范围-7200~+7200 */
void Motor_Stop(MotorID_t motor);            /* 滑行停止: 电机断电, 靠摩擦力慢慢停 */
void Motor_Brake(MotorID_t motor);           /* 刹车: 电机两端短接, 快速停转 */
void Motor_StopAll(void);                    /* 两个电机同时滑行停止 */

#endif

#include "bsp_motor.h"

/* htim3在tim.c中定义, 这里用extern声明才能访问它 */
extern TIM_HandleTypeDef htim3;

/*
 * 电机初始化
 * CubeMX只配置了定时器, 但不会自动启动PWM输出, 必须手动调用HAL_TIM_PWM_Start
 *
 * D153C引脚接线说明(沿用PB10~PB13):
 *   左轮(A通道): PWMA->PB1(TIM3_CH4), AIN1->PB10, AIN2->PB11
 *   右轮(B通道): PWMB->PB0(TIM3_CH3), BIN1->PB12, BIN2->PB13
 *
 * 注意: STBY引脚已用跳线帽接到3.3V, 不需要软件控制
 */
void Motor_Init(void)
{
    /* 启动TIM3 CH3和CH4的PWM输出 */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);   /* PB0 - PWMB(右电机) */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);   /* PB1 - PWMA(左电机) */

    /* 初始占空比为0, 电机不转 */
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
}

/*
 * 设置电机速度和方向
 * speed > 0: 正转, speed < 0: 反转, speed = 0: 停止
 * speed的绝对值是PWM占空比(0~7200), 绝对值越大转速越快
 *
 * TB6612方向控制真值表:
 *   xIN1=1, xIN2=0, PWM=H  → 正转
 *   xIN1=0, xIN2=1, PWM=H  → 反转
 *   xIN1=0, xIN2=0, PWM=x  → 滑行停止(高阻态)
 *   xIN1=1, xIN2=1, PWM=H  → 刹车(两端短接)
 *   xIN1=1, xIN2=1, PWM=L  → 高阻态(非刹车!)
 */
void Motor_SetSpeed(MotorID_t motor, int16_t speed)
{
    uint16_t pwm;   /* 实际写入CCR寄存器的占空比值, 必须是非负数 */

    /* 速度限幅: 防止超出-7200~+7200范围 */
    if (speed > MOTOR_PWM_MAX) speed = MOTOR_PWM_MAX;
    if (speed < -MOTOR_PWM_MAX) speed = -MOTOR_PWM_MAX;

    if (speed >= 0)
        {
            /* 正转: 方向引脚设为 1,0, PWM取speed本身 */
            pwm = (uint16_t)speed;
            if (motor == MOTOR_A)
                {
                    HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_SET);    /* AIN1=1 */
                    HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);  /* AIN2=0 */
                }
            else
                {
                    HAL_GPIO_WritePin(CIN1_GPIO_Port, CIN1_Pin, GPIO_PIN_SET);    /* BIN1=1 */
                    HAL_GPIO_WritePin(CIN2_GPIO_Port, CIN2_Pin, GPIO_PIN_RESET);  /* BIN2=0 */
                }
        }
    else
        {
            /* 反转: 方向引脚设为 0,1, PWM取speed的绝对值 */
            pwm = (uint16_t)(-speed);
            if (motor == MOTOR_A)
                {
                    HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);  /* AIN1=0 */
                    HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_SET);    /* AIN2=1 */
                }
            else
                {
                    HAL_GPIO_WritePin(CIN1_GPIO_Port, CIN1_Pin, GPIO_PIN_RESET);  /* BIN1=0 */
                    HAL_GPIO_WritePin(CIN2_GPIO_Port, CIN2_Pin, GPIO_PIN_SET);    /* BIN2=1 */
                }
        }

    /* 写入CCR寄存器, 设置PWM占空比
     * CCR值越大, 高电平时间越长, 电机转速越快
     * CCR=0时电机不转, CCR=7200时全速 */
    if (motor == MOTOR_A)
        {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pwm);  /* PB1 - PWMA */
        }
    else
        {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm);  /* PB0 - PWMB */
        }
}

/*
 * 滑行停止
 * 两个方向引脚都为0, TB6612输出高阻态, 电机断电自由滑行
 * 车轮会靠惯性转一会儿才停
 */
void Motor_Stop(MotorID_t motor)
{
    if (motor == MOTOR_A)
        {
            HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
        }
    else
        {
            HAL_GPIO_WritePin(CIN1_GPIO_Port, CIN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(CIN2_GPIO_Port, CIN2_Pin, GPIO_PIN_RESET);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
        }
}

/*
 * 刹车
 * 两个方向引脚都为1且PWM为高电平, 电机两端被短接, 产生反向电动势(制动力)
 * 比滑行停止更快停下, 类似汽车的刹车
 * 注意: PWM必须设为满占空比(高电平), 若PWM=0则IN1=1,IN2=1是高阻态而非刹车
 */
void Motor_Brake(MotorID_t motor)
{
    if (motor == MOTOR_A)
        {
            HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_SET);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, MOTOR_PWM_MAX);  /* PWM必须为高电平才是真刹车 */
        }
    else
        {
            HAL_GPIO_WritePin(CIN1_GPIO_Port, CIN1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(CIN2_GPIO_Port, CIN2_Pin, GPIO_PIN_SET);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, MOTOR_PWM_MAX);  /* PWM必须为高电平才是真刹车 */
        }
}

/* 两个电机同时滑行停止, 紧急停车时调用 */
void Motor_StopAll(void)
{
    Motor_Stop(MOTOR_A);
    Motor_Stop(MOTOR_B);
}

#include "bsp_motor.h"

/* htim3在tim.c中定义, 这里用extern声明才能访问它 */
extern TIM_HandleTypeDef htim3;

/*
 * 电机初始化
 * 安全启动顺序: 先设方向引脚为滑行(0,0) → CCR清0 → 再启动PWM
 * 这样PWM启动瞬间输出0占空比+方向引脚为安全态, 不会有毛刺
 *
 * D153C双路驱动板引脚接线:
 *   A通道(左轮): PWMA->PB1(TIM3_CH4), AIN1->PB10, AIN2->PB11
 *   B通道(右轮): PWMB->PB0(TIM3_CH3), BIN1->PB12, BIN2->PB13
 *
 * 注意: STBY引脚用跳线帽接到3.3V, 不需要软件控制
 */
void Motor_Init(void)
{
    /* 第1步: 方向引脚先设为滑行停止(0,0), 确保PWM启动前TB6612处于安全态 */
    HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);

    /* 第2步: CCR先清0, 确保PWM启动瞬间占空比为0 */
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);

    /* 第3步: 启动PWM输出, 此时CCR=0, 电机不会转 */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);   /* PB0 - PWMB(右电机) */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);   /* PB1 - PWMA(左电机) */
}

/*
 * 设置电机速度和方向
 * speed > 0: 正转, speed < 0: 反转, speed = 0: 停止
 * speed的绝对值是PWM占空比(0~7200), 绝对值越大转速越快
 *
 * 安全切换顺序: 先关PWM(CCR=0) → 再改方向引脚 → 最后设新PWM
 * 这样方向切换过程中不会出现IN1/IN2中间态+PWM输出的错误组合
 *
 * TB6612方向控制真值表:
 *   xIN1=1, xIN2=0, PWM=H  → 正转
 *   xIN1=0, xIN2=1, PWM=H  → 反转
 *   xIN1=0, xIN2=0, PWM=x  → 滑行停止(高阻态)
 *   xIN1=1, xIN2=1, PWM=x  → 刹车(两端短接)
 *   
 */
void Motor_SetSpeed(MotorID_t motor, int16_t speed)
{
    uint16_t pwm;   /* 实际写入CCR寄存器的占空比值, 必须是非负数 */
    uint32_t pwm_channel;
    GPIO_TypeDef *in1_port, *in2_port;
    uint16_t in1_pin, in2_pin;

    /* 速度限幅: 防止超出-7200~+7200范围 */
    if (speed > MOTOR_PWM_MAX) speed = MOTOR_PWM_MAX;
    if (speed < -MOTOR_PWM_MAX) speed = -MOTOR_PWM_MAX;

    /* 根据电机ID选择对应的PWM通道和方向引脚 */
    if (motor == MOTOR_A)
        {
            pwm_channel = TIM_CHANNEL_4;    /* PB1 - PWMA */
            in1_port = AIN1_GPIO_Port;
            in1_pin  = AIN1_Pin;
            in2_port = AIN2_GPIO_Port;
            in2_pin  = AIN2_Pin;
        }
    else
        {
            pwm_channel = TIM_CHANNEL_3;    /* PB0 - PWMB */
            in1_port = BIN1_GPIO_Port;
            in1_pin  = BIN1_Pin;
            in2_port = BIN2_GPIO_Port;
            in2_pin  = BIN2_Pin;
        }

    /* 第1步: 先关PWM, 确保切换方向时PWM输出为0 */
    __HAL_TIM_SET_COMPARE(&htim3, pwm_channel, 0);

    /* 第2步: 设置方向引脚 */
    if (speed > 0)
        {
            /* 正转: IN1=1, IN2=0 */
            HAL_GPIO_WritePin(in1_port, in1_pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(in2_port, in2_pin, GPIO_PIN_RESET);
            pwm = (uint16_t)speed;
        }
    else if (speed < 0)
        {
            /* 反转: IN1=0, IN2=1 */
            HAL_GPIO_WritePin(in1_port, in1_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(in2_port, in2_pin, GPIO_PIN_SET);
            pwm = (uint16_t)(-speed);
        }
    else
        {
            /* speed=0: 滑行停止 IN1=0, IN2=0, PWM=0 */
            HAL_GPIO_WritePin(in1_port, in1_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(in2_port, in2_pin, GPIO_PIN_RESET);
            pwm = 0;
        }

    /* 第3步: 方向引脚已稳定, 设置PWM占空比 */
    __HAL_TIM_SET_COMPARE(&htim3, pwm_channel, pwm);
}

/*
 * 滑行停止
 * 两个方向引脚都为0, TB6612输出高阻态, 电机断电自由滑行
 * 车轮会靠惯性转一会儿才停
 */
void Motor_Stop(MotorID_t motor)
{
    /* 先关PWM, 再设方向引脚为滑行态 */
    if (motor == MOTOR_A)
        {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
            HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
        }
    else
        {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
            HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);
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
    /* 刹车顺序: 先设IN1=IN2=1, 再设PWM为满占空比 */
    if (motor == MOTOR_A)
        {
            HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_SET);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, MOTOR_PWM_MAX);
        }
    else
        {
            HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_SET);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, MOTOR_PWM_MAX);
        }
}

/* 两个电机同时滑行停止, 紧急停车时调用 */
void Motor_StopAll(void)
{
    Motor_Stop(MOTOR_A);
    Motor_Stop(MOTOR_B);
}

/* 两个电机同时刹车, 快速停转 */
void Motor_BrakeAll(void)
{
    Motor_Brake(MOTOR_A);
    Motor_Brake(MOTOR_B);
}

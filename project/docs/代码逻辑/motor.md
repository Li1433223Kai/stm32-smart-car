void Motor_Init(void)  #初始化电机

1.方向引脚置零
例如：HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
2.CCR清零
__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
3.pwm_start，启动pwm
HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

void Motor_SetSpeed(MotorID_t motor, int16_t speed)
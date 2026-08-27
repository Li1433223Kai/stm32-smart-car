#include "app.h"
#include "bsp_motor.h"
#include "bsp_gray.h"
#include "bsp_encoder.h"
#include <stdio.h>
//App__Position_Control位置环参数

#define KP              1
#define TARGET_MIN      100
#define TARGET_MAX      200
#define BASE_SPEED      150
#define MAX_STEER       50
/* ==== 速度环PID参数 ==== */

#define PID_KP          15     /* 比例, 先小逐步调大 */
#define PID_KI          1     /* 先用0, P调稳后再加 */
#define INTEGRAL_MAX    2000    /* 积分限幅 */
#define PWM_LIMIT       7200    /* PWM限幅 */

static int32_t s_left_integral  = 0;
static int32_t s_right_integral = 0;
static int16_t target_left = 150 ;
static int16_t target_right = 150;
/*
 * 循迹模块初始化
*/
void App_Init(void)
{
    /* 暂无需要初始化的内容 */
}

//执行位置环pid
void App_Position_Control(void)
{
    int16_t pos;
    pos = Gray_GetPosition();   //误差
    float steer = (float)KP * ((float)pos / 7000.0f) * MAX_STEER; //归一化pos

	
	if(pos == GRAY_ALL_BLACK )  //全黑
		{
		    target_left = 0;			
			target_right = 0;
			s_left_integral  = 0;    // 清零积分
			s_right_integral = 0;
			return;
		}
		
	else if(pos == GRAY_ALL_WHITE)  //全白
		{
		    target_left = 0;			
			target_right = 0;
			s_left_integral  = 0;    // 清零积分
			s_right_integral = 0;
			return;
		}
		
	else
		{
			target_left  = (int16_t)(BASE_SPEED + steer);
			target_right = (int16_t)(BASE_SPEED - steer);
		}
		
//		rpm限幅
		if (target_left  < TARGET_MIN) target_left  = TARGET_MIN;
		if (target_left  > TARGET_MAX) target_left  = TARGET_MAX;
		if (target_right < TARGET_MIN) target_right = TARGET_MIN;
		if (target_right > TARGET_MAX) target_right = TARGET_MAX;
		
		printf("%d,%d,%d,%d,%d\n", pos, target_left, target_right,
       (int)Encoder_GetSpeed_Left(), (int)Encoder_GetSpeed_Right());
}
 
   

//==== 速度环PI: 每10ms调用一次, 让左右轮各自达到TARGET_SPEED ====
void APP_Speed_Control(void)
	{
		int32_t err_left = target_left - Encoder_GetSpeed_Left();
		int32_t err_right = target_right - Encoder_GetSpeed_Right();
		int32_t pwm_left,pwm_right;
		s_left_integral +=err_left;
		s_right_integral += err_right;
		
		
		//积分限幅
		if(s_left_integral  > INTEGRAL_MAX)  s_left_integral = INTEGRAL_MAX;
		if(s_left_integral  < -INTEGRAL_MAX)  s_left_integral = -INTEGRAL_MAX;
		if(s_right_integral  > INTEGRAL_MAX)  s_right_integral = INTEGRAL_MAX;
		if(s_right_integral  < -INTEGRAL_MAX)  s_right_integral = -INTEGRAL_MAX;
		
		//pid计算
		pwm_left = PID_KP * err_left + PID_KI * s_left_integral;
		pwm_right = PID_KP * err_right + PID_KI * s_right_integral;
		
		//pwm_限幅
	    if (pwm_left  > PWM_LIMIT)  pwm_left  = PWM_LIMIT;
		if (pwm_left  < -PWM_LIMIT) pwm_left  = -PWM_LIMIT;
		if (pwm_right > PWM_LIMIT)  pwm_right = PWM_LIMIT;
		if (pwm_right < -PWM_LIMIT) pwm_right = -PWM_LIMIT;
		
		//调速
		Motor_SetSpeed(MOTOR_A,pwm_left);
		Motor_SetSpeed(MOTOR_B,pwm_right);
		
		
	}
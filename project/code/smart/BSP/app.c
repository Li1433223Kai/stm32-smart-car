#include "app.h"
#include "bsp_motor.h"
#include "bsp_gray.h"
#include "bsp_encoder.h"
/*
 * 循迹控制参数定义
 * 本阶段先"纯直行": 不读位置做差速, 只用固定 base 速度让两轮同速,
 * 目的是先验证两电机都能正常正转、base 速度选得合适(高于死区)。
 * Kp/差速逻辑下一阶段再加。
 */
 //app_follow位置环参数
#define BASE_SPEED      1500   /* 基础速度 */
#define KP              1
#define MIN_SPEED       600
#define MAX_SPEED       7200

/* ==== 速度环PID参数 ==== */
#define TARGET_SPEED    150     /* 目标车轮RPM */
#define PID_KP          50     /* 比例, 先小逐步调大 */
#define PID_KI          1      /* 先用0, P调稳后再加 */
#define INTEGRAL_MAX    2000    /* 积分限幅 */
#define PWM_LIMIT       7200    /* PWM限幅 */

static int32_t s_left_integral  = 0;
static int32_t s_right_integral = 0;
/*
 * 循迹模块初始化
*/
void App_Init(void)
{
    /* 暂无需要初始化的内容 */
}

//执行位置环循迹
void App_Follow(void)
{
    int16_t pos;
    int32_t steer;
	int16_t right ,left;
    pos = Gray_GetPosition();
    steer = (int32_t)KP * pos ;
	
	if(pos == GRAY_ALL_BLACK )  //全黑
		{
		   Motor_SetSpeed(MOTOR_A, BASE_SPEED);    
		   Motor_SetSpeed(MOTOR_B, BASE_SPEED);   
			return;
		}
		
	else if(pos == GRAY_ALL_WHITE)  //全白
		{
		   Motor_SetSpeed(MOTOR_A, 0);    
		   Motor_SetSpeed(MOTOR_B, 0);
			return;
		}
		
	else
		{
			left = BASE_SPEED + steer;
			right = BASE_SPEED - steer;
		
//		pwm限幅
		if (left  < MIN_SPEED)  left  = MIN_SPEED;
		if (left  > MAX_SPEED)  left  = MAX_SPEED;
		if (right < MIN_SPEED)  right = MIN_SPEED;
		if (right > MAX_SPEED)  right = MAX_SPEED;
		}

 
    Motor_SetSpeed(MOTOR_A, left);    /* A = 左轮 */
    Motor_SetSpeed(MOTOR_B, right);    /* B = 右轮 */
}
//==== 速度环PI: 每10ms调用一次, 让左右轮各自达到TARGET_SPEED ====
void Speed_Control(void)
	{
		int32_t err_left = TARGET_SPEED - Encoder_GetSpeed_Left();
		int32_t err_right = TARGET_SPEED - Encoder_GetSpeed_Right();
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
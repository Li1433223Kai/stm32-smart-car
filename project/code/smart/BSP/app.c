#include "app.h"
#include "bsp_motor.h"
#include "bsp_gray.h"
#include "bsp_encoder.h"
#include "bsp_oled.h"
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

/* 最近一次显示刷新记录, 供 OLED 数据面板显示 */
static int16_t s_display_pos      = 0;    /* 黑线位置 */
static int16_t s_display_speed_L  = 0;    /* 左轮实测速度 */
static int16_t s_display_speed_R  = 0;    /* 右轮实测速度 */
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
			/* 记录显示数据 */
			s_display_pos     = pos;
			s_display_speed_L = Encoder_GetSpeed_Left();
			s_display_speed_R = Encoder_GetSpeed_Right();
			return;
		}
		
	else if(pos == GRAY_ALL_WHITE)  //全白
		{
		    target_left = 0;			
			target_right = 0;
			s_left_integral  = 0;    // 清零积分
			s_right_integral = 0;
			/* 记录显示数据 */
			s_display_pos     = pos;
			s_display_speed_L = Encoder_GetSpeed_Left();
			s_display_speed_R = Encoder_GetSpeed_Right();
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

		/* 记录本次数据, 供 OLED 面板显示 */
		s_display_pos     = pos;
		s_display_speed_L = Encoder_GetSpeed_Left();
		s_display_speed_R = Encoder_GetSpeed_Right();
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

/*
 * 刷新 OLED 数据面板(由 main 主循环按节拍调用, 如100ms一次)
 *
 * 面板布局 (4行, 每行16字; 列从左到右0~15):
 *   行0: ST:xxxx   (状态: RUN/LOST/CROSS/STOP)
 *   行1: 目标速度   "T 150/150"  (左/右目标, 3位)
 *   行2: 实测速度   "L 120 R 118" (左右轮实测RPM)
 *   行3: 位置偏差   "Pos 1250"    (黑线位置, 连符号最多5位)
 */
void App_OLED_Show(void)
{
	/* 状态字符串: 依据当前位置环判定 */
	const char *state_str;
	if      (s_display_pos == GRAY_ALL_WHITE) state_str = "LOST";
	else if (s_display_pos == GRAY_ALL_BLACK)  state_str = "CROSS";
	else if (target_left == 0 && target_right == 0) state_str = "STOP";
	else                                        state_str = "RUN";

	OLED_Clear();

	/* 行0: 状态 */
	OLED_ShowString(0, 0, "ST:");
	OLED_ShowString(0, 3, state_str);

	/* 行1: 目标速度 "T 150/150" */
	OLED_ShowString(1, 0,  "T ");
	OLED_ShowNum(1, 2,  target_left);
	OLED_ShowString(1, 6,  "/");
	OLED_ShowNum(1, 7,  target_right);

	/* 行2: 实测速度 "L 120 R 118" */
	OLED_ShowString(2, 0,  "L ");
	OLED_ShowNum(2, 2,  s_display_speed_L);
	OLED_ShowString(2, 6,  "R ");
	OLED_ShowNum(2, 8,  s_display_speed_R);

	/* 行3: 位置偏差 */
	OLED_ShowString(3, 0, "Pos ");
	OLED_ShowNum(3, 4, s_display_pos);
}
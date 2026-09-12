#include "app.h"
#include "bsp_motor.h"
#include "bsp_gray.h"
#include "bsp_encoder.h"
#include "bsp_key.h"
#include "bsp_oled.h"
#include <stdio.h>
//App__Position_Control位置环参数

#define KP              1.5
#define TARGET_MIN      40
#define TARGET_MAX      200
#define BASE_SPEED      120
#define MAX_STEER       80
/* ==== 速度环PID参数 ==== */

#define PID_KP          15     /* 比例, 先小逐步调大 */
#define PID_KI          1     /* 先用0, P调稳后再加 */
#define INTEGRAL_MAX    2000    /* 积分限幅 */
#define PWM_LIMIT       7200    /* PWM限幅 */

static int32_t s_left_integral  = 0;
static int32_t s_right_integral = 0;
static int16_t target_left = 100 ;
static int16_t target_right = 100;

//oled显示
static int16_t s_display_pos      = 0;    /* 黑线位置 */
static int16_t s_display_speed_L  = 0;    /* 左轮实测速度 */
static int16_t s_display_speed_R  = 0;    /* 右轮实测速度 */

static AppState_t s_state = APP_STATE_IDLE;
static uint8_t    s_brake = 0;     /* 1=刹车保持: 速度环不输出, 维持短接刹车 */
static uint32_t s_boot_time = 0;   /* 上电时刻 */
static uint32_t s_print_time = 0;
/*
 * 循迹模块初始化
*/

	void App_Init(void)
{
    s_state = APP_STATE_IDLE;
    s_brake = 0;
    s_boot_time = HAL_GetTick();   /* 记录上电时刻 */
}


//执行位置环pid (pos 由调用者传入: 本拍已读到的灰度位置, 避免重复读灰度)
void App_Position_Control(int16_t pos)
{
    float steer = (float)KP * ((float)pos / 7000.0f) * MAX_STEER; //归一化pos

	target_left  = (int16_t)(BASE_SPEED + steer);
	target_right = (int16_t)(BASE_SPEED - steer);

//	rpm限幅
	if (target_left  < TARGET_MIN) target_left  = TARGET_MIN;
	if (target_left  > TARGET_MAX) target_left  = TARGET_MAX;
	if (target_right < TARGET_MIN) target_right = TARGET_MIN;
	if (target_right > TARGET_MAX) target_right = TARGET_MAX;
}
 
   

//==== 速度环PI: 每10ms调用一次, 让左右轮各自达到TARGET_SPEED ====
void APP_Speed_Control(void)
	{
		int32_t err_left;
		int32_t err_right;
		int32_t pwm_left,pwm_right;

		/* 刹车保持期间不输出速度环:
		 * 否则本函数里的 Motor_SetSpeed 会重写方向引脚与PWM,
		 * 把 Motor_BrakeAll() 设的短接刹车覆盖掉(只剩几十微秒) */
		if (s_brake)
			{
				Motor_BrakeAll();    /* 幂等: 持续维持刹车态 */
				return;
			}

		err_left = target_left - Encoder_GetSpeed_Left();
		err_right = target_right - Encoder_GetSpeed_Right();
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
	switch (s_state) {
    case APP_STATE_IDLE:  state_str = "IDLE";  break;
    case APP_STATE_RUN:   state_str = "RUN";   break;
    case APP_STATE_LOST:  state_str = "LOST";  break;
    case APP_STATE_CROSS: state_str = "CROSS"; break;
    default:              state_str = "IDLE";  break;
}                                      
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

static void App_Stop(void) 
	{ 
		target_left=0;
		target_right=0;
		s_left_integral=0;
		s_right_integral=0;
	}

void App_State_Update(void)
{
    /* 读按键边沿: Key_Scan() 返回1=刚按下(从松到按) */
    uint8_t key_edge = Key_Scan();
    AppState_t prev_state = s_state;   /* 本拍入口状态, 用于检测状态转移 */

    switch (s_state)
		{
		case APP_STATE_IDLE:
		App_Stop();
		if ((HAL_GetTick() - s_boot_time) > 200 && key_edge)   /* 相减>200ms */
			s_state = APP_STATE_RUN;
		break;

		case APP_STATE_RUN:
			{
				int16_t pos = Gray_GetPosition();
				s_display_pos = pos;            /* 先记录(含 9999/-9999 哨兵), 供打印/OLED */
				if (pos == GRAY_ALL_WHITE)      
					s_state = APP_STATE_LOST;   /* 丢线 */
				else if (pos == GRAY_ALL_BLACK) 
					s_state = APP_STATE_CROSS;  /* 全黑 */
				else
					App_Position_Control(pos);        /* 正常循迹差速(复用本拍已读的 pos) */
			}
			break;

		/* LOST(丢线) 与 CROSS(十字): 刹车已在转移当拍执行并保持, 这里只处理恢复 */
		case APP_STATE_LOST:
		case APP_STATE_CROSS:
			if (key_edge)
				s_state = APP_STATE_RUN;      /* 摆回线上/离开十字后重新起步 */
			break;
		}

		/* ==== 状态转移动作: 统一在转移的当拍执行, 必然早于本拍的速度环 ==== */
		if (s_state != prev_state)
			{
				if (s_state == APP_STATE_LOST || s_state == APP_STATE_CROSS)
					{
						App_Stop();          /* 先清目标+积分, 消除残留目标驱动 */
						Motor_BrakeAll();    /* 立即短接刹车 */
						s_brake = 1;         /* 保持刹车: 速度环不再输出 */
					}
				else
					{
						s_brake = 0;         /* 离开停车态(如回 RUN): 解除刹车保持 */
					}
			}
		/* 显示快照: 每拍更新(含停车状态), 否则 OLED/打印会停留在最后一次 RUN 的旧值 */
		s_display_speed_L = Encoder_GetSpeed_Left();
		s_display_speed_R = Encoder_GetSpeed_Right();

		/* 每500ms打印一次, 避免刷屏/拖慢控制 */
	if (HAL_GetTick() - s_print_time >= 500)
		{
		s_print_time = HAL_GetTick();
		printf("ST:%d pos:%d L_t:%d R_t:%d L_s:%d R_s:%d\n",
		s_state, s_display_pos, target_left, target_right,
		(int)Encoder_GetSpeed_Left(), (int)Encoder_GetSpeed_Right());
		}
}

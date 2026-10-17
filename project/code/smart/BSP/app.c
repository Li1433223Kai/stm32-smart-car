#include "app.h"
#include "bsp_motor.h"
#include "bsp_gray.h"
#include "bsp_encoder.h"
#include "bsp_key.h"
#include "bsp_oled.h"
#include "bsp_uart.h"
#include "FreeRTOS.h"
#include "task.h"
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

static char s_frame[UART_FRAME_MAX];


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
 * 刷新 OLED 数据面板
 *
 * 面板布局 (4行, 每行16字; 列从左到右0~15):
 *   行0: ST:xxxx   (状态: RUN/LOST/CROSS/STOP)
 *   行1: 目标速度   "T 150/150"  (左/右目标, 3位)
 *   行2: 实测速度   "L 120 R 118" (左右轮实测RPM)
 *   行3: 位置偏差   "Pos 1250"    (黑线位置, 连符号最多5位)
 *
 * 由 displayTask 每 200ms 调用一次。每行走 OLED_ShowLine 整行覆盖,
 * 不再每帧全屏 Clear —— 整帧实测 133ms (含被控制任务抢占的时间)。
 * 原来逐字符写、每帧先全屏 Clear 的写法按同样速率约 310ms,
 * 直接放进 10ms 控制节拍会把周期拖慢十几倍。
 */
void App_OLED_Show(void)
{
    char line[OLED_LINE_MAX];
    const char *state_str;

    /* 状态字符串: 依据当前位置环判定 */
    switch (s_state)
        {
        case APP_STATE_IDLE:  state_str = "IDLE";  break;
        case APP_STATE_RUN:   state_str = "RUN";   break;
        case APP_STATE_LOST:  state_str = "LOST";  break;
        case APP_STATE_CROSS: state_str = "CROSS"; break;
        default:              state_str = "IDLE";  break;
        }

    /* 行0: 状态 "ST:RUN" */
    snprintf(line, sizeof(line), "ST:%s", state_str);
    OLED_ShowLine(0, line);

    /* 行1: 目标速度 "T 150/150" */
    snprintf(line, sizeof(line), "T %d/%d", target_left, target_right);
    OLED_ShowLine(1, line);

    /* 行2: 实测速度 "L 120 R 118" */
    snprintf(line, sizeof(line), "L %d R %d", s_display_speed_L, s_display_speed_R);
    OLED_ShowLine(2, line);

    /* 行3: 位置偏差 "Pos 1250" */
    snprintf(line, sizeof(line), "Pos %d", s_display_pos);
    OLED_ShowLine(3, line);
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
        snprintf(s_frame, UART_FRAME_MAX,
                 "ST:%d pos:%d L_t:%d R_t:%d L_s:%d R_s:%d\n",
                 s_state, s_display_pos, target_left, target_right,
                 (int)Encoder_GetSpeed_Left(), (int)Encoder_GetSpeed_Right());
        Uart_Send(s_frame);          /* 只入队, 不等发送 */
    }
}

/*
 * 显示任务: 每 200ms 刷一帧数据面板
 *
 * 优先级 1 排在控制任务(2)之后。控制任务每 10ms 里只忙约 1ms, 剩下 9ms
 * 本来就在 IDLE 空转, 刷屏就填在这段时间里; 控制任务随时抢占它, 10ms 节拍
 * 不受影响。SSD1306 不要求主机连续供时钟, 整帧被切碎只会变慢, 不会花屏。
 *
 * 实测: 单帧 133ms, 在 200ms 周期里占 67%; 同期控制任务仍是 100 拍 = 1000 tick,
 * 一拍不差。周期不能往下调 —— 一帧就要 133ms, 改成 100ms 会让 vTaskDelayUntil
 * 连着立即返回去追基准, 帧率反而掉下来。
 *
 * 屏只有这一个任务碰, 不需要互斥量。
 */
void displayTask(void *pvParameters)
{
    TickType_t last_wake;

    (void) pvParameters;

    OLED_Init();                             /* 内部已经清屏, 只跑一次 */

    /* 节拍基准放在 OLED 之后取: 若在它之前取, 任务醒来时已经过去十几拍,
     * vTaskDelayUntil 会连续立即返回去追基准, 白转十几圈才追平 */
    last_wake = xTaskGetTickCount();

    for (; ;)
    {
        App_OLED_Show();

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(200));
    }
}

void controlTask(void *pvParameters)
{
    TickType_t last_wake;

    (void) pvParameters;

    last_wake = xTaskGetTickCount();

    for (; ;)
    {
        Encoder_UpdateSpeed();
        App_State_Update();
        APP_Speed_Control();

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
    }
}



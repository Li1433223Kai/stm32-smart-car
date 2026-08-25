#include "app.h"
#include "bsp_motor.h"
#include "bsp_gray.h"

/*
 * 循迹控制参数定义
 * 本阶段先"纯直行": 不读位置做差速, 只用固定 base 速度让两轮同速,
 * 目的是先验证两电机都能正常正转、base 速度选得合适(高于死区)。
 * Kp/差速逻辑下一阶段再加。
 */
#define BASE_SPEED      1500   /* 基础速度 */
#define KP              1
#define MIN_SPEED       600
#define MAX_SPEED       7200
/*
 * 循迹模块初始化
*/
void App_Init(void)
{
    /* 暂无需要初始化的内容 */
}

//执行循迹
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

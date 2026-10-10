#include "bsp_encoder.h"
#include "tim.h"   /* 拿 htim2/htim4 */


extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;

static int16_t s_speed_left  = 0;
static int16_t s_speed_right = 0;
static uint32_t s_last_tick = 0;
void Encoder_Init(void)
	{
		/* 启动两个编码器定时器的计数 */
		HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
		HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);

		/* 计数清零 */
		__HAL_TIM_SET_COUNTER(&htim2, 0);
		__HAL_TIM_SET_COUNTER(&htim4, 0);
		
		s_last_tick = HAL_GetTick();   //记录当前时间
	}

	//读左轮计数值，然后清零
int16_t Encoder_GetCount_Left(void)
	{
		int16_t cnt = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
		__HAL_TIM_SET_COUNTER(&htim2, 0);
		return cnt;
	}
	//读右轮计数值，然后清零
int16_t Encoder_GetCount_Right(void)
	{
		int16_t cnt = (int16_t)__HAL_TIM_GET_COUNTER(&htim4);
		__HAL_TIM_SET_COUNTER(&htim4, 0);
		return cnt;
	}
	//读左轮速度
int16_t Encoder_GetSpeed_Left(void)
	{
		return s_speed_left;
	}
	//读右轮速度
int16_t Encoder_GetSpeed_Right(void)
	{
		return s_speed_right;
	}
	//计算速度
void Encoder_UpdateSpeed(void)
	 {
		int16_t Encoder_pluse_left = Encoder_GetCount_Left(); 
		int16_t Encoder_pluse_right = Encoder_GetCount_Right();   //一个采样周期脉冲数
		uint32_t now_time  = HAL_GetTick();
        uint32_t sample_gap= now_time - s_last_tick;     
        s_last_tick = now_time;               
		if(sample_gap == 0)
			return;
		s_speed_left =(int16_t) (((float)Encoder_pluse_left/(float) sample_gap *1000.0f * 60.0f) /(float) ENCODER_PULSE_PER_REV) ;
		s_speed_right =(int16_t) (((float)Encoder_pluse_right/(float) sample_gap *1000.0f * 60.0f) /(float) ENCODER_PULSE_PER_REV) ;   ///RPM
	 }
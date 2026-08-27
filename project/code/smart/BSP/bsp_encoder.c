#include "bsp_encoder.h"
#include "tim.h"   /* 拿 htim2/htim4 */


extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;

static int16_t s_speed_left  = 0;
static int16_t s_speed_right = 0;

void Encoder_Init(void)
	{
		/* 启动两个编码器定时器的计数 */
		HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
		HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);

		/* 计数清零 */
		__HAL_TIM_SET_COUNTER(&htim2, 0);
		__HAL_TIM_SET_COUNTER(&htim4, 0);
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
		 
		s_speed_left =(int16_t) (((float)Encoder_pluse_left/(float)ENCODER_SAMPLE_MS *1000.0f * 60.0f) /(float) ENCODER_PULSE_PER_REV) ;
		s_speed_right =(int16_t) (((float)Encoder_pluse_right/(float)ENCODER_SAMPLE_MS *1000.0f * 60.0f) /(float) ENCODER_PULSE_PER_REV) ;   ///RPM
	 }
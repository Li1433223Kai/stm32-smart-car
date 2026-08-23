#include "app.h"
#include "bsp_motor.h"
#include "bsp_gray.h"

/*
 * 循迹控制参数定义
 * 本阶段先"纯直行": 不读位置做差速, 只用固定 base 速度让两轮同速,
 * 目的是先验证两电机都能正常正转、base 速度选得合适(高于死区)。
 * Kp/差速逻辑下一阶段再加。
 */
#define BASE_SPEED      2000    /* 基础速度: 先给2000(高于左轮死区约600), 后面调 */

/*
 * 循迹模块初始化
 * 目前无额外配置, 保留接口方便后续扩展。
 */
void App_Init(void)
{
    /* 暂无需要初始化的内容 */
}

/*
 * 单次循迹控制 (由主循环按固定节拍调用)
 *
 * 本阶段: 直接给两轮固定基础速度, 观察是否直行、速度是否合适。
 * pos = Gray_GetPosition() 先读出来(便于调试打印), 但暂不参与差速。
 */
void App_Follow(void)
{
    int16_t pos;

    pos = Gray_GetPosition();
    (void)pos;    /* 本阶段暂不使用位置值, 防止未使用告警 */

    /* 差速逻辑(下一阶段): steer = Kp * pos, 左右 = base +/- steer, 再做下限钳位 */
    Motor_SetSpeed(MOTOR_A, BASE_SPEED);    /* A = 左轮 */
    Motor_SetSpeed(MOTOR_B, BASE_SPEED);    /* B = 右轮 */
}

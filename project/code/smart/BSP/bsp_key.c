#include "bsp_key.h"

static uint8_t  s_stable_level = 1;          /* 当前稳定电平: 1=高(松), 0=低(按) */
static uint32_t s_last_change  = 0;          /* 上次电平变化时刻 */
static uint8_t s_last_stable = 1;

void Key_Init(void)
{
    s_stable_level = (HAL_GPIO_ReadPin(key_GPIO_Port, key_Pin) == GPIO_PIN_SET) ? 1u : 0u;
    s_last_change  = HAL_GetTick();
    s_last_stable  = s_stable_level;   
}

uint8_t Key_Scan(void)
{
    uint8_t raw = (HAL_GPIO_ReadPin(key_GPIO_Port, key_Pin) == GPIO_PIN_SET) ? 1u : 0u;

    if (raw == s_stable_level)
    {
        /* 与稳定电平一致: 重置变化计时 */
        s_last_change = HAL_GetTick();
    }
    else
    {
        /* 电平与稳定态不同: 判断是否持续够久(主动去抖) */
        if (HAL_GetTick() - s_last_change >= KEY_WAIT_MS)   /* ①填时间差 */
        {
            s_stable_level = raw;                        /* 确认新稳定电平 */
        }
        /* 未到去抖时间: 保持原稳定态, 不切换 */
    }



    uint8_t edge = 0;
    if (s_stable_level == 0 && s_last_stable == 1)   /* 从松到按的跳变 */
        edge = 1;
    s_last_stable = s_stable_level;   /* 每次都要更新, 否则比较失效 */

    return edge;

}
//定义当前稳定，上次稳定电平为1，定义上次变换电平时刻，init初始化当前电平和时间;
//Key_IsPressed 定义raw为当前电平，假如与stable一致，重置时间，假如不一致，进入去抖判断：50ms内电平没变化（lastchange时间没刷新）则确认电平变化，raw赋值给stable
//定义边缘edge=0，假如stable = 0 laststable = 1 ，则有下降沿，edge = 1 更新lastsstable，最后返回edge
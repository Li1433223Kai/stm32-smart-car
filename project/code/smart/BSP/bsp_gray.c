#include "bsp_gray.h"

/*
 * 八路数字灰度传感器驱动
 *
 * 工作原理:
 *   模块通过AD0/AD1/AD2三根地址线(3-8译码器)选择8个通道中的一个,
 *   OUT引脚输出该通道的数字量检测结果(0=黑线, 1=白线)。
 *   依次切换地址线读取OUT引脚, 即可得到全部8路传感器状态。
 *
 * 接线对应:
 *   PA2 → AD0 (通道选择最低位)
 *   PA3 → AD1
 *   PA4 → AD2 (通道选择最高位)
 *   PA5 → OUT (数字量输出)
 *   VCC → 3.3V
 *   GND → GND
 *
 * 通道编号: 模块从左到右依次为通道0~7 (面向车头前方看)
 *   通道0 = 000 = 最左
 *   通道7 = 111 = 最右
 */

/*
 * 设置通道选择地址线
 * ch范围0~7, AD0=LSB, AD2=MSB
 */
static void Gray_SetChannel(uint8_t ch)
{
    HAL_GPIO_WritePin(GRAY_AD0_GPIO_Port, GRAY_AD0_Pin,
        (ch & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GRAY_AD1_GPIO_Port, GRAY_AD1_Pin,
        (ch & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GRAY_AD2_GPIO_Port, GRAY_AD2_Pin,
        (ch & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/*
 * 灰度传感器初始化
 * GPIO已在MX_GPIO_Init中配置好(AD0/AD1/AD2推挽输出, OUT浮空输入),
 * 这里不需要额外操作, 留空占位以便后续添加校准功能。
 */
void Gray_Init(void)
{
    /* AD0/AD1/AD2初始输出0, 选中通道0 */
    Gray_SetChannel(0);
}

/*
 * 读取单个灰度通道
 * ch: 通道号0~7
 * 返回值: GRAY_BLACK(0)=检测到黑线, GRAY_WHITE(1)=检测到白线
 */
uint8_t Gray_ReadChannel(uint8_t ch)
{
    if (ch >= GRAY_CHANNEL_NUM)
        {
            return GRAY_WHITE;  /* 无效通道返回白 */
        }

    /* 设置通道地址 */
    Gray_SetChannel(ch);

    /* 短暂延时等待多路选择器和比较器输出稳定
     * 74HC4051/类似模拟开关切换时间约几十ns, 加上LM393比较器响应时间几us,
     * 这里简单循环延时约5~10us足够 */
    for (volatile uint8_t i = 0; i < 10; i++)
        {
            __NOP();
        }

    /* 读取OUT引脚: 高电平=白(反射强), 低电平=黑(反射弱) */
    if (HAL_GPIO_ReadPin(GRAY_OUT_GPIO_Port, GRAY_OUT_Pin) == GPIO_PIN_SET)
        {
            return GRAY_WHITE;
        }
    else
        {
            return GRAY_BLACK;
        }
}

/*
 * 读取全部8路灰度值
 * values[0~7]: 从左到右8个传感器的状态, GRAY_BLACK或GRAY_WHITE
 */
void Gray_ReadAll(uint8_t values[GRAY_CHANNEL_NUM])
{
    uint8_t i;
    for (i = 0; i < GRAY_CHANNEL_NUM; i++)
        {
            values[i] = Gray_ReadChannel(i);
        }
}

/*
 * 计算黑线加权位置(用于循迹PID)
 *
 * 算法: 给每个通道分配权重(最左=-7000, 最右=+7000),
 *       只对检测到黑线(GRAY_BLACK)的通道累加权重,
 *       然后除以压在黑线上的传感器数量, 得到黑线中心位置。
 *
 * 返回值: -7000 ~ +7000
 *          0       = 黑线在正中(通道3和4中间)
 *          负值    = 黑线偏左
 *          正值    = 黑线偏右
 *          GRAY_ALL_WHITE(9999)  = 8路全白(丢线)
 *          GRAY_ALL_BLACK(-9999) = 8路全黑(十字线/起跑线)
 *
 * 注意: 全黑时也可能是传感器离地了, 上层逻辑需要处理
 */
int16_t Gray_GetPosition(void)
{
    uint8_t values[GRAY_CHANNEL_NUM];
    int32_t weighted_sum = 0;
    int16_t black_count = 0;
    uint8_t i;

    /* 权重表: 8个通道从左到右的权重值
     * 间距2000, 这样2~3个传感器同时压线时位置值连续平滑 */
    static const int16_t weights[GRAY_CHANNEL_NUM] = {
        -7000, -5000, -3000, -1000, 1000, 3000, 5000, 7000
    };

    Gray_ReadAll(values);

    for (i = 0; i < GRAY_CHANNEL_NUM; i++)
        {
            if (values[i] == GRAY_BLACK)
                {
                    weighted_sum += weights[i];
                    black_count++;
                }
        }

    if (black_count == 0)
        {
            return GRAY_ALL_WHITE;    /* 全白, 丢线 */
        }
    if (black_count == GRAY_CHANNEL_NUM)
        {
            return GRAY_ALL_BLACK;    /* 全黑, 十字线/起跑线 */
        }

    /* 加权平均 */
    return (int16_t)(weighted_sum / black_count);
}

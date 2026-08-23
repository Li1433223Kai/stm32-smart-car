#include "bsp_gray.h"

/*
 * 亚博八路数字灰度传感器驱动 (蓝光版, 白底黑线循迹)
 *
 * 工作原理:
 *   模块使用CD4051模拟开关, 通过AD0/AD1/AD2三根地址线选择8个通道之一,
 *   OUT引脚输出该通道的数字量检测结果。依次切换地址线读取OUT引脚,
 *   即可得到全部8路传感器状态。
 *
 * 接线对应:
 *   VCC → 5V          (注意: 是5V不是3.3V!)
 *   GND → GND
 *   PA2 → AD0 (通道选择最低位)
 *   PA3 → AD1
 *   PA4 → AD2 (通道选择最高位)
 *   PA5 → OUT (数字量输出, 灯亮=1=黑线)
 *
 * 通道编号: 模块丝印X1~X8从左到右 (面向车头前方看, 探头朝下)
 *   AD2=0,AD1=0,AD0=0 → CH1(最左, 数组索引0)
 *   AD2=1,AD1=1,AD0=1 → CH8(最右, 数组索引7)
 *
 */

/*
 * 简单微秒级延时
 
 * 72MHz主频下, 约1us需要72个时钟周期, 循环体大约4个周期/次,
 * 所以一次循环约0.055us, 延时us * 18 ≈ 所需循环次数
 */
static void Gray_Delay_us(uint32_t us)
{
    volatile uint32_t count = us * 18;
    while (count--)
        {
            __NOP();
        }
}

/*
 * 设置通道选择地址线
 * ch范围0~7, 对应CH1~CH8, AD0=LSB, AD2=MSB
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
 * 这里设置初始通道并等待模块稳定。
 */
void Gray_Init(void)
{
    Gray_SetChannel(0);
    Gray_Delay_us(50);    /* 等待模块上电稳定 */
}

/*
 * 读取单个灰度通道
 * ch: 通道号0~7 (对应CH1~CH8)
 * 返回值: GRAY_BLACK(1)=检测到黑线(灯亮), GRAY_WHITE(0)=白色地面(灯灭)
 */
uint8_t Gray_ReadChannel(uint8_t ch)
{
    if (ch >= GRAY_CHANNEL_NUM)
        {
            return GRAY_WHITE;    /* 无效通道返回白 */
        }

    /* 设置通道地址 */
    Gray_SetChannel(ch);

    /*等待CD4051切换和LM393比较器输出稳定*/
    Gray_Delay_us(50);

    /* 读取OUT引脚:
     * 高电平(SET) = 灯亮 = 检测到黑线 = GRAY_BLACK
     * 低电平(RESET) = 灯灭 = 白色地面 = GRAY_WHITE */
    if (HAL_GPIO_ReadPin(GRAY_OUT_GPIO_Port, GRAY_OUT_Pin) == GPIO_PIN_SET)
        {
            return GRAY_BLACK;
        }
    else
        {
            return GRAY_WHITE;
        }
}

/*
 * 读取全部8路灰度值
 * values[0~7]: 从左到右8个传感器的状态, GRAY_BLACK(1)或GRAY_WHITE(0)
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
 *          负值    = 黑线偏左(车需要左转)
 *          0       = 黑线在正中(直行)
 *          正值    = 黑线偏右(车需要右转)
 *          GRAY_ALL_WHITE(9999)  = 8路全白(丢线)
 *          GRAY_ALL_BLACK(-9999) = 8路全黑(十字线/起跑线)
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

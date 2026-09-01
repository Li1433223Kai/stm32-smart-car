# 项目交接文档 (Handoff)

> 更新时间: 2026-09-01
> 项目: 基于 STM32F103C8T6 的智能循迹小车
> 代码位置: `project/code/smart/` (Keil MDK 工程, Target: f103)
> 最新提交: (状态机重构 + 9.1 小改进尚未 commit, 提交后填 hash)

---

## 一、项目概况

- **主控**: STM32F103C8T6
- **驱动**: TB6612 双路（D153C）
- **电机**: MG310 直流减速电机（霍尔编码器，13 线，减速比 20）
- **传感器**: 8 路数字灰度
- **供电**: 12V 锂电池 → DCP3512 电源模块（4路3.3V/5V/12V/ADJ）→ 板子/电机/灰度
- **功能**: 白底黑线循迹小车，**速度环 PI + 位置环 P 双闭环**差速循迹

## 二、我做了什么（已完成）

### 1. 电机驱动 `bsp_motor.c/h`
- TB6612 双路，PWM(0~7200) + 方向控制
- 方向安全时序：先关 PWM → 改方向 → 恢复 PWM
- 引脚：PWMA=PB1, PWMB=PB0, AIN1/2=PB10/PB11, BIN1/2=PB12/PB13

### 2. 灰度 `bsp_gray.c/h`
- 8 路数字灰度，加权计算黑线位置
- `Gray_GetPosition()`: -7000~+7000，0居中，负偏左，正偏右；9999全白，-9999全黑
- 通道切换必须等 50us（CD4051）

### 3. 编码器测速 `bsp_encoder.c/h`
- TIM2(左)/TIM4(右)，4倍频，每圈脉冲数 = 20×13×4 = 1040
- 10ms 采样算 RPM
- 引脚：左 A=PA1/B=PA0，右 A=PB6/B=PB7

### 4. 控制层 `app.c/h`（核心）
- **位置环（外环）** `App_Position_Control()`：读灰度 pos → P 控制算左右目标速度(RPM)，差速转向
- **速度环（内环）** `Speed_Control()`：编码器反馈 → PI 控制 → PWM
- 整数限幅：`PID_KP=15, PID_KI=1, INTEGRAL_MAX=2000, PWM_LIMIT=7200`
- 位置环参数：`KP=1, BASE_SPEED=150, TARGET_MIN=100, TARGET_MAX=200, MAX_STEER=50`
- 转向量：`steer = KP * (pos/7000) * MAX_STEER`
- **状态机**：`App_State_Update()` 每 10ms 调度 IDLE/RUN/LOST/CROSS。IDLE 停车按 PA8 启动；RUN 正常压线跑位置环、全白→LOST、全黑→CROSS；LOST/CROSS 刚进入快速刹车一次、停留 target=0 保持静止，**按 PA8 可回到 RUN 重新起步**（不用断电重启）

### 5. VOFA+ 调参
- 串口 115200，USART1(PA9/PA10)
- printf 用纯 CSV `%d,%d\n`（FireWater），带标签会只出一个通道

### 6. 9.1 小改进（已完成）
- **编码器引脚上拉**：PA0/PA1/PB6/PB7 改 `GPIO_PULLUP`（tim.c + f103.ioc 同步），信号线虚接时计数不乱跳
- **刹车策略**：新增 `Motor_BrakeAll()`；LOST/CROSS 刚进入快速刹车一次，停留时 target=0 保持静止
- **按键恢复**：LOST/CROSS 按下 PA8 回 RUN 重新起步，丢线/十字停车后不用断电重启

### 7. 文档整理（done）
- `docs/调试记录.md`（19 条踩坑）、`模块说明.md`、`硬件连接.md`、`项目参数.md`、`版本说明.md`、`开发日志.md`
- `docs/项目总结.md` —— **空，未写**

---

## 三、我的目标

- **短期**：把循迹小车做成能体现工程能力的项目（文档+功能），作为**嵌入式软件**方向的简历/面试素材
- **中期**：先精软件（单片机开发，STM32→Linux 驱动），找嵌入式软件工作
- **长远**：T 型发展，软件为主、逐步补硬件能力

---

## 四、接下来要做什么（未完成）

### 1. OLED 显示（代码已完成，待新屏实测）
- 江科大套件 0.96 寸 4 针 I2C OLED（SSD1306，地址 0x3C）
- **方案已定：硬件 I2C1 重映射到 PB8(SCL)/PB9(SDA)**，3.3V 供电
- **已完成**：`BSP/bsp_oled.c/.h`（标准 SSD1306 ASCII 驱动，基于 HAL_I2C_Mem_Write，8x16 字模，显示坐标 row0~3/col0~15）；`app.c` 增加 `App_OLED_Show()` 刷新数据面板；`main.c` 集成 OLED_Init + 每100ms刷新；`bsp_oled.c/.h` 已加入 Keil 工程 `f103.uvprojx`；CubeMX 的 I2C1 重映射 PB8/PB9 已生成（i2c.c 含 `__HAL_AFIO_REMAP_I2C1_ENABLE()`）
- **面板布局（4行）**：行0 ST:状态 / 行1 目标速度 "T 150/150" / 行2 实测速度 "L 120 R 118" / 行3 位置偏差 "Pos 1250"；每100ms 刷新一次
- **待办**：① 旧屏烧了，等新屏到货实测；② `main.c` 里 OLED 刷新代码目前被注释（135-139 行），实测时取消注释即可
- **注意**：CubeMX 重新生成 .uvprojx 时可能会清掉手工加入的 bsp_oled.c/.h，生成后需确认这两个文件仍在工程里

### 2. 状态机重构（已完成）
- `app.c/h`：`AppState_t` 枚举 IDLE/RUN/LOST/CROSS，`App_State_Update()` 每 10ms 调度，替换原 if-else
- 转移：IDLE→RUN(按 PA8 启动)，RUN→LOST(全白丢线)，RUN→CROSS(全黑)，LOST/CROSS 停车（不做自动回找）
- `bsp_key.c/h`：PA8 上拉启停按键，50ms 非阻塞去抖 + 沿检测 `Key_Scan()`（返回按下沿，一次点击返 1）
- `main.c`：加 `Key_Init()`，主循环调 `App_State_Update()`；控制节拍 `CONTROL_TICK_MS=ENCODER_SAMPLE_MS`
- 坑：全黑/全白 `pos=-9999/9999` 要先判 pos 再跑位置环，否则算出错误差速轮子疯转

### 3. 其他（串口打印问题已解决）
- 坑：fputc 用 `HAL_UART_Transmit(..., HAL_MAX_DELAY)` 无限等待，串口一慢/一断就阻塞主循环，拖慢 10ms 节拍导致速度环采样错乱、轮子反转疯转
- 已修：fputc 超时改 50ms（异常放弃发送不死等）；打印限频 100ms→500ms

### 4. 待做（晚点）
- **OLED**：bsp_oled 已就绪，还差 CubeMX 里给 I2C1 选 SCL=PB8/SDA=PB9 重映射并重新生成（旧屏烧了，需换新屏）
- **PC13 指示灯**：当前常亮不闪，主循环飞快空转时 toggle 太快肉眼看不出，属观察误判，非功能问题（暂不管）
- **串口协议 + 上位机调参**：下发 PID/速度参数
- **全黑十字线/起跑线识别**：复用 CROSS 检测

---

## 五、关键坑（已解决）

1. 编码器 A/B 相接反 → 左 A=PA1/B=PA0 方向才对（待确认右是否也反）
2. 速度环 Kp=50 变速震荡 → 降到 15 才稳
3. 纯 P 有静差 → 加 Ki=1 消除
4. VOFA+ 带标签只出一个通道 → 用纯 CSV
5. 灰度地板不够白会误判（8号灯常亮）→ 此模块不能调阈值，需白底场地
6. TB6612 PWM 脚悬空会抖 / 驱动板反接烧板 / 电机个体差异（左轮死区>600）

---

## 六、代码/文档速查

- 主程序：`Core/Src/main.c`（初始化 + 主循环，10ms 调 App_State_Update + 速度环）
- 控制逻辑：`BSP/app.c`（状态机 + 位置环 + 速度环 + OLED/IPrintf）
- 按键：`BSP/bsp_key.c`（PA8 启停，去抖 + 沿检测）
- 驱动：`BSP/bsp_motor.c`, `bsp_gray.c`, `bsp_encoder.c`
- 文档：`project/docs/`（调试记录/模块说明/硬件连接/项目参数/版本说明/开发日志）

---

## 七、恢复上下文方法

下次直接说："读 handoff.md，继续做 OLED + 状态机"。或按项目情况推进。

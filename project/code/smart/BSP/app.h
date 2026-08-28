#ifndef __APP_H
#define __APP_H

/*
 * 应用层/业务层函数声明
 * 循迹控制逻辑放在这一层, 由 main.c 主循环按固定节拍调用。
 * 这里不直接操作寄存器, 只调用 bsp_motor / bsp_gray 的现成接口。
 */
void App_Init(void);

void App_Position_Control(void);
void APP_Speed_Control(void);

/* 刷新 OLED 数据面板(状态/左右速度/目标速度/pos偏差), 由 main 按节拍调用 */
void App_OLED_Show(void);

#endif

#ifndef __APP_H
#define __APP_H

/*
 * 应用层/业务层函数声明
 * 循迹控制逻辑放在这一层, 由 main.c 主循环按固定节拍调用。
 * 这里不直接操作寄存器, 只调用 bsp_motor / bsp_gray 的现成接口。
 */

/* 循迹模块初始化(目前无需额外配置, 预留) */
void App_Init(void);

/* 单次循迹控制: 读位置 -> 算差速 -> 钳位 -> 设电机。
 * 由 main.c 主循环按固定节拍(PERIOD)调用 */
void App_Follow(void);

#endif

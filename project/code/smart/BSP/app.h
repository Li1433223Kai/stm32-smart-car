#ifndef __APP_H
#define __APP_H

/*
 * 应用层/业务层函数声明
 * 循迹控制逻辑放在这一层, 由 main.c 主循环按固定节拍调用。
 * 这里不直接操作寄存器, 只调用 bsp_motor / bsp_gray 的现成接口。
 */
void App_Init(void);

void App_Follow(void);

void Speed_Control(void);

#endif

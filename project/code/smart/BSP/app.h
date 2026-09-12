#ifndef __APP_H
#define __APP_H

#include "main.h"   /* 提供 int16_t 等标准类型(与其它 BSP 头文件一致) */

typedef enum {
    APP_STATE_IDLE  = 0,   /* 停车待命, 等按键启动 */
    APP_STATE_RUN,         /* 正常循迹 */
    APP_STATE_LOST,        /* 全白丢线 */
    APP_STATE_CROSS        /* 全黑十字/起点 */
} AppState_t;

void App_Init(void);
void App_State_Update(void);
void App_Position_Control(int16_t pos);   /* pos 由调用者传入, 避免重复读灰度 */
void APP_Speed_Control(void);


void App_OLED_Show(void);

#endif
